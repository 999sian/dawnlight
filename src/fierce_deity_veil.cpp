#include "fierce_deity_veil.hpp"
#include "fierce_deity_veil_mesh.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_view.h"
#include "mods/svc/hook.hpp"

#include <chrono>
#include <cstring>
#include <mutex>

namespace dawnlight {
namespace {
DEFINE_HOOK(&dComIfGd_drawXluListDark, DarkVeilTranslucentHook);
using namespace dark_veil;
using Clock=std::chrono::steady_clock;
using LookupPresentationMatrix=bool (*)(const void*,Mtx);
LookupPresentationMatrix sPresented=nullptr;
GfxDrawTypeHandle sDraw=0;
GfxStageHookHandle sStage=0,sBegin=0,sEnd=0;
bool sHook=false;
fpc_ProcID sOwner=fpcM_ERROR_PROCESS_ID_e;
Motion sMotion;
Clock::time_point sLast{},sOrigin{};
float sFade=0;
struct GpuVertex { float clip[4],field[3],opacity; };
struct Draw { GfxRange vertices; uint32_t count; };
Draw sPending{};
bool sHasPending=false;
std::vector<Vertex> sMesh;
std::vector<GpuVertex> sVertices;

// GPU objects are used on the worker only, with shutdown serialized against it.
std::mutex sGpuMutex;
WGPUShaderModule sShader=nullptr;
struct Pipeline { uint64_t layout; bool reversed; WGPURenderPipeline handle; };
std::vector<Pipeline> sPipelines;
constexpr const char* kShader=
#include "fierce_deity_veil_shader.inc"
;
WGPUStringView label(const char* s) { return {s,std::strlen(s)}; }
WGPURenderPipeline pipeline_for(const GfxDrawContext& ctx) {
    for (const auto& p:sPipelines)
        if (p.layout==ctx.layout.key && p.reversed==ctx.uses_reversed_z) return p.handle;
    if (!sShader) {
        WGPUShaderSourceWGSL source=WGPU_SHADER_SOURCE_WGSL_INIT;
        source.code=label(kShader);
        WGPUShaderModuleDescriptor desc=WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
        desc.nextInChain=&source.chain;
        sShader=wgpuDeviceCreateShaderModule(ctx.device,&desc);
    }
    if (!sShader) return nullptr;
    WGPUVertexAttribute attributes[3]={WGPU_VERTEX_ATTRIBUTE_INIT,WGPU_VERTEX_ATTRIBUTE_INIT,WGPU_VERTEX_ATTRIBUTE_INIT};
    attributes[0].format=WGPUVertexFormat_Float32x4;
    attributes[1].format=WGPUVertexFormat_Float32x3; attributes[1].offset=offsetof(GpuVertex,field);
    attributes[2].format=WGPUVertexFormat_Float32; attributes[2].offset=offsetof(GpuVertex,opacity);
    for (unsigned i=0;i<3;++i) attributes[i].shaderLocation=i;
    WGPUVertexBufferLayout buffer=WGPU_VERTEX_BUFFER_LAYOUT_INIT;
    buffer.arrayStride=sizeof(GpuVertex); buffer.attributeCount=3; buffer.attributes=attributes;
    WGPUBlendState blend=WGPU_BLEND_STATE_INIT;
    blend.color.srcFactor=WGPUBlendFactor_SrcAlpha;
    blend.color.dstFactor=WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.srcFactor=WGPUBlendFactor_Zero; blend.alpha.dstFactor=WGPUBlendFactor_One;
    WGPUColorTargetState targets[GFX_MAX_COLOR_ATTACHMENTS];
    WGPUFragmentState fragment=WGPU_FRAGMENT_STATE_INIT;
    fragment.module=sShader; fragment.entryPoint=label("fs");
    fragment.targetCount=gfx_init_color_target_states(&ctx.layout,targets,&blend,
        WGPUColorWriteMask_Red|WGPUColorWriteMask_Green|WGPUColorWriteMask_Blue);
    fragment.targets=targets;
    WGPUDepthStencilState depth=WGPU_DEPTH_STENCIL_STATE_INIT;
    depth.format=ctx.layout.depth_stencil_format;
    depth.depthWriteEnabled=WGPUOptionalBool_False;
    depth.depthCompare=ctx.uses_reversed_z ? WGPUCompareFunction_GreaterEqual:WGPUCompareFunction_LessEqual;
    WGPURenderPipelineDescriptor desc=WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.label=label("Dark Link attached veil");
    desc.vertex.module=sShader; desc.vertex.entryPoint=label("vs");
    desc.vertex.bufferCount=1; desc.vertex.buffers=&buffer;
    desc.primitive.topology=WGPUPrimitiveTopology_TriangleList;
    desc.primitive.cullMode=WGPUCullMode_None;
    desc.depthStencil=depth.format==WGPUTextureFormat_Undefined ? nullptr:&depth;
    desc.multisample.count=ctx.layout.sample_count;
    desc.fragment=&fragment;
    auto pipeline=wgpuDeviceCreateRenderPipeline(ctx.device,&desc);
    if (pipeline) sPipelines.push_back({ctx.layout.key,ctx.uses_reversed_z,pipeline});
    return pipeline;
}
void draw(ModContext*,const GfxDrawContext* ctx,const void* data,size_t size,void*) {
    if (size!=sizeof(Draw)) return;
    std::lock_guard lock(sGpuMutex);
    auto pipeline=pipeline_for(*ctx);
    if (!pipeline) return;
    Draw batch; std::memcpy(&batch,data,sizeof(batch));
    wgpuRenderPassEncoderSetPipeline(ctx->pass,pipeline);
    wgpuRenderPassEncoderSetVertexBuffer(ctx->pass,0,ctx->vertex_buffer,batch.vertices.offset,batch.vertices.size);
    wgpuRenderPassEncoderDraw(ctx->pass,batch.count,1,0,0);
}
void reset_motion() { sMotion.reset(); sLast={}; sOrigin={}; sFade=0; }
bool drawable(daAlink_c* link) {
    return link && !link->checkPlayerNoDraw() && !link->checkStatusWindowDraw() &&
        !link->checkWolf() && !link->checkDeadHP() && !link->checkEventRun() &&
        !link->checkSceneChangeAreaStart() && link->mClothesChangeWaitTimer==0 &&
        link->mpLinkModel && link->mpLinkModel->getModelData() &&
        link->mpLinkModel->getModelData()->getJointNum()>kJoints.back();
}
bool copy_pose(daAlink_c* link,unsigned joint,Pose& pose) {
    auto* matrix=link->mpLinkModel->getAnmMtx(joint);
    if (!matrix) return false;
    Mtx presented;
    if (sPresented && sPresented(matrix,presented)) matrix=presented;
    for (unsigned r=0;r<3;++r) for (unsigned c=0;c<4;++c) pose[r][c]=matrix[r][c];
    return finite(pose);
}
void discard(ModContext*,const GfxStageContext*,void*) { sHasPending=false; }
void stage(ModContext*,const GfxStageContext* ctx,void*) {
    sHasPending=false;
    if (sOwner==fpcM_ERROR_PROCESS_ID_e || !ctx->game_view) return;
    auto* link=daAlink_getAlinkActorClass();
    if (!link || fopAcM_GetID(link)!=sOwner || !drawable(link)) { reset_motion(); return; }
    std::array<Pose,kJoints.size()> poses{};
    Pose root;
    if (!copy_pose(link,0,root)) { reset_motion(); return; }
    for (unsigned i=0;i<kJoints.size();++i)
        if (!copy_pose(link,kJoints[i],poses[i])) { reset_motion(); return; }
    const auto now=Clock::now();
    if (sLast==Clock::time_point{}) { sOrigin=now; sLast=now; }
    const float dt=std::chrono::duration<float>(now-sLast).count();
    sLast=now;
    sFade=std::min(1.0f,sFade+std::clamp(dt,0.0f,0.05f)*5);
    const float seconds=std::chrono::duration<float>(now-sOrigin).count();
    sMotion.update(poses,origin(root),dt,seconds);
    const auto& view=*static_cast<const view_class*>(ctx->game_view);
    const auto& eye=view.lookat.eye;
    build(poses,root,{eye.x,eye.y,eye.z},sMotion,seconds,sFade,sMesh);
    if (sMesh.empty()) return;
    GfxDeviceInfo device=GFX_DEVICE_INFO_INIT;
    if (svc_gfx->get_device_info(mod_ctx,&device)!=MOD_OK) return;
    sVertices.clear(); sVertices.reserve(kVertexCount);
    for (const auto& v:sMesh) {
        GpuVertex vertex{};
        const float world[3]={v.world.x,v.world.y,v.world.z};
        float camera[3];
        for (unsigned r=0;r<3;++r) {
            camera[r]=view.viewMtx[r][3];
            for (unsigned c=0;c<3;++c) camera[r]+=view.viewMtx[r][c]*world[c];
        }
        for (unsigned r=0;r<4;++r)
            vertex.clip[r]=view.projMtx[r][0]*camera[0]+view.projMtx[r][1]*camera[1]+
                view.projMtx[r][2]*camera[2]+view.projMtx[r][3];
        // Match Aurora's conversion from GX [-w,0] clip depth to WebGPU.
        vertex.clip[2]=device.uses_reversed_z ? -vertex.clip[2]:vertex.clip[2]+vertex.clip[3];
        vertex.field[0]=v.field.x; vertex.field[1]=v.field.y; vertex.field[2]=v.field.z;
        vertex.opacity=v.opacity;
        sVertices.push_back(vertex);
    }
    Draw batch{{},static_cast<uint32_t>(sVertices.size())};
    if (svc_gfx->push_verts(mod_ctx,sVertices.data(),sVertices.size()*sizeof(GpuVertex),alignof(GpuVertex),&batch.vertices)==MOD_OK) {
        sPending=batch; sHasPending=true;
    }
}
void after_translucent(ModContext*,void*,void*,void*) {
    if (!sHasPending) return;
    sHasPending=false;
    // After world XLU/fog overlays, before post-processing and HUD. Occlusion
    // still uses scene depth; neither depth, alpha nor normal targets are written.
    svc_gfx->push_draw(mod_ctx,sDraw,&sPending,sizeof(sPending));
}
}
void tick_dark_veil(daAlink_c* link,bool enabled) {
    if (!enabled || !drawable(link)) { stop_dark_veil(); return; }
    const auto id=fopAcM_GetID(link);
    if (sOwner!=id) { stop_dark_veil(); sOwner=id; }
}
void stop_dark_veil() { sOwner=fpcM_ERROR_PROCESS_ID_e; sHasPending=false; reset_motion(); }
void initialize_dark_veil() {
    if (kUseLegacyDarkFlames || !svc_gfx || !svc_hook || sDraw) return;
    void* address=nullptr;
    if (svc_hook->resolve && svc_hook->resolve(mod_ctx,"dusk::interp::lookup_replacement",&address,nullptr)==MOD_OK)
        sPresented=reinterpret_cast<LookupPresentationMatrix>(address);
    const GfxDrawTypeDesc drawDesc={sizeof(GfxDrawTypeDesc),"Dark Link veil",draw,nullptr};
    const GfxStageHookDesc stageDesc={sizeof(GfxStageHookDesc),stage,nullptr};
    const GfxStageHookDesc discardDesc={sizeof(GfxStageHookDesc),discard,nullptr};
    if (svc_gfx->register_draw_type(mod_ctx,&drawDesc,&sDraw)!=MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_SCENE_BEGIN,&discardDesc,&sBegin)!=MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_SCENE_AFTER_OPAQUE,&stageDesc,&sStage)!=MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_FRAME_BEFORE_HUD,&discardDesc,&sEnd)!=MOD_OK) {
        shutdown_dark_veil(); svc_log->warn(mod_ctx,"Dark Link veil: graphics registration failed"); return;
    }
    if (mods::hook::add_post<DarkVeilTranslucentHook>(svc_hook,after_translucent)!=MOD_OK) {
        mods::hook::uninstall<DarkVeilTranslucentHook>(svc_hook);
        shutdown_dark_veil(); svc_log->warn(mod_ctx,"Dark Link veil: translucent hook failed"); return;
    }
    sHook=true;
}
void shutdown_dark_veil() {
    stop_dark_veil();
    if (sHook) mods::hook::uninstall<DarkVeilTranslucentHook>(svc_hook);
    sHook=false;
    if (sBegin) svc_gfx->unregister_stage_hook(mod_ctx,sBegin);
    if (sStage) svc_gfx->unregister_stage_hook(mod_ctx,sStage);
    if (sEnd) svc_gfx->unregister_stage_hook(mod_ctx,sEnd);
    if (sDraw) svc_gfx->unregister_draw_type(mod_ctx,sDraw);
    sBegin=sStage=sEnd=sDraw=0; sPresented=nullptr;
    sMesh.clear(); sVertices.clear();
    std::lock_guard lock(sGpuMutex);
    for (const auto& p:sPipelines) wgpuRenderPipelineRelease(p.handle);
    sPipelines.clear();
    if (sShader) wgpuShaderModuleRelease(sShader);
    sShader=nullptr;
}
}

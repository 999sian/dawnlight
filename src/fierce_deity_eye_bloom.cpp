#include "fierce_deity_eye_bloom.hpp"
#include "service_imports.hpp"
#include "m_Do/m_Do_machine.h"
#include "dolphin/gx/GXAurora.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstring>
#include <mutex>
#include <vector>

namespace dawnlight {
namespace {
constexpr const char* kShader =
#include "fierce_deity_eye_bloom_shader.inc"
;
WGPUStringView label(const char* value) { return {value, std::strlen(value)}; }

// Game-thread state. Borrowed views never survive a scene/frame boundary.
GfxDrawTypeHandle sDraw = 0;
GfxComputeTypeHandle sCompute = 0;
GfxStageHookHandle sBegin = 0, sComposite = 0, sEnd = 0;
struct Mask { WGPUTextureView color, depth; uint32_t width, height; };
std::array<Mask, 8> sMasks{};
unsigned sMaskCount = 0;
bool sScene = false, sCapturing = false;
std::array<float, 6> sViewport{};
std::array<u32, 4> sScissor{};
struct Job { Mask mask; WGPUTextureView sceneDepth; bool reversed; };
static_assert(sizeof(Job) <= GFX_INLINE_DRAW_PAYLOAD_SIZE);

// Only worker callbacks touch GPU resources; the mutex also protects shutdown.
// Jobs execute compute -> composite in order, so two scratch textures suffice
// for all eyes/frames. Resizing releases references after the previous encoding;
// WebGPU retains resources referenced by already encoded commands.
std::mutex sGpuMutex;
WGPUShaderModule sShader = nullptr;
WGPUComputePipeline sExtract[2]{};
WGPUComputePipeline sHorizontal = nullptr;
WGPUSampler sSampler = nullptr;
WGPUTexture sTextures[2]{};
WGPUTextureView sViews[2]{};
uint32_t sWidth = 0, sHeight = 0;
bool sReady = false;
struct Pipeline { uint64_t key; WGPURenderPipeline handle; };
std::vector<Pipeline> sPipelines;

bool prepare(WGPUDevice device, uint32_t width, uint32_t height) {
    if (!sShader) {
        WGPUShaderSourceWGSL source = WGPU_SHADER_SOURCE_WGSL_INIT;
        source.code = label(kShader);
        WGPUShaderModuleDescriptor desc = WGPU_SHADER_MODULE_DESCRIPTOR_INIT;
        desc.nextInChain = &source.chain;
        sShader = wgpuDeviceCreateShaderModule(device, &desc);
        for (unsigned reversed = 0; reversed < 2; ++reversed) {
            WGPUConstantEntry constant = WGPU_CONSTANT_ENTRY_INIT;
            constant.key = label("reversedZ"); constant.value = reversed;
            WGPUComputePipelineDescriptor pipeline = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
            pipeline.compute.module = sShader;
            pipeline.compute.entryPoint = label("extract");
            pipeline.compute.constantCount = 1;
            pipeline.compute.constants = &constant;
            sExtract[reversed] = wgpuDeviceCreateComputePipeline(device, &pipeline);
        }
        WGPUComputePipelineDescriptor pipeline = WGPU_COMPUTE_PIPELINE_DESCRIPTOR_INIT;
        pipeline.compute.module = sShader;
        pipeline.compute.entryPoint = label("horizontal");
        sHorizontal = wgpuDeviceCreateComputePipeline(device, &pipeline);
        WGPUSamplerDescriptor sampler = WGPU_SAMPLER_DESCRIPTOR_INIT;
        sampler.minFilter = sampler.magFilter = WGPUFilterMode_Linear;
        sSampler = wgpuDeviceCreateSampler(device, &sampler);
    }
    if (width != sWidth || height != sHeight) {
        for (unsigned i = 0; i < 2; ++i) {
            if (sViews[i]) wgpuTextureViewRelease(sViews[i]);
            if (sTextures[i]) wgpuTextureRelease(sTextures[i]);
            WGPUTextureDescriptor texture = WGPU_TEXTURE_DESCRIPTOR_INIT;
            texture.label = label("Dark Link eye bloom scratch");
            texture.size = {width, height, 1};
            texture.format = WGPUTextureFormat_RGBA16Float;
            texture.usage = WGPUTextureUsage_StorageBinding | WGPUTextureUsage_TextureBinding;
            sTextures[i] = wgpuDeviceCreateTexture(device, &texture);
            sViews[i] = sTextures[i] ? wgpuTextureCreateView(sTextures[i], nullptr) : nullptr;
        }
        sWidth = width; sHeight = height;
    }
    return sShader && sExtract[0] && sExtract[1] && sHorizontal && sSampler && sViews[0] && sViews[1];
}

WGPUBindGroup bindings(WGPUDevice device, WGPUBindGroupLayout layout,
                       const WGPUBindGroupEntry* entries, size_t count) {
    WGPUBindGroupDescriptor desc = WGPU_BIND_GROUP_DESCRIPTOR_INIT;
    desc.layout = layout; desc.entryCount = count; desc.entries = entries;
    auto group = wgpuDeviceCreateBindGroup(device, &desc);
    wgpuBindGroupLayoutRelease(layout);
    return group;
}
void dispatch(WGPUCommandEncoder encoder, WGPUComputePipeline pipeline, WGPUBindGroup group) {
    WGPUComputePassDescriptor desc = WGPU_COMPUTE_PASS_DESCRIPTOR_INIT;
    auto pass = wgpuCommandEncoderBeginComputePass(encoder, &desc);
    wgpuComputePassEncoderSetPipeline(pass, pipeline);
    wgpuComputePassEncoderSetBindGroup(pass, 0, group, 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, (sWidth + 7) / 8, (sHeight + 7) / 8, 1);
    wgpuComputePassEncoderEnd(pass);
    wgpuComputePassEncoderRelease(pass);
    wgpuBindGroupRelease(group);
}
void compute(ModContext*, const GfxComputeContext* ctx, const void* data, size_t size, void*) {
    std::lock_guard lock(sGpuMutex);
    sReady = false;
    if (size != sizeof(Job)) return;
    Job job; std::memcpy(&job, data, sizeof(job));
    if (!prepare(ctx->device, (job.mask.width + 1) / 2, (job.mask.height + 1) / 2)) return;
    WGPUBindGroupEntry entries[4];
    const WGPUTextureView views[] = {job.mask.color, job.mask.depth, job.sceneDepth, sViews[0]};
    for (unsigned i = 0; i < 4; ++i) {
        entries[i] = WGPU_BIND_GROUP_ENTRY_INIT;
        entries[i].binding = i; entries[i].textureView = views[i];
    }
    auto extract = sExtract[job.reversed ? 1 : 0];
    auto group = bindings(ctx->device, wgpuComputePipelineGetBindGroupLayout(extract, 0), entries, 4);
    if (!group) return;
    dispatch(ctx->encoder, extract, group);
    for (unsigned i = 0; i < 2; ++i) {
        entries[i].binding = i + 4; entries[i].textureView = sViews[i];
    }
    group = bindings(ctx->device, wgpuComputePipelineGetBindGroupLayout(sHorizontal, 0), entries, 2);
    if (!group) return;
    dispatch(ctx->encoder, sHorizontal, group);
    sReady = true;
}

WGPURenderPipeline pipeline_for(WGPUDevice device, const GfxRenderTargetLayout& layout) {
    for (const auto& p : sPipelines) if (p.key == layout.key) return p.handle;
    WGPUBlendState blend = WGPU_BLEND_STATE_INIT;
    blend.color.srcFactor = blend.color.dstFactor = WGPUBlendFactor_One;
    blend.alpha.srcFactor = WGPUBlendFactor_Zero;
    blend.alpha.dstFactor = WGPUBlendFactor_One;
    WGPUColorTargetState targets[GFX_MAX_COLOR_ATTACHMENTS];
    WGPUFragmentState fragment = WGPU_FRAGMENT_STATE_INIT;
    fragment.module = sShader; fragment.entryPoint = label("fs");
    fragment.targetCount = gfx_init_color_target_states(&layout, targets, &blend,
        WGPUColorWriteMask_Red | WGPUColorWriteMask_Green | WGPUColorWriteMask_Blue);
    fragment.targets = targets;
    WGPUDepthStencilState depth = WGPU_DEPTH_STENCIL_STATE_INIT;
    depth.format = layout.depth_stencil_format;
    depth.depthWriteEnabled = WGPUOptionalBool_False;
    depth.depthCompare = WGPUCompareFunction_Always;
    WGPURenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_INIT;
    desc.label = label("Dark Link eye surface bloom");
    desc.vertex.module = sShader; desc.vertex.entryPoint = label("vs");
    desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
    desc.depthStencil = depth.format == WGPUTextureFormat_Undefined ? nullptr : &depth;
    desc.multisample.count = layout.sample_count;
    desc.fragment = &fragment;
    auto pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);
    if (pipeline) sPipelines.push_back({layout.key, pipeline});
    return pipeline;
}
void draw(ModContext*, const GfxDrawContext* ctx, const void* data, size_t size, void*) {
    std::lock_guard lock(sGpuMutex);
    if (!sReady) return;
    sReady = false;
    if (size != sizeof(GfxRange)) return;
    GfxRange parameters; std::memcpy(&parameters, data, sizeof(parameters));
    auto pipeline = pipeline_for(ctx->device, ctx->layout);
    if (!pipeline) return;
    WGPUBindGroupEntry entries[3] = {WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT, WGPU_BIND_GROUP_ENTRY_INIT};
    entries[0].binding = 6; entries[0].textureView = sViews[1];
    entries[1].binding = 7; entries[1].sampler = sSampler;
    entries[2].binding = 8; entries[2].buffer = ctx->uniform_buffer;
    entries[2].offset = parameters.offset; entries[2].size = parameters.size;
    auto group = bindings(ctx->device, wgpuRenderPipelineGetBindGroupLayout(pipeline, 0), entries, 3);
    if (!group) return;
    const auto& target = ctx->layout.color_attachments[GFX_SCENE_COLOR_ATTACHMENT_INDEX];
    wgpuRenderPassEncoderSetViewport(ctx->pass, 0, 0, target.width, target.height, 0, 1);
    wgpuRenderPassEncoderSetScissorRect(ctx->pass, 0, 0, target.width, target.height);
    wgpuRenderPassEncoderSetPipeline(ctx->pass, pipeline);
    wgpuRenderPassEncoderSetBindGroup(ctx->pass, 0, group, 0, nullptr);
    wgpuRenderPassEncoderDraw(ctx->pass, 3, 1, 0, 0);
    wgpuBindGroupRelease(group);
}

void boundary(ModContext*, const GfxStageContext* ctx, void*) {
    sMasks = {}; sMaskCount = 0;
    sScene = ctx->stage == GFX_STAGE_SCENE_BEGIN;
}
void composite(ModContext*, const GfxStageContext*, void*) {
    const auto count = sMaskCount;
    const auto masks = sMasks;
    sMasks = {}; sMaskCount = 0; sScene = false;
    if (!count) return;
    GfxResolveDesc desc = {sizeof(GfxResolveDesc), false, true, 0};
    GfxResolvedTargets scene = GFX_RESOLVED_TARGETS_INIT;
    GfxDeviceInfo device = GFX_DEVICE_INFO_INIT;
    if (svc_gfx->resolve_pass(mod_ctx, &desc, &scene) != MOD_OK || !scene.depth ||
        svc_gfx->get_device_info(mod_ctx, &device) != MOD_OK) return;
    // Sample one continuous signal per scene, shared by both eyes. Three gentle
    // frequencies avoid a mechanical pulse or frame-rate-dependent random jumps.
    // Only the blurred spill varies (+/-12%); sharp eye color/alpha stay intact.
    using Clock = std::chrono::steady_clock;
    static const auto origin = Clock::now();
    const double seconds = std::chrono::duration<double>(Clock::now() - origin).count();
    constexpr double tau = 6.283185307179586;
    const std::array<float, 4> intensity = {float(1.0 +
        0.070 * std::sin(tau * 2.3 * seconds) +
        0.035 * std::sin(tau * 5.7 * seconds + 1.2) +
        0.015 * std::sin(tau * 9.1 * seconds + 0.7)), 0, 0, 0};
    GfxRange parameters{};
    if (svc_gfx->push_uniform(mod_ctx, intensity.data(), sizeof(intensity), &parameters) != MOD_OK) return;
    for (unsigned i = 0; i < count; ++i) {
        if (masks[i].width != scene.width || masks[i].height != scene.height) continue;
        const Job job = {masks[i], scene.depth, device.uses_reversed_z};
        if (svc_gfx->push_compute(mod_ctx, sCompute, &job, sizeof(job)) == MOD_OK)
            svc_gfx->push_draw(mod_ctx, sDraw, &parameters, sizeof(parameters));
    }
}
} // namespace

bool begin_eye_bloom_capture() {
    if (!sScene || !sDraw || !sCompute || sCapturing || sMaskCount == sMasks.size()) return false;
    auto* mode = mDoMch_render_c::getRenderModeObj();
    GfxRenderTargetLayout layout = GFX_RENDER_TARGET_LAYOUT_INIT;
    if (!mode || !mode->fbWidth || !mode->efbHeight ||
        svc_gfx->get_scene_target_layout(mod_ctx, &layout) != MOD_OK) return false;
    const auto& target = layout.color_attachments[GFX_SCENE_COLOR_ATTACHMENT_INDEX];
    if (!target.width || !target.height) return false;
    GXGetViewportv(sViewport.data());
    if (!(sViewport[2] > 0 && sViewport[3] > 0)) return false;
    GXGetScissor(&sScissor[0], &sScissor[1], &sScissor[2], &sScissor[3]);
    if (svc_gfx->create_pass(mod_ctx, target.width, target.height) != MOD_OK) return false;
    sCapturing = true;
    const float sx = float(target.width) / mode->fbWidth;
    const float sy = float(target.height) / mode->efbHeight;
    // create_pass resets its viewport. Retain the scene's logical crop/letterbox
    // and depth range; leave projection and the live skinning matrices untouched.
    GXSetViewportRender(sViewport[0]*sx, sViewport[1]*sy, sViewport[2]*sx, sViewport[3]*sy,
                        sViewport[4], sViewport[5]);
    // Match Aurora's outward rounding and clamp to the real attachment.
    const auto left = u32(std::clamp(std::floor(sScissor[0]*sx), 0.0f, float(target.width)));
    const auto top = u32(std::clamp(std::floor(sScissor[1]*sy), 0.0f, float(target.height)));
    const auto right = u32(std::clamp(std::ceil((float(sScissor[0])+sScissor[2])*sx), float(left), float(target.width)));
    const auto bottom = u32(std::clamp(std::ceil((float(sScissor[1])+sScissor[3])*sy), float(top), float(target.height)));
    GXSetScissorRender(left, top, right-left, bottom-top);
    return true;
}
void end_eye_bloom_capture() {
    if (!sCapturing) return;
    GfxResolveDesc desc = {sizeof(GfxResolveDesc), true, true, 0};
    GfxResolvedTargets mask = GFX_RESOLVED_TARGETS_INIT;
    const auto result = svc_gfx->resolve_pass(mod_ctx, &desc, &mask);
    // Closing the offscreen pass resumes the scene without clearing it.
    sCapturing = false;
    GXSetViewport(sViewport[0], sViewport[1], sViewport[2], sViewport[3], sViewport[4], sViewport[5]);
    GXSetScissor(sScissor[0], sScissor[1], sScissor[2], sScissor[3]);
    if (result == MOD_OK && mask.color && mask.depth && sMaskCount < sMasks.size())
        sMasks[sMaskCount++] = {mask.color, mask.depth, mask.width, mask.height};
}

void initialize_eye_bloom() {
    if (!svc_gfx || sDraw) return;
    const GfxDrawTypeDesc drawDesc = {sizeof(GfxDrawTypeDesc), "Dark Link eye bloom", draw, nullptr};
    const GfxComputeTypeDesc computeDesc = {sizeof(GfxComputeTypeDesc), "Dark Link eye mask blur", compute, nullptr};
    const GfxStageHookDesc boundaryDesc = {sizeof(GfxStageHookDesc), boundary, nullptr};
    const GfxStageHookDesc compositeDesc = {sizeof(GfxStageHookDesc), composite, nullptr};
    if (svc_gfx->register_draw_type(mod_ctx, &drawDesc, &sDraw) != MOD_OK ||
        svc_gfx->register_compute_type(mod_ctx, &computeDesc, &sCompute) != MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx, GFX_STAGE_SCENE_BEGIN, &boundaryDesc, &sBegin) != MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx, GFX_STAGE_SCENE_AFTER_OPAQUE, &compositeDesc, &sComposite) != MOD_OK ||
        svc_gfx->register_stage_hook(mod_ctx, GFX_STAGE_FRAME_BEFORE_HUD, &boundaryDesc, &sEnd) != MOD_OK) {
        shutdown_eye_bloom();
        svc_log->warn(mod_ctx, "Dark Link eye bloom unavailable; retaining self-lit eye surfaces");
    }
}
void shutdown_eye_bloom() {
    sMasks = {}; sMaskCount = 0; sScene = sCapturing = false;
    if (sBegin) svc_gfx->unregister_stage_hook(mod_ctx, sBegin);
    if (sComposite) svc_gfx->unregister_stage_hook(mod_ctx, sComposite);
    if (sEnd) svc_gfx->unregister_stage_hook(mod_ctx, sEnd);
    if (sDraw) svc_gfx->unregister_draw_type(mod_ctx, sDraw);
    if (sCompute) svc_gfx->unregister_compute_type(mod_ctx, sCompute);
    sBegin = sComposite = sEnd = sDraw = sCompute = 0;
    std::lock_guard lock(sGpuMutex);
    sReady = false;
    for (const auto& p : sPipelines) wgpuRenderPipelineRelease(p.handle);
    sPipelines.clear();
    for (unsigned i = 0; i < 2; ++i) {
        if (sExtract[i]) wgpuComputePipelineRelease(sExtract[i]);
        if (sViews[i]) wgpuTextureViewRelease(sViews[i]);
        if (sTextures[i]) wgpuTextureRelease(sTextures[i]);
        sExtract[i] = nullptr; sViews[i] = nullptr; sTextures[i] = nullptr;
    }
    if (sHorizontal) wgpuComputePipelineRelease(sHorizontal);
    if (sShader) wgpuShaderModuleRelease(sShader);
    if (sSampler) wgpuSamplerRelease(sSampler);
    sHorizontal = nullptr; sShader = nullptr; sSampler = nullptr;
    sWidth = sHeight = 0;
}
} // namespace dawnlight

"""Run production aura routing with native-pose and graphics-service stand-ins.

Checks the Collection camera, separate histories, palette and Aura/eye-bloom
independence. Mesh geometry and GPU output have their own regression tests.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/fierce_deity_veil.cpp').read_text()
state = source[source.index('using Clock='):source.index('// GPU objects')]
callbacks = source[source.index('bool drawable('):source.index('\n}\nvoid tick_dark_veil')]
lifecycle = source[source.index('void tick_dark_veil('):source.index('void initialize_dark_veil(')]

fixture = r'''
#include "fierce_deity_veil_mesh.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <limits>
using namespace dawnlight::dark_veil;
using Mtx=float[3][4]; using Mtx44=float[4][4];
using fpc_ProcID=unsigned;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
using GfxDrawTypeHandle=unsigned; using GfxStageHookHandle=unsigned;
struct GfxRange { size_t offset=0,size=0; };
struct ModContext {}; ModContext* mod_ctx=nullptr;
struct GfxStageContext { const void* game_view=nullptr; };
struct GfxDeviceInfo { bool uses_reversed_z=false; };
#define GFX_DEVICE_INFO_INIT {}
constexpr int MOD_OK=0, HOOK_CONTINUE=0; using HookAction=int;
struct view_class { Mtx viewMtx{}; Mtx44 projMtx{}; struct {Vec3 eye;} lookat; };
enum class FierceDeityTint {None,Dark,White,Gold};
FierceDeityTint tint=FierceDeityTint::Dark;
bool active=true,enabled=true,aura=true,reload=false,reversed=false;
bool fierce_deity_active(){return active;}
bool fierce_deity_enabled(){return enabled;}
bool fierce_deity_model_reload_active(){return reload;}
bool dark_link_aura_enabled(){return aura;}
FierceDeityTint fierce_deity_displayed_tint(){return tint;}
struct ModelData {unsigned joints=25; unsigned getJointNum(){return joints;}};
struct Model {
    ModelData data; Mtx pose[25]{};
    ModelData* getModelData(){return &data;}
    float (*getAnmMtx(unsigned j))[4]{return pose[j];}
};
struct daAlink_c {
    Model* mpLinkModel=nullptr; unsigned id=1; int mClothesChangeWaitTimer=0;
    bool wolf=false,dead=false,status=false,hidden=false,event=false,scene=false;
    bool checkWolf(){return wolf;} bool checkDeadHP(){return dead;}
    bool checkStatusWindowDraw(){return status;} bool checkPlayerNoDraw(){return hidden;}
    bool checkEventRun(){return event;} bool checkSceneChangeAreaStart(){return scene;}
};
daAlink_c* player=nullptr;
daAlink_c* daAlink_getAlinkActorClass(){return player;}
unsigned fopAcM_GetID(daAlink_c* link){return link->id;}
struct J3DSys {Mtx view{}; float (*getViewMtx())[4]{return view;}} j3dSys;
std::array<float,7> projection{0,2,.2f,3,.3f,-1,-2};
void GXGetProjectionv(float* p){std::copy(projection.begin(),projection.end(),p);}
bool invertible=true;
bool MTXInverse(const Mtx in,Mtx out){
    if(!invertible)return false;
    // Test camera is rigid: transpose its rotation and invert translation.
    for(int r=0;r<3;++r){for(int c=0;c<3;++c)out[r][c]=in[c][r];
        out[r][3]=0;for(int c=0;c<3;++c)out[r][3]-=out[r][c]*in[c][3];}
    return true;
}
int bloomBegins=0,bloomEnds=0,draws=0,uploads=0,presentationCalls=0;
void begin_collection_eye_bloom(){++bloomBegins;}
void end_collection_eye_bloom(){++bloomEnds;}
struct Gfx {
    int get_device_info(ModContext*,GfxDeviceInfo* d){d->uses_reversed_z=reversed;return MOD_OK;}
    int push_verts(ModContext*,const void*,size_t size,size_t,GfxRange* range){
        assert(size>0);*range={0,size};++uploads;return MOD_OK;}
    int push_draw(ModContext*,unsigned,const void*,size_t){++draws;return MOD_OK;}
} gfx;
Gfx* svc_gfx=&gfx;
bool presented(const void* native,Mtx out){
    ++presentationCalls;std::memcpy(out,native,sizeof(Mtx));out[0][3]+=100;return true;
}
void stop_dark_veil();
'''
checks = r'''
void advance(Animation& a){
    a.owner=player->id;a.last=Clock::now()-std::chrono::milliseconds(16);
    a.origin=Clock::now()-std::chrono::seconds(1);
}
void menu_frame(){
    before_collection(nullptr,nullptr,nullptr,nullptr);
    advance(sCollection);after_collection(nullptr,nullptr,nullptr,nullptr);
}
int main(){
    Model model;daAlink_c link;link.mpLinkModel=&model;player=&link;
    for(auto& pose:model.pose){for(int i=0;i<3;++i)pose[i][i]=1;pose[2][3]=-100;}
    view_class view;for(int i=0;i<3;++i)view.viewMtx[i][i]=1;
    view.projMtx[0][0]=2;view.projMtx[1][1]=3;view.projMtx[2][2]=-1;
    view.projMtx[2][3]=-2;view.projMtx[3][2]=-1;view.lookat.eye={0,0,0};
    GfxStageContext ctx{&view};sPresented=presented;
    for(auto palette:{FierceDeityTint::Dark,FierceDeityTint::White,FierceDeityTint::Gold,FierceDeityTint::None}){
        tint=palette;tick_dark_veil(&link,true);advance(sWorld);stage(nullptr,&ctx,nullptr);
        assert(sHasPending && presentationCalls>0 && !sVertices.empty());
        const auto& v=sVertices.front();
        assert(v.color[0]==(palette==FierceDeityTint::White||palette==FierceDeityTint::Gold?1:0));
        assert(v.color[1]==(palette==FierceDeityTint::White?1:palette==FierceDeityTint::Gold?.68f:0));
        assert(v.color[2]==(palette==FierceDeityTint::White?1:palette==FierceDeityTint::Gold?.09f:0));
        after_translucent(nullptr,nullptr,nullptr,nullptr);assert(!sHasPending);
    }
    aura=false;tick_dark_veil(&link,true);stage(nullptr,&ctx,nullptr);assert(!sHasPending);
    aura=true;link.status=true;link.hidden=true;link.event=true;
    for(int i=0;i<3;++i)j3dSys.view[i][i]=1;j3dSys.view[2][3]=-1000;
    const int oldCalls=presentationCalls;const int oldDraws=draws;
    sWorld.fade=.37f;menu_frame();
    assert(draws==oldDraws+1 && bloomBegins==1 && bloomEnds==1);
    assert(presentationCalls==oldCalls && sWorld.fade==.37f);
    assert(sCollectionEye.z==1000 && sCollectionProjection[3][2]==-1);
    // Projection uses the native Collection camera, never the world camera or
    // presentation replacements (+100 X in this fixture).
    const auto v=sVertices.front();const auto w=sMesh.front().world;
    assert(std::abs(v.clip[0]-(2*w.x+.2f*(w.z-1000)))<.001f);
    assert(std::abs(v.clip[1]-(3*w.y+.3f*(w.z-1000)))<.001f);
    assert(std::abs(v.clip[3]+w.z-1000)<.001f);
    assert(std::abs(v.clip[2]-(1998-2*w.z))<.001f);
    reversed=true;menu_frame();assert(std::abs(sVertices.front().clip[2]-(sMesh.front().world.z-998))<.001f);
    reversed=false;
    projection={1,2,4,3,5,-.01f,-.1f};menu_frame();
    assert(sCollectionProjection[3][3]==1 && sCollectionProjection[3][2]==0);
    assert(sCollectionProjection[0][3]==4 && sCollectionProjection[1][3]==5);
    // The body switch must not close the Collection eye-capture window.
    aura=false;const int before=draws;menu_frame();
    assert(draws==before && bloomBegins==4 && bloomEnds==4 && sCollection.fade==0);
    aura=true;
    // No Collection effects for inactive, loading, wolf/dead, invalid or absent Link.
    for(int gate=0;gate<9;++gate){
        if(gate==0)active=false;if(gate==1)enabled=false;if(gate==2)reload=true;
        if(gate==3)link.status=false;if(gate==4)link.wolf=true;if(gate==5)link.dead=true;
        if(gate==6)link.mClothesChangeWaitTimer=1;if(gate==7)model.data.joints=24;if(gate==8)player=nullptr;
        before_collection(nullptr,nullptr,nullptr,nullptr);after_collection(nullptr,nullptr,nullptr,nullptr);
        assert(bloomBegins==4 && bloomEnds==4 && draws==before);
        active=enabled=true;reload=false;link.status=true;link.wolf=link.dead=false;
        link.mClothesChangeWaitTimer=0;model.data.joints=25;player=&link;
    }
    invertible=false;before_collection(nullptr,nullptr,nullptr,nullptr);
    after_collection(nullptr,nullptr,nullptr,nullptr);assert(bloomBegins==4);
    invertible=true;model.pose[0][0][0]=std::numeric_limits<float>::quiet_NaN();
    menu_frame();assert(draws==before && sCollection.owner==fpcM_ERROR_PROCESS_ID_e);
    stop_dark_veil();assert(sWorld.fade==0 && sCollection.fade==0 && !sHasPending);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'aura.cpp', Path(tmp)/'aura'
    cpp.write_text(fixture + state + callbacks + lifecycle + checks)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror',
                    '-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Aura routing: palettes, Collection camera/pose, isolated histories, toggle-independent eye capture and lifecycle gates passed')

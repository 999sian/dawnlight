"""Run the production flame controller with instrumented native particle/model stubs."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <array>
#include <cassert>
#include <cstddef>
using u8=unsigned char; using u16=unsigned short; using u32=unsigned;
using s16=short;
namespace JGeometry { template<class T> struct TVec3 { T x,y,z; TVec3(T x,T y,T z):x(x),y(y),z(z){} }; }
using fpc_ProcID=unsigned; constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
constexpr u16 ID_ZF_J_FIRE02_FIRE=0x3af;
using Mtx=float[3][4]; using MtxP=float(*)[4];
struct cXyz { float x,y,z; cXyz(float x=0,float y=0,float z=0):x(x),y(y),z(z){} };
struct csXyz { short x,y,z; csXyz(short x,short y,short z):x(x),y(y),z(z){} };
struct ModelData { unsigned joints=25; unsigned getJointNum(){return joints;} };
struct Model {
    ModelData data; Mtx pose[25]{};
    ModelData* getModelData(){return &data;}
    MtxP getAnmMtx(unsigned i){assert(i<data.joints);return pose[i];}
};
struct JPABaseEmitter {
    bool stopped=false,hidden=false,cleared=false,continuous=false;
    u32 id=0; unsigned updates=0; cXyz pos,dir,scale;
    float rate=0,speed=0,random=0,psx=0,psy=0; int life=0,alpha=0;
    void stopCreateParticle(){stopped=true;}
    void stopDrawParticle(){hidden=true;}
    void deleteAllParticle(){cleared=true;}
    void becomeContinuousParticle(){continuous=true;}
    void setGlobalPrmColor(int r,int g,int b){assert(r==0&&g==0&&b==0);}
    void setGlobalEnvColor(int r,int g,int b){assert(r==0&&g==0&&b==0);}
    void setGlobalAlpha(int a){alpha=a;}
    void setLocalTranslation(const cXyz& p){assert(p.x==0&&p.y==0&&p.z==0);}
    void setLocalRotation(const JGeometry::TVec3<s16>& p){assert(p.x==0&&p.y==0&&p.z==0);}
    void setLocalScale(const cXyz& s){scale=s;}
    void setDirection(const cXyz& d){dir=d;}
    void setDirectionalSpeed(float s){speed=s;}
    void setAwayFromCenterSpeed(float s){assert(s==0);}
    void setAwayFromAxisSpeed(float s){assert(s==0);}
    void setRandomDirectionSpeed(float s){random=s;}
    void setSpread(float s){assert(s>0&&s<0.2f);}
    void setVolumeSize(unsigned s){assert(s<=15);}
    void setRate(float r){rate=r;}
    void setRateStep(unsigned s){assert(s==0);}
    void setLifeTime(int n){life=n;}
    void setGlobalParticleScale(float x,float y){psx=x;psy=y;}
};
std::array<JPABaseEmitter,256> emitters{};
unsigned allocations=0,lookups=0; bool spawnFails=false,controllerAlive=true;
struct Resources { bool available=true; void* getResource(u16 id){assert(id==0x3af);return available?this:nullptr;} } resources;
struct JPAEmitterManager { Resources* res=&resources; Resources* getResourceManager(u8 i){assert(i==0);return res;} } manager1,manager2;
JPAEmitterManager* currentManager=&manager1;
struct dPa_control_c { static JPAEmitterManager* getEmitterManager(){return currentManager;} };
struct { struct { void* getParticle(){return controllerAlive?this:nullptr;} } play; } g_dComIfG_gameInfo;
JPABaseEmitter* dComIfGp_particle_getEmitter(u32 key) {
    assert(controllerAlive&&currentManager);++lookups;
    for(auto& e:emitters) if(e.id==key) return &e;
    return nullptr;
}
struct daAlink_c {
    unsigned id=1; Model* mpLinkModel=nullptr;
    bool hidden=false,wolf=false,dead=false,event=false,scene=false;
    int mClothesChangeWaitTimer=0;
    bool checkPlayerNoDraw(){return hidden;} bool checkWolf(){return wolf;}
    bool checkDeadHP(){return dead;} bool checkEventRun(){return event;}
    bool checkSceneChangeAreaStart(){return scene;}
    JPABaseEmitter* setEmitter(u32* key,u16 effect,const cXyz* pos,const csXyz* rot) {
        assert(effect==0x3af&&rot->x==0&&rot->y==0&&rot->z==0);
        if(spawnFails){*key=0;return nullptr;}
        auto* e=*key?dComIfGp_particle_getEmitter(*key):nullptr;
        if(!e){assert(allocations<emitters.size());e=&emitters[allocations++];*key=e->id=100+allocations;}
        e->pos=*pos;++e->updates;return e;
    }
};
unsigned fopAcM_GetID(daAlink_c* p){return p->id;}
'''
draw_fixture = r'''
struct ModContext {};
namespace mods { template<class T> T arg(void* args,int i){return static_cast<T>(static_cast<void**>(args)[i]);} }
constexpr int GX_BM_BLEND=1,GX_BL_ZERO=0,GX_BL_INVSRCALPHA=2,GX_LO_COPY=3,GX_FALSE=0;
std::array<int,4> blend{};std::array<int,3> depth{};unsigned gxWrites=0;
void GXSetBlendMode(int a,int b,int c,int d){blend={a,b,c,d};++gxWrites;}
void GXSetZMode(int a,int b,int c){depth={a,b,c};++gxWrites;}
struct JPABaseShape {
    int getBlendMode(){return 1;}int getBlendSrc(){return 4;}
    int getBlendDst(){return 5;}int getLogicOp(){return 6;}
    int getZEnable(){return 1;}int getZCmp(){return 7;}int getZUpd(){return 1;}
    void nativeSetup(){GXSetBlendMode(1,4,5,6);GXSetZMode(1,7,1);}
};
struct JPAResource {JPABaseShape shape;JPABaseShape* getBsp(){return &shape;}};
struct JPAEmitterWorkData {JPABaseEmitter* mpEmtr=nullptr;};
'''
draw_checks = r'''
void check_flame_drawing(JPABaseEmitter* own) {
    JPAResource resource;JPAEmitterWorkData work{own};JPABaseEmitter foreign;
    void* setupArgs[]={&resource.shape,&work};void* drawArgs[]={&resource,&work,own};
    resource.shape.nativeSetup();auto originalBlend=blend;auto originalDepth=depth;
    after_flame_shape_setup(nullptr,setupArgs,nullptr,nullptr);
    assert((blend==std::array<int,4>{GX_BM_BLEND,GX_BL_ZERO,GX_BL_INVSRCALPHA,GX_LO_COPY}));
    assert((depth==std::array<int,3>{1,7,GX_FALSE}));
    // Batched drawing replays native shape setup; reapply our blend each time.
    resource.shape.nativeSetup();after_flame_shape_setup(nullptr,setupArgs,nullptr,nullptr);
    assert(blend[1]==GX_BL_ZERO&&blend[2]==GX_BL_INVSRCALPHA);
    after_flame_resource_draw(nullptr,drawArgs,nullptr,nullptr);
    assert(blend==originalBlend&&depth==originalDepth);
    // A torch using exactly the same JPA resource must remain entirely native.
    work.mpEmtr=&foreign;drawArgs[2]=&foreign;auto writes=gxWrites;
    after_flame_shape_setup(nullptr,setupArgs,nullptr,nullptr);
    after_flame_resource_draw(nullptr,drawArgs,nullptr,nullptr);
    assert(gxWrites==writes&&blend==originalBlend&&depth==originalDepth);
    setupArgs[1]=nullptr;after_flame_shape_setup(nullptr,setupArgs,nullptr,nullptr);
    assert(gxWrites==writes);
}
'''
checks = r'''
int main() {
    Model model; daAlink_c link; link.mpLinkModel=&model;
    constexpr std::array<unsigned,11> expectedJoints{1,2,3,7,8,12,13,18,19,23,24};
    constexpr auto count=expectedJoints.size();
    for(unsigned j=0;j<25;++j){model.pose[j][0][3]=10*j;model.pose[j][1][3]=30*j;}
    tick_dark_flame(&link,false);assert(allocations==0);
    tick_dark_flame(&link,true);assert(allocations==count);
    check_flame_drawing(&emitters[0]);
    const auto keys=s_darkFlame.keys;
    for(unsigned i=0;i<count;++i){
        auto& e=emitters[i];assert(e.pos.x==10*expectedJoints[i]&&e.pos.y==30*expectedJoints[i]);
        assert(e.continuous&&e.dir.x==0&&e.dir.y==1&&e.dir.z==0);
        assert(e.speed>e.random&&e.rate*e.life*count<=60.0f&&e.life<=45&&e.alpha<200);
    }
    for(int tick=0;tick<100;++tick) tick_dark_flame(&link,true);
    assert(allocations==count&&s_darkFlame.keys==keys); // bounded, not a new burst each frame
    model.pose[2][0][3]=900;tick_dark_flame(&link,true);assert(emitters[1].pos.x==900);
    // Limbs move independently of the torso: flame must follow each bone.
    model.pose[8][0][3]=-123; model.pose[24][2][3]=456;
    tick_dark_flame(&link,true);
    assert(emitters[4].pos.x==-123&&emitters[10].pos.z==456);
    assert(emitters[1].pos.x==900);
    Model replacement;replacement.pose[2][1][3]=777;
    replacement.pose[13][1][3]=333;replacement.pose[19][0][3]=-444;
    link.mpLinkModel=&replacement;tick_dark_flame(&link,true);
    assert(emitters[1].pos.y==777&&emitters[6].pos.y==333&&emitters[8].pos.x==-444); // read new outfit, no retained model matrices
    tick_dark_flame(&link,false);
    for(unsigned i=0;i<count;++i)assert(emitters[i].stopped&&emitters[i].hidden&&emitters[i].cleared);
    assert(s_darkFlame.owner==nullptr);
    stop_dark_flame(); // idempotent
    // No effects on hidden Link, wolf, death, cutscenes, warps, or clothes loads.
    for(auto flag:{&link.hidden,&link.wolf,&link.dead,&link.event,&link.scene}){
        tick_dark_flame(&link,true);auto old=allocations;
        *flag=true;tick_dark_flame(&link,true);assert(s_darkFlame.owner==nullptr&&allocations==old);
        *flag=false;
    }
    link.mClothesChangeWaitTimer=1;tick_dark_flame(&link,true);assert(!s_darkFlame.owner);
    link.mClothesChangeWaitTimer=0;replacement.data.joints=24;
    tick_dark_flame(&link,true);assert(!s_darkFlame.owner);replacement.data.joints=25;
    link.mpLinkModel=nullptr;tick_dark_flame(&link,true);assert(!s_darkFlame.owner);
    link.mpLinkModel=&replacement;
    tick_dark_flame(&link,true);auto old=allocations;++link.id;
    tick_dark_flame(&link,true);assert(allocations==old+count); // actor address reuse
    for(unsigned i=old-count;i<old;++i)assert(emitters[i].cleared);
    // A replaced manager must never receive old handles, even if IDs repeat.
    auto calls=lookups;currentManager=&manager2;stop_dark_flame();assert(lookups==calls);
    tick_dark_flame(&link,true);controllerAlive=false;calls=lookups;
    stop_dark_flame();assert(lookups==calls);controllerAlive=true;
    currentManager=nullptr;tick_dark_flame(&link,true);assert(!s_darkFlame.owner);
    currentManager=&manager1;manager1.res=nullptr;tick_dark_flame(&link,true);assert(!s_darkFlame.owner);
    manager1.res=&resources;resources.available=false;tick_dark_flame(&link,true);assert(!s_darkFlame.owner);
    resources.available=true;spawnFails=true;old=allocations;
    tick_dark_flame(&link,true);assert((allocations==old&&s_darkFlame.keys==std::array<u32,count>{}));
    spawnFails=false;tick_dark_flame(&link,true);assert(allocations==old+count);
    // Expired generation keys are harmless on cleanup.
    for(auto& e:emitters)e.id=0;
    stop_dark_flame();tick_dark_flame(nullptr,true);assert(!s_darkFlame.owner);
}
'''
with tempfile.TemporaryDirectory(prefix='dawnlight-dark-flame-') as temp:
    cpp, exe = Path(temp)/'test.cpp', Path(temp)/'test'
    cpp.write_text(fixture + (root/'src/fierce_deity_flame.inc').read_text() + draw_fixture +
                   (root/'src/fierce_deity_flame_draw.inc').read_text() + draw_checks + checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Dark Link flame: pose attachment, bounded emitters, black blending, draw-state isolation, teardown and missing resources passed')

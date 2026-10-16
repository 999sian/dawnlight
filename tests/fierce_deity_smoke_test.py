"""Run the production smoke controller with instrumented native particle/model stubs."""
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
constexpr u16 ID_ZI_J_ARWB_SMOKE_A=0x1df;
using Mtx=float[3][4]; using MtxP=float(*)[4];
struct cXyz { float x,y,z; cXyz(float x=0,float y=0,float z=0):x(x),y(y),z(z){} };
struct csXyz { short x,y,z; csXyz(short x,short y,short z):x(x),y(y),z(z){} };
struct ModelData { unsigned joints=4; unsigned getJointNum(){return joints;} };
struct Model {
    ModelData data; Mtx pose[4]{};
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
std::array<JPABaseEmitter,100> emitters{};
unsigned allocations=0,lookups=0; bool spawnFails=false,controllerAlive=true;
struct Resources { bool available=true; void* getResource(u16 id){assert(id==0x1df);return available?this:nullptr;} } resources;
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
        assert(effect==0x1df&&rot->x==0&&rot->y==0&&rot->z==0);
        if(spawnFails){*key=0;return nullptr;}
        auto* e=*key?dComIfGp_particle_getEmitter(*key):nullptr;
        if(!e){assert(allocations<emitters.size());e=&emitters[allocations++];*key=e->id=100+allocations;}
        e->pos=*pos;++e->updates;return e;
    }
};
unsigned fopAcM_GetID(daAlink_c* p){return p->id;}
'''
checks = r'''
int main() {
    Model model; daAlink_c link; link.mpLinkModel=&model;
    for(int j=1;j<=3;++j){model.pose[j][0][3]=10*j;model.pose[j][1][3]=30*j;}
    tick_dark_smoke(&link,false);assert(allocations==0);
    tick_dark_smoke(&link,true);assert(allocations==3);
    const auto keys=s_darkSmoke.keys;
    for(unsigned i=0;i<3;++i){
        auto& e=emitters[i];assert(e.pos.x==10*(i+1)&&e.pos.y==30*(i+1));
        assert(e.continuous&&e.dir.x==0&&e.dir.y==1&&e.dir.z==0);
        assert(e.speed>e.random&&e.rate<0.5f&&e.life<=45&&e.alpha<200);
    }
    for(int tick=0;tick<100;++tick) tick_dark_smoke(&link,true);
    assert(allocations==3&&s_darkSmoke.keys==keys); // bounded, not a new burst each frame
    model.pose[2][0][3]=900;tick_dark_smoke(&link,true);assert(emitters[1].pos.x==900);
    Model replacement;replacement.pose[2][1][3]=777;
    link.mpLinkModel=&replacement;tick_dark_smoke(&link,true);
    assert(emitters[1].pos.y==777); // read new outfit, no retained model matrices
    tick_dark_smoke(&link,false);
    for(unsigned i=0;i<3;++i)assert(emitters[i].stopped&&emitters[i].hidden&&emitters[i].cleared);
    assert(s_darkSmoke.owner==nullptr);
    stop_dark_smoke(); // idempotent
    // No effects on hidden Link, wolf, death, cutscenes, warps, or clothes loads.
    for(auto flag:{&link.hidden,&link.wolf,&link.dead,&link.event,&link.scene}){
        tick_dark_smoke(&link,true);auto old=allocations;
        *flag=true;tick_dark_smoke(&link,true);assert(s_darkSmoke.owner==nullptr&&allocations==old);
        *flag=false;
    }
    link.mClothesChangeWaitTimer=1;tick_dark_smoke(&link,true);assert(!s_darkSmoke.owner);
    link.mClothesChangeWaitTimer=0;replacement.data.joints=3;
    tick_dark_smoke(&link,true);assert(!s_darkSmoke.owner);replacement.data.joints=4;
    link.mpLinkModel=nullptr;tick_dark_smoke(&link,true);assert(!s_darkSmoke.owner);
    link.mpLinkModel=&replacement;
    tick_dark_smoke(&link,true);auto old=allocations;++link.id;
    tick_dark_smoke(&link,true);assert(allocations==old+3); // actor address reuse
    for(unsigned i=old-3;i<old;++i)assert(emitters[i].cleared);
    // A replaced manager must never receive old handles, even if IDs repeat.
    auto calls=lookups;currentManager=&manager2;stop_dark_smoke();assert(lookups==calls);
    tick_dark_smoke(&link,true);controllerAlive=false;calls=lookups;
    stop_dark_smoke();assert(lookups==calls);controllerAlive=true;
    currentManager=nullptr;tick_dark_smoke(&link,true);assert(!s_darkSmoke.owner);
    currentManager=&manager1;manager1.res=nullptr;tick_dark_smoke(&link,true);assert(!s_darkSmoke.owner);
    manager1.res=&resources;resources.available=false;tick_dark_smoke(&link,true);assert(!s_darkSmoke.owner);
    resources.available=true;spawnFails=true;old=allocations;
    tick_dark_smoke(&link,true);assert((allocations==old&&s_darkSmoke.keys==std::array<u32,3>{}));
    spawnFails=false;tick_dark_smoke(&link,true);assert(allocations==old+3);
    // Expired generation keys are harmless on cleanup.
    for(auto& e:emitters)e.id=0;
    stop_dark_smoke();tick_dark_smoke(nullptr,true);assert(!s_darkSmoke.owner);
}
'''
with tempfile.TemporaryDirectory(prefix='dawnlight-dark-smoke-') as temp:
    cpp, exe = Path(temp)/'test.cpp', Path(temp)/'test'
    cpp.write_text(fixture + (root/'src/fierce_deity_smoke.inc').read_text() + checks)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Dark Link smoke: pose attachment, bounded emitters, appearance, teardown and missing resources passed')

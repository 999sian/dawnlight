"""Production shoulder camera, reticle convergence and ownership/lifecycle checks."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <vector>
#include <source_location>
#include <cstdio>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}float inprod(cXyz b)const{return x*b.x+y*b.y+z*b.z;}
 float abs2()const{return inprod(*this);}float abs2(cXyz b)const{return (*this-b).abs2();}
 float abs()const{return std::sqrt(abs2());}float absXZ()const{return std::hypot(x,z);}
 void normalize(){*this=*this*(1.f/abs());}};
using s16=short;short cM_rad2s(float a){return short(a*32768/M_PI);}
float gyroX=0,gyroY=0;bool gyroActive=false;
void survivors_gyro_deltas(bool active,float& yaw,float& pitch){yaw=active?gyroX:0;pitch=active?gyroY:0;gyroActive=active;}
short cM_atan2s(float a,float b){return short(std::atan2(a,b)*32768/M_PI);}
float cM_ssin(short a){return std::sin(a*M_PI/32768);}float cM_scos(short a){return std::cos(a*M_PI/32768);}
struct fopAc_ac_c{ActorId id=7;int health=10;struct{cXyz pos;}current;};
struct daAlink_c:fopAc_ac_c{int field_0x317c=0;bool held=true,pressed=false,wolf=false;
 bool swordButton(){return held;}bool swordTrigger(){return pressed;}bool checkWolf(){return wolf;}
 struct{short y=0;}shape_angle;struct{int mCutTurnChargeCheckTimer=0;}mProcVar5;
 struct{bool drawn=false,locked=false;cXyz position;void setPos(const cXyz* p){position=*p;}
 void onDrawFlg(){drawn=true;}void offDrawFlg(){drawn=false;}void offLockFlg(){locked=false;}}mSight;
}player;
ActorId s_player=7;Run s_run;bool playing=true,busy=false,nativeAim=false,cameraPresent=true,arena=true;
bool playable(){return playing;}bool in_arena(){return arena;}bool jump_abilities_busy_for(daAlink_c*){return busy;}
ActorId fopAcM_GetID(fopAc_ac_c* p){return p->id;}daAlink_c* daAlink_getAlinkActorClass(){return &player;}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct dBgS_CamLinChk{cXyz hit;cXyz GetCross(){return hit;}};
struct dCamera_c{cXyz mCenter{0,130,0},mEye{0,130,-300};float mFovy=45;int mPadID=0;bool blocked=false;
 struct{cXyz value;void Val(cXyz v){value=v;}}mDirection;
 cXyz Up(){return{0,1,0};}static bool isAimActive(){return nativeAim;}
 bool lineBGCheck(cXyz* start,cXyz* end,dBgS_CamLinChk* hit,int){hit->hit=*start+(*end-*start)*.5f;return blocked;}
};
struct CameraActor{dCamera_c mCamera;}cameraActor;
CameraActor* dComIfGp_getCamera(int){return cameraPresent?&cameraActor:nullptr;}
cXyz aim_vector_to_world(daAlink_c*,cXyz v){return v;}
float wallDistance=10000;
struct dBgS_LinChk{cXyz start,end;void Set(const cXyz* a,const cXyz* b,daAlink_c*){start=*a;end=*b;}
 cXyz GetCross(){cXyz d=end-start;d.normalize();return start+d*wallDistance;}};
struct World{bool LineCross(dBgS_LinChk*){return wallDistance<10000;}}world;
World& dComIfG_Bgsp(){return world;}
struct Enemy{bool initialized=true,retiring=false,credited=false;}enemy;
Enemy* tracked(ActorId id){return id==12?&enemy:nullptr;}
struct cCcD_CpsAttr{cXyz start,end;float radius=0;void Set(cXyz a,cXyz b,float r){start=a;end=b;radius=r;}};
struct Shape{cXyz center;float radius=30;
 bool CrossAtTg(const cCcD_CpsAttr& ray,cXyz*){
  const auto d=ray.end-ray.start;const float len=d.abs2();
  const float t=len>0?std::clamp((center-ray.start).inprod(d)/len,0.f,1.f):0;
  return (ray.start+d*t-center).abs()<=radius+ray.radius;
 }};
struct Target{fopAc_ac_c actor;Shape shape;fopAc_ac_c* GetAc(){return &actor;}Shape* GetShapeAttr(){return &shape;}}targetObject;
struct Server{unsigned mObjTgCount=0;std::array<Target*,1>mpObjTg{};}server;
Server* dComIfG_Ccsp(){return &server;}
struct Attack{bool active=true,aimPending=true;unsigned weapon=0;float aimSpread=0;cXyz position{0,65,0},velocity;};
std::vector<Attack>s_attacks;
'''
main=r'''
void near(cXyz a,cXyz b,std::source_location site=std::source_location::current()){if((a-b).abs()>=.25f)std::fprintf(stderr,"line %u: %f %f %f vs %f %f %f\n",site.line(),a.x,a.y,a.z,b.x,b.y,b.z);assert((a-b).abs()<.25f);}
void reset(){gyroX=gyroY=0;gyroActive=false;s_swordAim={};player={};cameraActor={};s_run.start(9);s_player=7;playing=arena=cameraPresent=true;
 busy=nativeAim=false;server={};s_attacks.clear();wallDistance=10000;enemy={};}
int main(){
 reset();auto& camera=cameraActor.mCamera;const auto baseEye=camera.mEye,baseCenter=camera.mCenter;
 for(int i=0;i<100;++i){restore_sword_aim_camera(&camera);near(camera.mEye,baseEye);near(camera.mCenter,baseCenter);
  assert(update_sword_aim_camera(&camera));near(camera.mCenter,{60,170,0});near(camera.mEye,{60,170,-300});assert(player.mSight.drawn);}
 // Gyro adds to native orbit, accumulates while held, and never rotates Link.
 reset();gyroX=.05f;gyroY=.02f;const auto shape=player.shape_angle.y;
 assert(update_sword_aim_camera(&camera)&&gyroActive);const auto first=camera.mCenter-camera.mEye;
 restore_sword_aim_camera(&camera);assert(update_sword_aim_camera(&camera));
 const auto second=camera.mCenter-camera.mEye;
 assert(second.x>first.x&&second.y<first.y&&player.shape_angle.y==shape);
 for(int i=0;i<100;++i){restore_sword_aim_camera(&camera);gyroY=.1f;assert(update_sword_aim_camera(&camera));}
 assert(s_swordAim.gyroPitch==14500);
 player.held=false;restore_sword_aim_camera(&camera);assert(!update_sword_aim_camera(&camera)&&!gyroActive&&!s_swordAim.gyroYaw);
 reset();s_run.options.gyro_aim=false;gyroX=gyroY=.2f;
 assert(update_sword_aim_camera(&camera)&&!gyroActive);near(camera.mEye,{60,170,-300});
 reset();assert(update_sword_aim_camera(&camera));
 // Reticle's vertical coordinate and projectile convergence share the exact ray.
 for(int y:{-40,0,40})for(float fov:{35.f,45.f,80.f})for(float distance:{400.f,700.f,1300.f}){
  restore_sword_aim_camera(&camera);s_swordAim.active=false;camera.mFovy=fov;s_run.options.reticle_y=y;wallDistance=distance;
  assert(update_sword_aim_camera(&camera));cXyz eye,forward;
  assert(third_person_reticle_ray(&camera,y,fov,eye,forward));near(player.mSight.position,eye+forward*distance);
  const auto axis=camera.mCenter-camera.mEye;const float slope=(forward.y/forward.z-axis.y/axis.z);
  assert(y==0?std::abs(slope)<.001f:y*slope>0);
  for(auto weapon:{Upgrade::Wave,Upgrade::Bow}) {
   s_attacks={Attack{}};s_attacks[0].weapon=unsigned(weapon);s_attacks[0].position={-25,145,30};
   resolve_manual_projectiles(&camera);assert(!s_attacks[0].aimPending);
   const float speed=weapon==Upgrade::Bow?70.f:42.f;
   const auto hit=s_attacks[0].position+s_attacks[0].velocity*((player.mSight.position-s_attacks[0].position).abs()/speed);
   near(hit,player.mSight.position);
   // A launched projectile cannot be redirected by subsequent camera changes.
   const auto velocity=s_attacks[0].velocity;camera.mCenter.x+=200;resolve_manual_projectiles(&camera);near(s_attacks[0].velocity,velocity);
   camera.mCenter.x-=200;
  }
 }
 // Native target geometry beats distant terrain without snapping off the ray.
 reset();assert(update_sword_aim_camera(&camera));cXyz eye,forward;third_person_reticle_ray(&camera,0,45,eye,forward);
 targetObject.actor.id=12;targetObject.shape.center=eye+forward*600;server.mpObjTg[0]=&targetObject;server.mObjTgCount=1;
 cXyz target,ray;assert(sword_aim_target(&player,target,ray));near(target,eye+forward*570);
 wallDistance=400;sword_aim_target(&player,target,ray);near(target,eye+forward*400);
 wallDistance=10000;enemy.retiring=true;sword_aim_target(&player,target,ray);near(target,eye+forward*10000);
 // Button release, pause, scene ownership, native item aim and traversal release the view/sight.
 for(int reason=0;reason<8;++reason){reset();assert(update_sword_aim_camera(&camera));restore_sword_aim_camera(&camera);
  if(reason==0)player.held=false;if(reason==1)playing=false;if(reason==2)s_run.phase=Phase::Upgrade;
  if(reason==3)s_player=99;if(reason==4)nativeAim=true;if(reason==5)busy=true;if(reason==6)player.wolf=true;
  if(reason==7)s_run.phase=Phase::Treasure;
  assert(!update_sword_aim_camera(&camera));assert(!s_swordAim.active&&!player.mSight.drawn);near(camera.mEye,{0,130,-300});}
 reset();assert(update_sword_aim_camera(&camera));clear_sword_aim();near(camera.mEye,{0,130,-300});assert(!player.mSight.drawn);
 // A replacement/event camera must not be overwritten with an old snapshot.
 reset();assert(update_sword_aim_camera(&camera));camera.mEye={100,200,300};clear_sword_aim();near(camera.mEye,{100,200,300});
 reset();camera.blocked=true;assert(update_sword_aim_camera(&camera));assert((camera.mEye-camera.mCenter).abs()<160);
 reset();assert(update_sword_aim_camera(&camera));dCamera_c foreign;foreign.mPadID=1;
 assert(!update_sword_aim_camera(&foreign)&&s_swordAim.active&&player.mSight.drawn);
 restore_sword_aim_camera(&foreign);assert(s_swordAim.camera==&camera);
 restore_sword_aim_camera(&camera);wallDistance=100;assert(update_sword_aim_camera(&camera));
 s_attacks={Attack{}};resolve_manual_projectiles(&camera);assert(s_attacks[0].velocity.z>0); // Never shoot backwards into the camera.
 reset();cameraPresent=false;assert(manual_direction(&player).z==1);
 reset();player.held=false;player.pressed=true;assert(manual_attack(&player));assert(!update_sword_aim_camera(&camera));
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/aim_third_person.inc').read_text()+(root/'src/survivors_manual_attack.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors sword aim: shared camera, reticle projection, near/occluded targets, wave convergence and cleanup passed')

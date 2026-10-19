"""Exercise production elite spacing scopes and attack collision dimensions."""
from pathlib import Path
import os, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <any>
#include <cassert>
#include <cmath>
#include <vector>
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
using process_method_func=int(*)(void*);
int execute(void*){return 1;}int draw(void*){return 1;}
struct process_method_class {process_method_func execute_method=execute;}methods;
enum {fpcNm_E_OC_e,fpcNm_E_RD_e,fpcNm_E_RDY_e,fpcNm_E_S1_e,fpcNm_E_SH_e,fpcNm_E_DN_e,fpcNm_E_KR_e,Other};
struct fopAc_ac_c {int id=1,health=10;void* sub_method=&methods;struct {float x=2;}scale;
 struct Vec {float x=0,y=0,z=0;}speed;struct {Vec pos;}current;float speedF=0;
 struct {short y=0;}shape_angle;};
struct dBgS_Acch {fopAc_ac_c* owner=nullptr;bool line=false;
 fopAc_ac_c* getMyAc(){return owner;}void OnLineCheck(){line=true;}};
float cM_ssin(short a){return std::sin(a*3.141592653589793/32768);}
float cM_scos(short a){return std::cos(a*3.141592653589793/32768);}
int fopAcM_GetID(const fopAc_ac_c* a){return a->id;}
void* nonActor=nullptr;bool is_actor_process(void* p){return p && p!=nonActor;}
struct Sphere {fopAc_ac_c* owner=nullptr;float r=1;bool active=false;float center=123;
 fopAc_ac_c* GetAc(){return owner;}void SetR(float v){r=v;}float GetR(){return r;}};
using cCcD_Obj=Sphere;
struct daE_OC_c:fopAc_ac_c {int action=3,mOcState=0;float field_0x6a0=0;dBgS_Acch mAcch;
 int getActionMode(){return action;}};
struct e_rd_class:fopAc_ac_c {int action=3,field_0x9ab=0;Sphere at_sph;};
struct e_rdy_class:fopAc_ac_c {int mAction=3;};
struct e_s1_class:fopAc_ac_c {int mAction=2;};
struct e_sh_class:fopAc_ac_c {int field_0x676=2,field_0x678=0;float field_0x6a4=0;dBgS_Acch mObjAcch;};
struct e_dn_class:fopAc_ac_c {int tail_at_sph_flag=0;Sphere at_sph;};
struct e_kr_class:fopAc_ac_c {Sphere mSphere1,mSphere4;};
struct Player:fopAc_ac_c {bool horse=false,boar=false;
 bool checkHorseRide()const{return horse;}bool checkBoarRide()const{return boar;}}player;
bool hasPlayer=true;Player* daAlink_getAlinkActorClass(){return hasPlayer?&player:nullptr;}
struct Enemy {int id=1,profile=fpcNm_E_OC_e;bool elite=true,initialized=true,retiring=false,credited=false,dormant=false,stagger=false;}enemy;
Enemy* tracked(int id){return id==enemy.id?&enemy:nullptr;}
bool faster_boss(const Enemy& e){return e.elite;}
bool arena=true,transition=false;bool in_arena(){return arena;}bool transitioning(){return transition;}
enum class Phase {Running,Upgrade};struct {Phase phase=Phase::Running;}s_run;
'''
main = r'''
float distance(fopAc_ac_c* a,float world,const fopAc_ac_c* target=&player){
 std::any args[]{static_cast<const fopAc_ac_c*>(a),target};
 after_elite_melee_distance(nullptr,args,&world,nullptr);return world;
}
void sphere(Sphere& s,fopAc_ac_c& a,float expected){
 s.owner=&a;std::any args[]{0,static_cast<cCcD_Obj*>(&s)};
 const auto center=s.center;const auto active=s.active;
 for(int i=0;i<3;++i){before_elite_attack_scale(nullptr,args,nullptr,nullptr);assert(std::abs(s.r-expected)<0.001f);}
 assert(s.center==center && s.active==active);
}
int main(){
 daE_OC_c oc;e_rd_class rd;e_rdy_class rdy;e_s1_class s1;e_sh_class sh;e_dn_class dn;e_kr_class kr;
 fopAc_ac_c* actors[]{&oc,&rd,&rdy,&s1,&sh};
 // The native decision receives model units, while every query outside Execute
 // (including nested actor/draw callbacks) continues to receive world units.
 for(int p=0;p<5;++p){
  enemy={};enemy.profile=p;auto* a=actors[p];
  std::any scope[]{process_method_func(execute),static_cast<void*>(a)};
  assert(distance(a,600)==600);
  before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  assert(distance(a,600)==300 && distance(a,600)==300);
  fopAc_ac_c other;other.id=2;
  assert(distance(a,600,&other)==600 && distance(&other,600)==600);
  std::any nested[]{process_method_func(execute),static_cast<void*>(&other)};
  before_elite_scale_scope(nullptr,nested,nullptr,nullptr);
  assert(distance(a,600)==600);
  after_elite_scale_scope(nullptr,nested,nullptr,nullptr);
  assert(distance(a,600)==300);
  scope[0]=process_method_func(draw);
  before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  assert(distance(a,600)==600);
  after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  assert(distance(a,600)==300);
  after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  assert(s_eliteScaleFrames.empty() && distance(a,600)==600);
 }
 enemy={};std::any scope[]{process_method_func(execute),static_cast<void*>(&oc)};
 before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 assert(distance(&oc,402)>200 && distance(&oc,798)<400); // Scaled attack window
 for(int state:{0,4,5,9}){oc.action=state;assert(distance(&oc,600)==600);}
 for(int state:{3,15,16}){oc.action=state;assert(distance(&oc,600)==300);}
 after_elite_scale_scope(nullptr,scope,nullptr,nullptr);oc.action=3;
 // Approach and recovery thresholds scale, without changing any action state.
 enemy.profile=fpcNm_E_S1_e;scope[1]=static_cast<void*>(&s1);
 before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 assert(distance(&s1,599)<300 && distance(&s1,601)>300);
 s1.mAction=3;assert(distance(&s1,619)<310 && s1.mAction==3);
 s1.mAction=5;assert(distance(&s1,600)==600);
 after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 enemy.profile=fpcNm_E_RDY_e;scope[1]=static_cast<void*>(&rdy);
 before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 assert(distance(&rdy,499)<250 && distance(&rdy,599)<300);
 for(int state:{4,5,6,7}){rdy.mAction=state;assert(distance(&rdy,600)==600);}
 after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 enemy.profile=fpcNm_E_SH_e;scope[1]=static_cast<void*>(&sh);
 before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 const float randomFactor=1.2f;
 assert(randomFactor*distance(&sh,999)<600 && randomFactor*distance(&sh,1001)>600);
 sh.field_0x676=3;assert(randomFactor*distance(&sh,1300)<800 && sh.field_0x676==3);
 sh.field_0x676=10;assert(distance(&sh,600)==600);
 after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 enemy.profile=fpcNm_E_RD_e;scope[1]=static_cast<void*>(&rd);
 before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 for(int state:{4,5,6,7}){rd.action=state;assert(distance(&rd,600)==600);}
 after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 // Each lifecycle exclusion blocks both AI normalization and collider writes.
 for(int guard=0;guard<13;++guard){
  enemy={};oc.health=10;oc.scale.x=2;oc.sub_method=&methods;
  arena=hasPlayer=true;transition=false;s_run.phase=Phase::Running;
  switch(guard){
  case 0:enemy.elite=false;break;case 1:enemy.initialized=false;break;
  case 2:enemy.retiring=true;break;case 3:enemy.credited=true;break;
  case 4:enemy.dormant=true;break;case 5:enemy.stagger=true;break;
  case 6:oc.health=0;break;case 7:oc.scale.x=1;break;
  case 8:arena=false;break;case 9:transition=true;break;
  case 10:s_run.phase=Phase::Upgrade;break;case 11:enemy.id=2;break;
  case 12:hasPlayer=false;break;}
  scope[1]=static_cast<void*>(&oc);before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  assert(distance(&oc,600)==600);after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  if(guard!=12){enemy.profile=fpcNm_E_KR_e;kr.health=oc.health;kr.scale=oc.scale;kr.mSphere1.r=25;sphere(kr.mSphere1,kr,25);}
 }
 enemy={};arena=hasPlayer=true;transition=false;s_run.phase=Phase::Running;
 enemy.profile=fpcNm_E_DN_e;
 for(int mode=0;mode<3;++mode){dn.tail_at_sph_flag=mode;sphere(dn.at_sph,dn,mode==0?110:mode==1?176:154);}
 enemy.profile=fpcNm_E_KR_e;kr.health=10;kr.scale.x=2;
 sphere(kr.mSphere1,kr,50);sphere(kr.mSphere4,kr,160);
 enemy.profile=fpcNm_E_RD_e;rd.at_sph.r=60;
 sphere(rd.at_sph,rd,120);player.horse=true;sphere(rd.at_sph,rd,144);
 rd.field_0x9ab=2;sphere(rd.at_sph,rd,120);
 rd.at_sph.r=-200;sphere(rd.at_sph,rd,-200);
 // Enlarged attack movement reaches the scaled window. Apply only once before
 // native wall correction, retaining knockback, height and stored velocity.
 enemy={};oc.health=10;oc.scale.x=2;oc.mAcch.owner=&oc;
 for(bool reverse:{false,true}){
  oc.action=4;oc.mOcState=reverse?3:1;oc.field_0x6a0=100;oc.current.pos={10,20,30};
  scope[1]=static_cast<void*>(&oc);before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  const float delta=reverse?-15.f:25.f;
  oc.field_0x6a0+=delta;oc.current.pos.z+=delta;oc.action=3; // Last attack frame
  std::any movement[]{&oc.mAcch};
  before_elite_attack_motion(nullptr,movement,nullptr,nullptr);
  before_elite_attack_motion(nullptr,movement,nullptr,nullptr);
  assert(oc.current.pos.z==30+delta*2 && oc.current.pos.x==10 && oc.current.pos.y==20 && oc.mAcch.line);
  after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 }
 enemy.profile=fpcNm_E_SH_e;sh.mObjAcch.owner=&sh;
 for(int exclusion=0;exclusion<4;++exclusion){
  sh.field_0x676=3;sh.field_0x678=3;sh.field_0x6a4=0;sh.speedF=40;sh.speed={0,20,40};sh.current.pos={0,20,60};
  if(exclusion==1)sh.field_0x678=1; // Windup
  if(exclusion==2)sh.speedF=-5; // Shield rebound
  if(exclusion==3)sh.field_0x6a4=45; // Knockback
  scope[1]=static_cast<void*>(&sh);before_elite_scale_scope(nullptr,scope,nullptr,nullptr);
  std::any movement[]{&sh.mObjAcch};
  before_elite_attack_motion(nullptr,movement,nullptr,nullptr);
  before_elite_attack_motion(nullptr,movement,nullptr,nullptr);
  assert(sh.current.pos.z==(exclusion?60:120) && sh.current.pos.y==20 && sh.speed.z==40 && sh.speed.y==20);
  after_elite_scale_scope(nullptr,scope,nullptr,nullptr);
 }
 assert(s_eliteScaleFrames.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; exe=Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_elite_scale.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Elite scale: melee boundaries, native states, nested scopes, lifecycle exclusions and collision radii passed')

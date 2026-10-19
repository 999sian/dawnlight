"""Independent abilities and lossless per-activation hit collection."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <array>
#include <algorithm>
#include <any>
#include <cassert>
#include <cmath>
#include <span>
#include <unordered_map>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_projectiles.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using u8=unsigned char;using u16=unsigned short;using u32=unsigned;
constexpr unsigned kNone=~0u;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator*(float v)const{return{x*v,y*v,z*v};}
 cXyz& operator+=(cXyz b){x+=b.x;y+=b.y;z+=b.z;return *this;}};
struct Actor{unsigned id=1,parentActorID=kNone;int health=100,name=0;struct{cXyz pos;}current;};
using fopAc_ac_c=Actor;
Actor* actor_by_id(unsigned id);
struct Status{void Init(int,int,Actor*){}};
struct Collider {Actor* owner=nullptr;unsigned power=0,hits=0,hitActorId=kNone;Actor* GetAc(){return owner;}
 Collider* GetGObjInf(){return this;}void SetAtHitApid(unsigned id){hitActorId=id;}
 bool ChkAtHit(){return hits && actor_by_id(hitActorId)!=nullptr;}
 void SetAtHit(Collider*){++hits;}unsigned GetAtAtp(){return power;}void SetAtAtp(unsigned n){power=n;}
 float radius=0;void SetR(float r){radius=r;}void Set(int){}void SetStts(Status*){}void SetAtType(int){}void SetAtMtrl(int){}void OnAtNoConHit(){}
 void ResetAtHit(){}void OnAtSetBit(){}};
using cCcD_Obj=Collider;
using dCcD_GObjInf=Collider;
struct daAlink_c:Actor {Collider mAtSph,mAtCps[3],mGuardAtCps;int tevStr=0;unsigned shields=0;
 struct{short y=0;}shape_angle;struct{bool ChkGroundHit(){return true;}}mLinkAcch;
 bool checkEquipAnime(){return false;}int procGuardAttackInit(){++shields;return 1;}
 struct HIO{struct{struct{struct{float mAttackRadius=260,mLightAttackRadius=400,mLargeAttackRadius=400;}m;}mCutTurn;}mCut;} hio,*mpHIO=&hio;
}player;
struct Enemy{unsigned id=2;bool initialized=true,retiring=false,credited=false;};std::array<Enemy,12>enemies;
std::array<Actor,12>actors;
Enemy* tracked(unsigned id){for(auto&e:enemies)if(e.id==id)return &e;return nullptr;}
Actor* actor_by_id(unsigned id){for(auto&a:actors)if(a.id==id)return &a;return nullptr;}
unsigned fopAcM_GetID(Actor*a){return a->id;}int fopAcM_GetName(Actor*a){return a->name;}
constexpr int fpcNm_E_MM_MT_e=77;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
struct Effect{DamageQueue attribution;std::vector<uint64_t>bowVolleys;unsigned pending=0,meleeSerial=0,jumpSerial=0;bool pendingCritical=false,pendingStagger=false;};std::array<Effect,12>effects;
void queue_effect_damage(Effect& e,DamageSource source,unsigned power){e.pending+=power;e.attribution.add(source,power);}
Effect& effect_for(Enemy&e){return effects[&e-enemies.data()];}
struct Attack{bool manualFlight=false,aimPending=false;unsigned bowVolleySlot=20;uint64_t bowVolley=0;bool active=false;unsigned weapon=0,life=0,ballVictimCount=0,arrowTargets=1;std::array<unsigned,enemy_capacity>ballVictims{};
 Collider collider;Status status;std::array<unsigned,9>emitters{};float radius=0,effectBaseRadius=260;cXyz position,velocity;};
std::deque<Attack>s_attacks(initial_projectile_slots);
Run s_run;unsigned s_player=1,s_meleeSerial=1;
enum class SwordAction{None,Normal,Spin,Jump};SwordAction s_swordAction=SwordAction::Normal;
bool native_sword_collider(const Collider*c){return c==&player.mAtSph||c==&player.mAtCps[0]||c==&player.mAtCps[1]||c==&player.mAtCps[2];}
struct dCcU_AtInfo{Collider*mpCollider=nullptr;unsigned mAttackPower=0;};unsigned statusHits=0;
void record_status_hit(Actor*,dCcU_AtInfo*){++statusHits;}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void*a,unsigned i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}}
bool arena=true;bool in_arena(){return arena;}
struct Emitter{cXyz position,scale;int effect=0;void setGlobalTranslation(cXyz p){position=p;}};
std::unordered_map<unsigned,Emitter>emitters;unsigned nextEmitter=1;
Emitter*dComIfGp_particle_getEmitter(unsigned k){auto i=emitters.find(k);return i==emitters.end()?nullptr:&i->second;}
unsigned dComIfGp_particle_set(int,int effect,const cXyz*p,const int*,void*,const cXyz*scale,int,void*,int,void*,void*,void*){
 unsigned k=nextEmitter++;emitters[k]={*p,*scale,effect};return k;}
constexpr int kAttackSource=0,AT_TYPE_NORMAL_SWORD=1,dCcD_MTRL_NONE=0,ID_ZI_J_KAITENGIRI_A=1,ID_ZI_J_KAITENGIRI_B=2;
constexpr u16 ID_ZI_J_KAITENGIRID_A=21,ID_ZI_J_KAITENGIRID_B=22,ID_ZI_J_KAITENGIRID_C=23,ID_ZI_J_KAITENGIRID_D=24,ID_ZI_J_KAITENGIRID_E=25,ID_ZI_J_KAITENGIRID_F=26;
#include "survivors_spin_effects.inc"

void stop_attack(Attack&a){for(auto k:a.emitters)emitters.erase(k);a.active=false;}
float distance=100;bool enemy_in_range(daAlink_c*,float range){return distance<range;}
bool manual=false;bool manual_attack(daAlink_c*){return manual;}
cXyz manual_direction(daAlink_c*){return{1,0,0};}
cXyz target_direction(daAlink_c*){return{0,0,1};}
std::array<unsigned,unsigned(Upgrade::Rupees)>s_cooldowns{};unsigned waves=0;
unsigned spins=0;bool spinReady=true;bool begin_spin_attack(daAlink_c*){if(!spinReady)return false;++spins;s_swordAction=SwordAction::Spin;return true;}
bool waveReady=true;
 bool poseReady=true;bool ranged_pose_active(){return false;}bool ranged_pose_available(daAlink_c*){return poseReady;}
 short cM_atan2s(float x,float z){return short(std::atan2(x,z)*32768/3.14159265);}
 bool s_wavePending=false,s_trackMelee=false;unsigned s_meleeTarget=kNone,s_comboStep=0,s_comboTimeout=0;
 void new_melee_swing(){++s_meleeSerial;}
 bool guardBlocked=false;bool block_boss_contact(const Enemy&,Actor*,Collider*,Collider*){return guardBlocked;}

void update_sword_wave(daAlink_c*){if(waveReady&&s_run.ranks[0]&&!s_cooldowns[0]){++waves;s_cooldowns[0]=s_run.cooldown(60);}}
'''
manual_source=(root/'src/survivors_manual_attack.inc').read_text()
start=manual_source.index('bool manual_skill(')
fixture+=manual_source[start:manual_source.index('\n}',start)+2]
main=r'''
int main(){
 s_run.start(123);enemies[0].id=2;enemies[1].id=3;actors[0].id=2;actors[1].id=3;
 Collider target;target.owner=&actors[0];
 auto hit=[&](Collider*at,Collider*tg){std::any args[]={nullptr,at,tg};return before_concurrent_hit(nullptr,args,nullptr,nullptr);};
 // Four independent same-frame projectiles survive the native one-hit boundary.
 for(unsigned i=0;i<4;++i){auto&a=s_attacks[i];a.active=true;a.weapon=8+i;a.collider.power=3+i;
  assert(hit(&a.collider,&target)==HOOK_SKIP_ORIGINAL);}
 assert(effects[0].pending==18&&statusHits==4);
 // Native ChkAtHit needs both the base hit and a resolvable target actor ID.
 for(unsigned i=0;i<4;++i)assert(s_attacks[i].collider.ChkAtHit()&&s_attacks[i].collider.hitActorId==2);
 for(unsigned frame=0;frame<15;++frame)for(unsigned i=0;i<4;++i)hit(&s_attacks[i].collider,&target);
 assert(effects[0].pending==18); // persistent bomb/pulse/wave collision is not DPS per frame
 // Fresh arrow, even in the very next frame, is an additional hit.
 s_attacks[0].ballVictimCount=0;hit(&s_attacks[0].collider,&target);assert(effects[0].pending==21);
 // Multiple separate arrows in the same frame may hit the same boss.
 for(unsigned i=4;i<8;++i){auto&a=s_attacks[i];a.active=true;a.weapon=8;a.collider.power=3;hit(&a.collider,&target);}
 assert(effects[0].pending==33);
 // A single arrow cannot damage a second target, but a wave can.
 target.owner=&actors[1];hit(&s_attacks[0].collider,&target);assert(!effects[1].pending);
 s_attacks[8].active=true;s_attacks[8].weapon=0;s_attacks[8].collider.power=7;
 hit(&s_attacks[8].collider,&target);target.owner=&actors[0];hit(&s_attacks[8].collider,&target);
 assert(effects[1].pending==7&&effects[0].pending==40);
 // Ball return clears just its own per-leg victims; every new pass can hit.
 s_attacks[3].ballVictimCount=0;hit(&s_attacks[3].collider,&target);assert(effects[0].pending==46);
 assert(effects[0].pendingStagger);
 // Several sword capsules are one swing; the next swing is independent.
 hit(&player.mAtCps[0],&target);hit(&player.mAtCps[1],&target);assert(effects[0].pending==49);
 ++s_meleeSerial;hit(&player.mAtCps[2],&target);assert(effects[0].pending==52);
 // Jump's sword slash and its landing splash are distinct activations.
 s_swordAction=SwordAction::Jump;++s_meleeSerial;s_run.ranks[unsigned(Upgrade::JumpStrike)]=1;
 hit(&player.mAtCps[0],&target);hit(&player.mAtSph,&target);hit(&player.mAtSph,&target);assert(effects[0].pending==56);
 // Native Spin uses one activation per enemy, with Spin attribution and no
 // parallel pulse causing a second hit in the same animation.
 s_swordAction=SwordAction::Spin;++s_meleeSerial;s_run.ranks[1]=1;
 const auto beforeSpin=effects[0].pending;hit(&player.mAtSph,&target);hit(&player.mAtSph,&target);
 assert(effects[0].pending==beforeSpin+s_run.attack_power(1));
 effects[0].pending=56;s_swordAction=SwordAction::Jump;
 // Armor child's collision feeds the parent, without double-counting its body.
 Actor shell;shell.id=77;shell.name=fpcNm_E_MM_MT_e;shell.parentActorID=2;target.owner=&shell;
 ++s_meleeSerial;hit(&player.mAtCps[0],&target);assert(player.mAtCps[0].hitActorId==shell.id);
 target.owner=&actors[0];hit(&player.mAtCps[1],&target);assert(effects[0].pending==59);
 Collider foreign;assert(hit(&foreign,&target)==HOOK_CONTINUE);
 arena=false;assert(hit(&s_attacks[0].collider,&target)==HOOK_CONTINUE);arena=true;
 s_run.phase=Phase::Upgrade;assert(hit(&s_attacks[0].collider,&target)==HOOK_CONTINUE);s_run.phase=Phase::Running;
 enemies[0].credited=true;assert(hit(&s_attacks[0].collider,&target)==HOOK_CONTINUE);enemies[0].credited=false;

 // Native shield owns its animation and damage capsule, no independent pulse.
 s_run.ranks[0]=s_run.ranks[2]=1;s_run.ranks[1]=0;s_run.shield=1;s_cooldowns={};
 const auto shields=player.shields;update_parallel_skills(&player);
 assert(waves==1&&player.shields==shields+1&&s_swordAction==SwordAction::None);
 assert(player.mGuardAtCps.radius==s_run.shield_radius()&&s_cooldowns[2]);
 effects={};target.owner=&actors[0];
 hit(&player.mGuardAtCps,&target);hit(&player.mGuardAtCps,&target);
 assert(effects[0].pending==s_run.attack_power(2)&&effects[0].pendingStagger);
 guardBlocked=true;++s_meleeSerial;hit(&player.mGuardAtCps,&target);
 assert(effects[0].pending==s_run.attack_power(2));guardBlocked=false;
 // Busy body animation leaves Shield ready instead of spending its cooldown
 // through a manual Shockwave fallback.
 poseReady=false;s_cooldowns={};s_run.ranks[unsigned(Upgrade::Shockwave)]=1;manual=true;
 const auto count=player.shields;update_parallel_skills(&player);assert(player.shields==count&&!s_cooldowns[2]);
 poseReady=true;update_parallel_skills(&player);assert(player.shields==count+1&&s_cooldowns[2]);
 for(bool shieldManual:{false,true})for(bool waveManual:{false,true})for(bool targetPresent:{false,true}) {
  s_run.start(123);s_run.shield=1;s_run.ranks[2]=s_run.ranks[unsigned(Upgrade::Shockwave)]=1;
  s_run.options.manual_aim[unsigned(ManualAbility::ShieldAttack)]=shieldManual;
  s_run.options.manual_aim[unsigned(ManualAbility::Shockwave)]=waveManual;
  s_cooldowns={};s_attacks=std::deque<Attack>(initial_projectile_slots);distance=targetPresent?100:100000;manual=true;
  const auto before=player.shields;update_parallel_skills(&player);
  assert((player.shields>before)==(shieldManual||targetPresent));
  const auto wave=std::find_if(s_attacks.begin(),s_attacks.end(),[](const auto&a){return a.active&&a.weapon==unsigned(Upgrade::Shockwave);});
  assert((wave!=s_attacks.end())==(waveManual||targetPresent));
  if(wave!=s_attacks.end())assert(wave->velocity.x==(waveManual?45:0)&&wave->aimPending==waveManual);
 }
 manual=false;
 // Same-volley extra hits are quarter damage; separate/overlapping volleys
 // and different targets retain full first-hit damage, even on one frame.
 effects={};s_attacks=std::deque<Attack>(initial_projectile_slots);target.owner=&actors[0];s_run.ranks[unsigned(Upgrade::CritChance)]=0;
 for(unsigned i=0;i<4;++i){auto&a=s_attacks[i];a.active=true;a.weapon=unsigned(Upgrade::Bow);a.collider.power=20;
  a.bowVolleySlot=i==2?101:100;a.bowVolley=i==2?101:100;}
 hit(&s_attacks[0].collider,&target);hit(&s_attacks[1].collider,&target);
 hit(&s_attacks[2].collider,&target);hit(&s_attacks[3].collider,&target);
 assert(effects[0].pending==50);hit(&s_attacks[3].collider,&target);assert(effects[0].pending==50);
 s_attacks[4]=s_attacks[0];s_attacks[4].ballVictimCount=0;target.owner=&actors[1];
 hit(&s_attacks[4].collider,&target);assert(effects[1].pending==20);
 s_attacks[5]=s_attacks[0];s_attacks[5].ballVictimCount=0;s_attacks[5].bowVolley=102;
 target.owner=&actors[0];hit(&s_attacks[5].collider,&target);assert(effects[0].pending==70);
 // Every piercing rank applies independently to every arrow in a volley.
 // Duplicated target geometry consumes neither an extra target nor extra damage.
 s_run.start(123);
 for(unsigned rank=0;rank<=10;++rank){
  effects={};s_attacks=std::deque<Attack>(initial_projectile_slots);s_run.ranks[unsigned(Upgrade::Piercing)]=rank;
  for(unsigned i=0;i<actors.size();++i){actors[i].id=enemies[i].id=100+i;actors[i].health=100;}
  for(unsigned shot=0;shot<8;++shot){auto& arrow=s_attacks[shot];
   arrow.active=true;arrow.weapon=unsigned(Upgrade::Bow);arrow.collider.power=20;
   arrow.arrowTargets=s_run.arrow_targets();arrow.bowVolley=500;arrow.bowVolleySlot=0;
   for(unsigned i=0;i<actors.size();++i){target.owner=&actors[i];
    hit(&arrow.collider,&target);hit(&arrow.collider,&target);
    assert(effects[i].pending==(i<=rank?20+5*shot:0));
   }
   assert(arrow.ballVictimCount==rank+1);
  }
 }
 // One independently moving piercing shockwave per shield bash; direct
 // melee damage reaches every target once, without using Shield's damage rank.
 s_run.start(1);s_run.shield=1;s_run.ranks[2]=1;s_run.ranks[unsigned(Upgrade::Shockwave)]=1;
 s_run.ranks[unsigned(Upgrade::Power)]=17;distance=100;
 for(unsigned reach:{0u,5u,30u}){
  effects={};s_attacks=std::deque<Attack>(initial_projectile_slots);s_run.ranks[unsigned(Upgrade::Reach)]=reach;
  assert(launch_parallel_pulse(&player,Upgrade::ShieldAttack));
  auto& wave=s_attacks.back();assert(wave.weapon==unsigned(Upgrade::Shockwave)&&wave.life==30);
  assert(wave.radius==s_run.radius(95)&&wave.velocity.z==45&&wave.collider.power==s_run.melee_power());
  for(unsigned i=0;i<actors.size();++i){target.owner=&actors[i];hit(&wave.collider,&target);hit(&wave.collider,&target);
   assert(effects[i].pending==s_run.melee_power());}
 }
 distance=1e6;assert(!launch_parallel_pulse(&player,Upgrade::Spin));
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_concurrent_hits.inc').read_text()+(root/'src/survivors_parallel.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors parallel: overlapping skills, simultaneous hits, rapid shots, per-activation dedup, armor, isolation and pulse geometry passed')

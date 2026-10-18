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
 void Set(int){}void SetStts(Status*){}void SetAtType(int){}void SetAtMtrl(int){}void OnAtNoConHit(){}
 void ResetAtHit(){}void OnAtSetBit(){}};
using cCcD_Obj=Collider;
using dCcD_GObjInf=Collider;
struct daAlink_c:Actor {Collider mAtSph,mAtCps[3];int tevStr=0;
 struct HIO{struct{struct{struct{float mAttackRadius=260,mLightAttackRadius=400,mLargeAttackRadius=400;}m;}mCutTurn;}mCut;} hio,*mpHIO=&hio;
}player;
struct Enemy{unsigned id=2;bool initialized=true,retiring=false,credited=false;};std::array<Enemy,2>enemies;
std::array<Actor,2>actors;
Enemy* tracked(unsigned id){for(auto&e:enemies)if(e.id==id)return &e;return nullptr;}
Actor* actor_by_id(unsigned id){for(auto&a:actors)if(a.id==id)return &a;return nullptr;}
unsigned fopAcM_GetID(Actor*a){return a->id;}int fopAcM_GetName(Actor*a){return a->name;}
constexpr int fpcNm_E_MM_MT_e=77;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
struct Effect{std::vector<uint64_t>bowVolleys;unsigned pending=0,meleeSerial=0,jumpSerial=0;bool pendingCritical=false,pendingStagger=false;};std::array<Effect,2>effects;
Effect& effect_for(Enemy&e){return effects[&e-enemies.data()];}
struct Attack{unsigned bowVolleySlot=20;uint64_t bowVolley=0;bool active=false;unsigned weapon=0,life=0,ballVictimCount=0;std::array<unsigned,enemy_capacity>ballVictims{};
 Collider collider;Status status;std::array<unsigned,9>emitters{};float radius=0;cXyz position,velocity;};
std::array<Attack,initial_projectile_slots>s_attacks;
Run s_run;unsigned s_player=1,s_meleeSerial=1;
enum class SwordAction{None,Normal,Jump};SwordAction s_swordAction=SwordAction::Normal;
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
cXyz target_direction(daAlink_c*){return{0,0,1};}
std::array<unsigned,unsigned(Upgrade::Rupees)>s_cooldowns{};unsigned waves=0;
bool waveReady=true;
void update_sword_wave(daAlink_c*){if(waveReady&&s_run.ranks[0]&&!s_cooldowns[0]){++waves;s_cooldowns[0]=s_run.cooldown(60);}}
'''
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
 // Armor child's collision feeds the parent, without double-counting its body.
 Actor shell;shell.id=77;shell.name=fpcNm_E_MM_MT_e;shell.parentActorID=2;target.owner=&shell;
 ++s_meleeSerial;hit(&player.mAtCps[0],&target);assert(player.mAtCps[0].hitActorId==shell.id);
 target.owner=&actors[0];hit(&player.mAtCps[1],&target);assert(effects[0].pending==59);
 Collider foreign;assert(hit(&foreign,&target)==HOOK_CONTINUE);
 arena=false;assert(hit(&s_attacks[0].collider,&target)==HOOK_CONTINUE);arena=true;
 s_run.phase=Phase::Upgrade;assert(hit(&s_attacks[0].collider,&target)==HOOK_CONTINUE);s_run.phase=Phase::Running;
 enemies[0].credited=true;assert(hit(&s_attacks[0].collider,&target)==HOOK_CONTINUE);enemies[0].credited=false;
 // Wave, spin and shield become active together, without changing Link's sword action.
 s_run.ranks[0]=s_run.ranks[1]=s_run.ranks[2]=1;s_run.shield=1;s_swordAction=SwordAction::Normal;
 update_parallel_skills(&player);assert(waves==1&&s_attacks[kSpinSlot].active&&s_attacks[kShieldSlot].active);
 assert(s_swordAction==SwordAction::Normal&&s_cooldowns[0]&&s_cooldowns[1]&&s_cooldowns[2]);
 update_parallel_skills(&player);assert(waves==1);
 waveReady=false;s_cooldowns={};update_parallel_skills(&player);
 assert(waves==1&&!s_cooldowns[0]&&s_cooldowns[1]&&s_cooldowns[2]);waveReady=true;
 for(unsigned reach:{0u,5u,30u})for(unsigned master:{0u,5u,30u})for(unsigned level:{1u,14u,15u,30u}){
  s_run.ranks[1]=level;s_run.ranks[5]=reach;s_run.master_sword=master;assert(launch_parallel_pulse(&player,Upgrade::Spin));
  auto&a=s_attacks[kSpinSlot];float base=level>=15?400:260;float radius=s_run.area_radius(base);
  assert(a.radius==radius);unsigned count=0;
  for(auto k:a.emitters)if(k){assert(std::abs(emitters[k].scale.x*base-radius)<.01);
   assert(emitters[k].effect==survivor_spin_effects(level>=15)[count]);++count;}
  assert(count==(level>=15?6u:2u));
  player.current.pos={20,30,40};update_parallel_pulse(a,&player);assert(a.position.y==95);
 }
 // Same-volley extra hits are quarter damage; separate/overlapping volleys
 // and different targets retain full first-hit damage, even on one frame.
 effects={};s_attacks={};target.owner=&actors[0];s_run.ranks[unsigned(Upgrade::CritChance)]=0;
 for(unsigned i=0;i<4;++i){auto&a=s_attacks[i];a.active=true;a.weapon=unsigned(Upgrade::Bow);a.collider.power=20;
  a.bowVolleySlot=i==2?101:100;a.bowVolley=i==2?101:100;}
 hit(&s_attacks[0].collider,&target);hit(&s_attacks[1].collider,&target);
 hit(&s_attacks[2].collider,&target);hit(&s_attacks[3].collider,&target);
 assert(effects[0].pending==50);hit(&s_attacks[3].collider,&target);assert(effects[0].pending==50);
 s_attacks[4]=s_attacks[0];s_attacks[4].ballVictimCount=0;target.owner=&actors[1];
 hit(&s_attacks[4].collider,&target);assert(effects[1].pending==20);
 s_attacks[5]=s_attacks[0];s_attacks[5].ballVictimCount=0;s_attacks[5].bowVolley=102;
 target.owner=&actors[0];hit(&s_attacks[5].collider,&target);assert(effects[0].pending==70);
 distance=1e6;assert(!launch_parallel_pulse(&player,Upgrade::Spin));
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_concurrent_hits.inc').read_text()+(root/'src/survivors_parallel.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors parallel: overlapping skills, simultaneous hits, rapid shots, per-activation dedup, armor, isolation and pulse geometry passed')

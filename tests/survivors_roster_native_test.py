"""Production roster adapters: typed targets, linear hits, status, lifecycle and AI gates."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
native=(root/'src/survivors_roster_native.inc').read_text().split('// Suppress only')[0]
profiles=sorted(set(re.findall(r'fpcNm_\w+',native))|{'fpcNm_E_MM_MT_e'})
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include "survivors_rules.hpp"
#include "survivors_roster.hpp"
using namespace dawnlight::survivors;
using ProfileName=int;using ActorId=unsigned;using u32=unsigned;using s16=short;using s8=signed char;
constexpr ActorId kNone=~0u;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* p,unsigned i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator-(cXyz v)const{return{x-v.x,y-v.y,z-v.z};}float absXZ()const{return std::sqrt(x*x+z*z);}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
struct fopAc_ac_c {ActorId id=1,parentActorID=kNone;int profile=0,health=100,field_0x560=100;
 struct {cXyz pos;struct{s16 y=0;}angle;}current,home;struct{s16 y=0;}shape_angle;cXyz scale{1,1,1};};
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
std::map<ActorId,fopAc_ac_c*> actors;fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
struct dCcD_Stts {void Move(){}void Init(int,int,fopAc_ac_c*){}};
struct cCcD_Obj {fopAc_ac_c* owner=nullptr;bool tg=true,at=true;fopAc_ac_c* GetAc(){return owner;}
 void OffTgSetBit(){tg=false;}void OffAtSetBit(){at=false;}};
struct dCcD_GObjInf:cCcD_Obj {cCcD_Obj* hit=nullptr;bool shield=true;unsigned type=0,atp=0;float r=1,h=1;
 bool ChkTgHit(){return hit;}cCcD_Obj* GetTgHitObj(){return hit;}void ResetTgHit(){hit=nullptr;}
 void OnTgSetBit(){tg=true;}void SetTgType(unsigned t){type=t;}void OffTgShield(){shield=false;}
 void OffTgShieldFrontRange(){}void SetR(float n){r=n;}void SetH(float n){h=n;}
 void Set(int){}void SetStts(dCcD_Stts*){}void SetAtType(unsigned){}void SetAtAtp(unsigned n){atp=n;}
 void SetAtMtrl(int){} };
using dCcD_Sph=dCcD_GObjInf;using dCcD_Cyl=dCcD_GObjInf;
struct dCcU_AtInfo {cCcD_Obj* mpCollider=nullptr;int mPowerType=1;};
struct Model {cXyz scale;void setBaseScale(cXyz s){scale=s;}};
struct mDoExt_McaMorfSO {Model model;Model* getModel(){return &model;}};
// Some SDK actors use a scalar sphere, others an array; both share the same mock interface.
struct TargetSet:dCcD_GObjInf {dCcD_GObjInf extra[3];
 dCcD_GObjInf& operator[](unsigned i){return i?extra[i-1]:*this;}};
struct NativeActor:fopAc_ac_c {
 enum {ACT_WAIT=1,ACT_MOVE=2,ACT_CHASEH=3};
 int mAction=0,mMode=0,mActionMode=0,mMoveMode=0,mActionMode1=0,action=0,mode=0,mActionPhase=0;
 int field_0x5b4=0,mCurAction=0,field_0x672=0,mCryTimer=0,combine_off_timer=0;
 float field_0x6d4=0,pl_range=0,field_0x6b8=0,mRecognizeDist=0,pl_recognize_dist=0,pl_search_range=0;
 s16 angle_target=0,mTargetAngleY=0,mCurrentAngleYTarget=0,field_0x6c0=0;int timers[4]{};
 ActorId armorChildId=kNone;
 dCcD_Sph mSphere,mSphere2,mSphere3,cc_sph[3],tate_sph,mCcSph[3],mCcSphs[3],mTateSph;
 dCcD_Sph field_0xa7c[3],field_0xf5c,mCcHookCyl,mCcShieldSph,mSphA[3],mSphB[3],mSphC,m_sphere,m_sph;
 TargetSet ccSph;dCcD_Cyl mCcCyl;
 dCcU_AtInfo mAtInfo,at_info,atInfo;dCcD_Stts mStts,stts,ccStts,mCcStts,mStts1;
 mDoExt_McaMorfSO morph; mDoExt_McaMorfSO* mpMorfSO=&morph,*mpMorf=&morph,*anm_p=&morph,*modelMorf=&morph,*mpModelMorf=&morph,*mpModelMorf1=&morph;
 void setActionMode(int a,int m=0){mAction=mActionMode=mActionMode1=a;mMode=mMoveMode=m;}
 void setBck(int,int,float,float){}
};
using daE_TT_c=NativeActor;using daE_GE_c=NativeActor;using e_kr_class=NativeActor;using e_dn_class=NativeActor;
using e_mm_class=NativeActor;using e_mm_mt_class=NativeActor;using e_cr_class=NativeActor;using e_bs_class=NativeActor;
using daE_GI_c=NativeActor;using e_sf_class=NativeActor;using e_fs_class=NativeActor;using e_bu_class=NativeActor;
using e_sm2_class=NativeActor;using e_mf_class=NativeActor;using daB_GG_c=NativeActor;using daB_TN_c=NativeActor;
constexpr int ACTION_COMBINE=2,ACTION_ATTACK=1;
struct Enemy {ActorId id=kNone;ProfileName profile=0;unsigned species=0,stagger=0,hitCooldown=0;
 cCcD_Obj* childHit=nullptr;bool elite=false,credited=false,retiring=false,initialized=true;};
std::array<Enemy,enemy_capacity> s_enemies;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id!=kNone&&e.id==id)return &e;return nullptr;}
Run s_run;bool arena=true;bool in_arena(){return arena;}
bool arena_aggro(fopAc_ac_c* a,int p){return arena&&a&&a->profile==p;}
struct Player:fopAc_ac_c {dCcD_Sph mGuardAtCps;}player;
Player* daAlink_getAlinkActorClass(){return &player;}ActorId s_player=100;
struct Effect {unsigned pending=0,damage=0,meleeSerial=0;bool injected=false,critical=false,pendingCritical=false;
 dCcD_Stts status;dCcD_Sph collider;};
std::array<Effect,enemy_capacity> effects;
Effect& effect_for(Enemy& e){return effects[&e-s_enemies.data()];}
struct Attack {dCcD_Sph collider;};std::array<Attack,4>s_attacks;
dCcD_Sph sword;bool native_sword_collider(const cCcD_Obj* a){return a==&sword;}
bool effect_collider(const cCcD_Obj* a){for(auto& e:effects)if(a==&e.collider)return true;return false;}
enum class SwordAction {Normal,None};SwordAction s_swordAction=SwordAction::Normal;unsigned s_meleeSerial=1;
constexpr int kAttackSource=0,AT_TYPE_ARROW=1,dCcD_MTRL_NONE=0;
int ballResult=-1;int ball_contact(fopAc_ac_c*,const cCcD_Obj*){return ballResult;}
bool causes_stagger(const cCcD_Obj* a){return a==&player.mGuardAtCps;}
unsigned calls=0,power=7,credits=0,disappears=0,deletes=0;bool injectedCritical=false;
void cc_at_check(fopAc_ac_c* a,dCcU_AtInfo* info){++calls;assert(info->mPowerType==0);
 unsigned damage=power;
 for(auto& e:effects)if(info->mpCollider==&e.collider){damage=e.damage;injectedCritical=e.critical;}
 a->health=std::max(0,a->health-int(damage));}
void credit_death(Enemy& e,fopAc_ac_c*){if(!e.credited){++credits;e.credited=true;}}
void fopAcM_createDisappear(fopAc_ac_c*,cXyz*,int,int,int){++disappears;}
struct Service{void delete_actor(ModContext*,ActorId){++deletes;}}service;auto* svc_actor=&service;ModContext* mod_ctx=nullptr;
struct Collision {unsigned sets=0;void Set(cCcD_Obj*){++sets;}}collision;Collision* dComIfG_Ccsp(){return &collision;}
'''
main=r'''
int main(){
 player.id=s_player;player.current.pos={100,0,0};s_run.start(5);
 for(unsigned kind=5;kind<species_count;++kind) {
  NativeActor actor;actor.profile=native_roster[kind].profile;actors={{1,&actor}};
  auto& enemy=s_enemies[0];s_enemies={};enemy.id=1;enemy.species=kind;enemy.profile=actor.profile;
  auto body=extended_body(&actor,actor.profile);assert(body.targets[0]&&body.info&&body.status&&body.morph);
  initialize_extended_enemy(enemy,&actor);assert(actor.health==int(native_roster[kind].hp));
  s_run.options.enemy_hp=3;enemy.elite=true;actor.scale={elite_scale(Species(kind)),elite_scale(Species(kind)),elite_scale(Species(kind))};
  initialize_extended_enemy(enemy,&actor);assert(actor.health==360);s_run.options.enemy_hp=1;
  actor.health=100;
  for(auto* target:body.targets)if(target){target->owner=&actor;target->hit=&sword;}
  const auto beforeCalls=calls;
  assert(update_extended_enemy(&actor,enemy));assert(calls==beforeCalls+1&&actor.health==93);
  for(auto* target:body.targets)if(target){assert(!target->hit);std::any args[]={0,static_cast<cCcD_Obj*>(target)};
   before_roster_targets(nullptr,args,nullptr,nullptr);const float radius=target->r;
   before_roster_targets(nullptr,args,nullptr,nullptr);assert(target->r==radius&&!target->shield&&target->tg);
  }
  body.targets[0]->hit=&sword;update_extended_enemy(&actor,enemy);assert(actor.health==93); // real cooldown
  auto& effect=effect_for(enemy);effect.pending=13;effect.pendingCritical=true;enemy.hitCooldown=0;
  update_extended_enemy(&actor,enemy);assert(actor.health==80&&injectedCritical&&!effect.pending&&!effect.injected&&!effect.critical);
  s_run.dual_wield=1;effect.meleeSerial=s_meleeSerial;body.targets[0]->hit=&sword;enemy.hitCooldown=0;
  update_extended_enemy(&actor,enemy);assert(actor.health==80);s_run.dual_wield=0;
  body.targets[0]->hit=&s_attacks[0].collider;ballResult=0;update_extended_enemy(&actor,enemy);assert(actor.health==80);
  ballResult=1;enemy.hitCooldown=10;body.targets[0]->hit=&s_attacks[0].collider;
  update_extended_enemy(&actor,enemy);assert(actor.health==73);ballResult=-1;
  enemy.hitCooldown=0;body.targets[0]->hit=&player.mGuardAtCps;
  assert(!update_extended_enemy(&actor,enemy)&&enemy.stagger==7&&actor.health==66);
  for(int n=0;n<7;++n)assert(!update_extended_enemy(&actor,enemy));
  assert(update_extended_enemy(&actor,enemy));
  // Foreign contacts never become run damage; only validated attacks count.
  dCcD_Sph foreign;body.targets[0]->hit=&foreign;enemy.hitCooldown=0;
  update_extended_enemy(&actor,enemy);assert(actor.health==66);
  std::any args[]={static_cast<fopAc_ac_c*>(&actor)};
  assert(before_roster_native_damage(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
  arena=false;assert(before_roster_native_damage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);arena=true;
  bool cry=true;before_roster_cry(nullptr,args,&cry,nullptr);assert(!cry);
  enemy.hitCooldown=0;body.targets[0]->hit=&sword;actor.health=7;unsigned beforeCredits=credits;
  assert(!update_extended_enemy(&actor,enemy)&&actor.health==0&&enemy.retiring&&enemy.credited&&credits==beforeCredits+1);
  // Normal runtime does not execute retiring actors again.
 }
 assert(credits==species_count-5&&disappears==credits&&deletes==credits);
 // Shell targets use parent health and lose shield immunity, while preserving model ownership.
 NativeActor helmet,shell;helmet.profile=fpcNm_E_MM_e;helmet.id=1;helmet.armorChildId=2;
 shell.id=2;shell.profile=fpcNm_E_MM_MT_e;shell.parentActorID=1;
 actors={{1,&helmet},{2,&shell}};s_enemies={};auto& e=s_enemies[0];e.id=1;e.profile=fpcNm_E_MM_e;
 auto body=extended_body(&helmet,e.profile);assert(body.targets[1]==&shell.m_sphere&&body.targets[2]==&shell.m_sph);
 shell.m_sphere.owner=&shell;std::any targetArgs[]={0,static_cast<cCcD_Obj*>(&shell.m_sphere)};
 before_roster_targets(nullptr,targetArgs,nullptr,nullptr);assert(!shell.m_sphere.shield);
 shell.m_sphere.hit=&sword;effects={};update_extended_enemy(&helmet,e);assert(helmet.health==93&&shell.health==100);
 // Child-first execution queues one hit without triggering native armor damage.
 e.hitCooldown=0;shell.m_sphere.hit=&sword;std::any shellArgs[]={static_cast<e_mm_mt_class*>(&shell)};
 before_roster_shell(nullptr,shellArgs,nullptr,nullptr);assert(e.childHit==&sword&&!shell.m_sphere.hit);
 update_extended_enemy(&helmet,e);assert(helmet.health==86&&!e.childHit);
 update_extended_enemy(&helmet,e);assert(helmet.health==86);
 NativeActor tektite;tektite.profile=fpcNm_E_TT_e;std::any searchArgs[]={static_cast<fopAc_ac_c*>(&tektite)};bool found=false;
 assert(before_roster_search(nullptr,searchArgs,&found,nullptr)==HOOK_SKIP_ORIGINAL&&found);
 NativeActor bubble;bubble.profile=fpcNm_E_BU_e;bubble.pl_search_range=1e9;
 std::any bubbleArgs[]={static_cast<e_bu_class*>(&bubble),2000.f};int seen=0;
 assert(before_roster_bubble_search(nullptr,bubbleArgs,&seen,nullptr)==HOOK_SKIP_ORIGINAL&&seen);
 bubbleArgs[1]=200.f;assert(before_roster_bubble_search(nullptr,bubbleArgs,&seen,nullptr)==HOOK_CONTINUE);
 arena=false;assert(before_roster_search(nullptr,searchArgs,&found,nullptr)==HOOK_CONTINUE);arena=true;
 // Acquisition changes no real player distances and excludes heavy-hit stagger.
 NativeActor liz;e.profile=fpcNm_E_DN_e;e.stagger=0;engage_extended_enemy(&liz,e);assert(liz.action==3&&liz.pl_range>100000);
 assert(extended_locomotion(&liz,e.profile));liz.action=5;assert(!extended_locomotion(&liz,e.profile));
 face_extended_enemy(&liz,e);assert(liz.shape_angle.y==0x4000);e.stagger=1;liz.shape_angle.y=7;face_extended_enemy(&liz,e);assert(liz.shape_angle.y==7);
 NativeActor chu;e.stagger=0;e.profile=fpcNm_E_SM2_e;chu.action=ACTION_COMBINE;engage_extended_enemy(&chu,e);
 assert(chu.combine_off_timer==30000&&chu.action==ACTION_ATTACK);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    constants='constexpr int '+','.join(f'{p}={i+1}' for i,p in enumerate(profiles))+';\n'
    cpp.write_text(fixture+constants+native+(root/'src/survivors_extended_ai.inc').read_text()+
                   (root/'src/survivors_extended_damage.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors roster: all typed targets, HP, one-hit damage/status, cooldowns, stagger, shell routing and defeat ownership passed')

"""Execute production shield and nonlethal reaction hooks at engine boundaries."""
from pathlib import Path
import os
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors_defense.inc').read_text()
fixture=r'''
#include <cassert>
#include <cmath>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;using ProfileName=int;
constexpr int fpcNm_E_OC_e=1,fpcNm_E_BA_e=2,fpcNm_E_MS_e=3;
constexpr int dCcD_MTRL_NONE=0,dCcD_MTRL_LIGHT=1;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct cXyz{float x=0,y=0,z=0;cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
struct fopAc_ac_c{int id=2,health=50;struct{cXyz pos;struct{s16 y=0;}angle;}current;};
struct Collider{
 bool hit=false,at=true,tg=true,co=true;fopAc_ac_c* actor=nullptr;Collider* attack=nullptr;int material=0;
 bool ChkTgHit(){return hit;}fopAc_ac_c* GetTgHitAc(){return actor;}
 Collider* GetTgHitGObj(){return attack;}Collider* GetTgHitObj(){return attack;}
 void ResetTgHit(){hit=false;}void OffAtSetBit(){at=false;}void ResetAtHit(){}
 int GetAtSpl(){return 0;}void SetR(float){}
 bool ChkTgSet(){return tg;}bool ChkCoSet(){return co;}
 void OnTgSetBit(){tg=true;}void OffTgSetBit(){tg=false;}void OnCoSetBit(){co=true;}void OffCoSetBit(){co=false;}
 int GetAtMtrl(){return material;}void SetAtMtrl(int m){material=m;}
};
using cCcD_Obj=Collider;using dCcD_GObjInf=Collider;
struct daPy_py_c{enum{FLG2_UNK_8000000=1,FLG3_UNK_100000=2};};
struct daAlink_c:fopAc_ac_c{
 enum{PROC_WAIT,PROC_GUARD_ATTACK,PROC_GUARD_SLIP,PROC_GUARD_BREAK,PROC_CUT,UPPER_2};
 int mProcID=PROC_WAIT,mDamageTimer=0,field_0x2fd5=0,guardSounds=0,blocks=0;bool wolf=false,guard=true,light=false;
 struct{bool grounded=true;bool ChkGroundHit(){return grounded;}}mLinkAcch;
 struct{s16 y=0;}shape_angle;
 Collider mTgCyls[3],mAtCps[3],mAtSph,mGuardAtCps;
 bool checkWolf(){return wolf;}void resetUpperAnime(int,int){}
 void setGuardSe(Collider*){++guardSounds;}
 void procGuardSlipInit(int,Collider*){mProcID=PROC_GUARD_SLIP;++blocks;}
 bool procGuardAttackInit(){mProcID=PROC_GUARD_ATTACK;return true;}
 void offNoResetFlg2(int){guard=false;}void onNoResetFlg3(int){light=true;}
}player;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
int fopAcM_GetID(fopAc_ac_c* a){return a->id;}
struct daE_OC_c:fopAc_ac_c{
 int mActionMode=3,mOcState=7,mOldActionMode=2; s16 field_0x6c0=12,field_0x6c2=14,field_0x6cc=0;
 Collider mSphs_cc[2];
};
struct e_ba_class:fopAc_ac_c{int mAction=2,mMode=3,mIFrames=0;float mKnockbackSpeed=0;s16 mKnockbackAngle=0;bool mIsDying=false;Collider mSph;};
struct e_ms_class:fopAc_ac_c{int mAction=1,mMode=3;s16 mCooldown1=0;Collider mSph1;};
struct Enemy{int profile=1;bool initialized=true,retiring=false,credited=false;}enemy;
bool present=true,playing=true;
Enemy* tracked(int id){return present&&id==2?&enemy:nullptr;}
bool in_arena(){return playing;}bool playable(){return playing;}
struct{bool active=false;}s_ranged;void cancel_ranged(){s_ranged.active=false;}
enum class SwordAction{None,Normal};SwordAction s_swordAction=SwordAction::Normal;
bool s_wavePending=true;unsigned s_comboStep=3,s_comboTimeout=50;
Run s_run;int s_player=1;std::array<unsigned,13>s_cooldowns{};
bool sword_action_available(daAlink_c* p){return p->mProcID==daAlink_c::PROC_WAIT;}
bool near=true;bool enemy_in_range(daAlink_c*,float){return near;}
cXyz target_direction(daAlink_c*){return {1,0,0};}
struct Attack{unsigned weapon=0,ballVictimCount=0;Collider collider;std::array<int,enemy_capacity>ballVictims{};};std::array<Attack,24>s_attacks;
struct dCcU_AtInfo{Collider* mpCollider;};
void prepare_effect_hit(fopAc_ac_c*,Enemy&){}
void finish_effect_hit(fopAc_ac_c*,ProfileName){}
void record_status_hit(fopAc_ac_c*,dCcU_AtInfo*){}
struct Args{void* first;void* second;};
namespace mods{template<class T>T arg(void* a,int i){return static_cast<T>(i?static_cast<Args*>(a)->second:static_cast<Args*>(a)->first);}}
'''
main=r'''
int main(){
 s_run.start(123);player.id=1;fopAc_ac_c attacker;attacker.current.pos={100,0,0};Collider hit;
 Args args{&player,nullptr};int result=0;
 // Zero rank never blocks; each rolled rank has its promised probability.
 for(unsigned rank:{0u,1u,4u,7u,10u}){
  s_run.shield=rank;unsigned blocks=0;
  for(unsigned i=0;i<10000;++i){player.mProcID=daAlink_c::PROC_CUT;
   player.mTgCyls[0].hit=true;player.mTgCyls[0].actor=&attacker;player.mTgCyls[0].attack=&hit;
   s_ranged.active=true;s_wavePending=true;s_comboStep=3;
   if(before_shield_block(nullptr,&args,&result,nullptr)==HOOK_SKIP_ORIGINAL){++blocks;
    assert(result==1&&player.shape_angle.y==0x4000&&player.mProcID==daAlink_c::PROC_GUARD_SLIP);
    assert(!player.mTgCyls[0].hit&&!s_ranged.active&&!s_wavePending&&s_comboStep==0&&!player.mAtSph.at);
   }
  }
  const int expected=int(rank*500);assert(std::abs(int(blocks)-expected)<180);
 }
 player.mProcID=daAlink_c::PROC_WAIT;
 assert(before_survivors_guard(nullptr,&args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL&&!player.guard);
 s_run.ranks[2]=1;s_ranged.active=false;
 assert(update_shield_attack(&player)&&player.mProcID==daAlink_c::PROC_GUARD_ATTACK&&s_cooldowns[2]);
 assert(before_survivors_guard(nullptr,&args,nullptr,nullptr)==HOOK_CONTINUE);
 // Native damage/effect code may run, but nonlethal ordinary hits preserve the action and timers.
 daE_OC_c bok;args={&bok,nullptr};enemy.profile=fpcNm_E_OC_e;
 bok.mSphs_cc[0].hit=true;bok.mSphs_cc[0].attack=&hit;
 before_enemy_hit(nullptr,&args,nullptr,nullptr);
 bok.health=40;bok.mActionMode=5;bok.mOcState=0;bok.mOldActionMode=3;bok.field_0x6c0=0;
 bok.mSphs_cc[0].tg=false;
 after_enemy_hit(nullptr,&args,nullptr,nullptr);
 assert(bok.health==40&&bok.mActionMode==3&&bok.mOcState==7&&bok.field_0x6c0==12&&bok.mSphs_cc[0].tg);
 // Both exception weapons retain native stagger; lethal ordinary hits retain death state.
 s_attacks[0].weapon=unsigned(Upgrade::Ball);
 for(auto* collider:{&player.mGuardAtCps,&s_attacks[0].collider}){
  bok.mActionMode=3;bok.mSphs_cc[0].attack=collider;
  before_enemy_hit(nullptr,&args,nullptr,nullptr);bok.mActionMode=5;after_enemy_hit(nullptr,&args,nullptr,nullptr);
  assert(bok.mActionMode==5);
 }
 bok.mSphs_cc[0].attack=&hit;before_enemy_hit(nullptr,&args,nullptr,nullptr);bok.health=0;bok.mActionMode=9;
 after_enemy_hit(nullptr,&args,nullptr,nullptr);assert(bok.mActionMode==9);
 e_ba_class bat;args={&bat,nullptr};enemy.profile=fpcNm_E_BA_e;bat.mSph.hit=true;bat.mSph.attack=&hit;
 before_enemy_hit(nullptr,&args,nullptr,nullptr);bat.health=30;bat.mKnockbackSpeed=80;bat.mAction=8;
 after_enemy_hit(nullptr,&args,nullptr,nullptr);assert(bat.health==30&&bat.mKnockbackSpeed==0&&bat.mAction==2);
 e_ms_class rat;args={&rat,nullptr};enemy.profile=fpcNm_E_MS_e;rat.mSph1.hit=true;rat.mSph1.attack=&hit;
 before_enemy_hit(nullptr,&args,nullptr,nullptr);rat.health=10;rat.mAction=4;rat.current.angle.y=300;
 after_enemy_hit(nullptr,&args,nullptr,nullptr);assert(rat.health==10&&rat.mAction==1&&rat.current.angle.y==0);
 present=false;before_enemy_hit(nullptr,&args,nullptr,nullptr);assert(s_hitStates.empty());present=true;
 // A ball can damage the same enemy once on each pass, even inside native i-frames.
 args={&rat,nullptr};rat.mCooldown1=10;rat.mSph1.hit=true;rat.mSph1.attack=&s_attacks[0].collider;
 s_attacks[0].ballVictimCount=0;before_enemy_hit(nullptr,&args,nullptr,nullptr);
 assert(rat.mCooldown1==0&&rat.mSph1.hit&&s_attacks[0].ballVictimCount==1);after_enemy_hit(nullptr,&args,nullptr,nullptr);
 before_enemy_hit(nullptr,&args,nullptr,nullptr);assert(!rat.mSph1.hit);after_enemy_hit(nullptr,&args,nullptr,nullptr);
 s_attacks[0].ballVictimCount=0;rat.mCooldown1=10;rat.mSph1.hit=true;
 before_enemy_hit(nullptr,&args,nullptr,nullptr);assert(rat.mCooldown1==0&&rat.mSph1.hit);after_enemy_hit(nullptr,&args,nullptr,nullptr);
 // Downed Bokoblins must be registered as targets before collision registration.
 args={&bok,nullptr};enemy.profile=fpcNm_E_OC_e;bok.health=50;bok.mActionMode=6;bok.mOcState=5;
 for(auto& sphere:bok.mSphs_cc)sphere.tg=false;
 before_bokoblin_targets(nullptr,&args,nullptr,nullptr);
 unsigned registered=0;for(auto& sphere:bok.mSphs_cc)registered+=sphere.ChkTgSet();
 assert(registered==2&&bok.mActionMode==6&&bok.mOcState==5);
 // Body hits and each ability use the same native damage entry; ordinary
 // nonlethal hits preserve the prone animation and its original recovery timer.
 for(auto* collider:{&player.mAtCps[0],&player.mAtSph,&s_attacks[1].collider,
                    &s_attacks[2].collider,&s_attacks[3].collider,&s_attacks[4].collider}){
  bok.health=50;bok.mActionMode=6;bok.mOcState=5;bok.field_0x6c0=30;
  bok.mSphs_cc[0].hit=true;bok.mSphs_cc[0].attack=collider;bok.field_0x6cc=0;
  before_enemy_hit(nullptr,&args,nullptr,nullptr);
  bok.health-=3;bok.mActionMode=6;bok.mOcState=0;bok.field_0x6c0=0;
  for(auto& sphere:bok.mSphs_cc)sphere.tg=false;
  after_enemy_hit(nullptr,&args,nullptr,nullptr);
  assert(bok.health==47&&bok.mActionMode==6&&bok.mOcState==5&&bok.field_0x6c0==30&&bok.mSphs_cc[0].tg);
 }
 // Dead, retiring, untracked, paused and normal story enemies stay native.
 for(int reason=0;reason<7;++reason){
  bok.health=50;bok.mActionMode=6;present=playing=true;enemy.retiring=enemy.credited=false;s_run.phase=Phase::Running;
  if(reason==0)bok.health=0;
  if(reason==1)bok.mActionMode=9;
  if(reason==2)enemy.retiring=true;
  if(reason==3)enemy.credited=true;
  if(reason==4)present=false;
  if(reason==5)playing=false;
  if(reason==6)s_run.phase=Phase::Upgrade;
  for(auto& sphere:bok.mSphs_cc)sphere.tg=false;
  before_bokoblin_targets(nullptr,&args,nullptr,nullptr);assert(!bok.mSphs_cc[0].tg);
 }
 present=playing=true;enemy.retiring=enemy.credited=false;s_run.phase=Phase::Running;
 // Rat damage animation may last longer than ten frames. Its native per-frame
 // assignment of 6 must not stop the actual hit timer from reaching zero.
 args={&rat,nullptr};enemy.profile=fpcNm_E_MS_e;rat.health=30;rat.mAction=4;rat.mMode=1;rat.mCooldown1=10;
 for(int expected=9;expected>=0;--expected){
  --rat.mCooldown1;before_rat_recovery(nullptr,&args,nullptr,nullptr);
  rat.mCooldown1=6;after_rat_recovery(nullptr,&args,nullptr,nullptr);assert(rat.mCooldown1==expected);
 }
 rat.mSph1.hit=true;rat.mSph1.attack=&hit;
 before_enemy_hit(nullptr,&args,nullptr,nullptr);rat.health-=3;rat.mMode=0;rat.mCooldown1=10;
 after_enemy_hit(nullptr,&args,nullptr,nullptr);assert(rat.health==27&&rat.mAction==4&&rat.mMode==1&&rat.mCooldown1==10);
 // Lethal/water-death and landing transitions keep their native timer.
 for(int state=0;state<3;++state){
  rat.health=30;rat.mAction=4;rat.mMode=1;rat.mCooldown1=2;
  before_rat_recovery(nullptr,&args,nullptr,nullptr);rat.mCooldown1=6;
  if(state==0)rat.health=0;else if(state==1)rat.mMode=2;else rat.mAction=1;
  after_rat_recovery(nullptr,&args,nullptr,nullptr);assert(rat.mCooldown1==6&&s_ratRecoveries.empty());
 }
 // Light-sword glow is arena-scoped; damage temporarily uses neutral material.
 args={&player,nullptr};s_run.master_sword=5;after_light_sword(nullptr,&args,nullptr,nullptr);assert(player.light);
 player.mAtCps[0].material=dCcD_MTRL_LIGHT;dCcU_AtInfo info{&player.mAtCps[0]};args={&rat,&info};
 before_hit_material(nullptr,&args,nullptr,nullptr);assert(player.mAtCps[0].material==dCcD_MTRL_NONE);
 after_hit_material(nullptr,&args,nullptr,nullptr);assert(player.mAtCps[0].material==dCcD_MTRL_LIGHT&&s_hitMaterials.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+source+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors defense: block chances, direction, action priority, shield bash, stagger exceptions, deaths and light-sword damage passed')

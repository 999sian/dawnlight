"""Automatic attack scheduling pauses for traversal without replacing its pose."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
schedule=core[core.index('void update_weapons('):core.index('bool native_sword_collider(',core.index('void update_weapons('))]
jump=(root/'src/jump_hooks.cpp').read_text()
busy=jump[jump.index('bool jump_abilities_busy_for('):jump.index('bool glide_active_for(')]
fixture=r'''
#include <cassert>
#include <array>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
struct cXyz{float x=0,y=0,z=0;cXyz operator+(cXyz b){return {x+b.x,y+b.y,z+b.z};}};
struct Collider{bool active=true;void OffAtSetBit(){active=false;}void ResetAtHit(){}bool ChkAtShieldHit(){return false;}
 void SetC(cXyz){}void SetR(float){}void SetAtVec(cXyz){}};
struct daAlink_c {
 static constexpr unsigned MODE_PLAYER_FLY=0x70C52;
 struct{bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;
 enum {PROC_WAIT,PROC_CUT_LARGE_JUMP,PROC_CUT_LARGE_JUMP_LAND,PROC_SCREAM_WAIT};int mProcID=PROC_WAIT;
 static constexpr unsigned ERFLG1_NS_SCREAM=1;
 bool scream=false;bool checkEndResetFlg1(unsigned){return scream;}
 bool equipped=true;bool checkItemSwordEquip(){return equipped;}
 unsigned flags=0;bool checkModeFlg(unsigned n){return flags&n;}
 std::array<Collider,3> mAtCps;Collider mAtSph;
};
enum class GaleCharge{Idle,Airborne,Ready};constexpr unsigned kNoGlideActor=~0u;
struct{GaleCharge charge=GaleCharge::Idle;unsigned cucco=kNoGlideActor;bool retiring=false;}s_jumpAbilities;
bool owns=true;bool owns_jump_abilities(daAlink_c*){return owns;}
enum class SwordAction{None,Normal,Spin,Jump};SwordAction s_swordAction=SwordAction::None;
bool s_wavePending=false,s_trackMelee=false;unsigned s_meleeTarget=1,s_comboStep=2,s_comboTimeout=60;
constexpr unsigned kNone=~0u;
std::array<unsigned,20>s_cooldowns;
struct{bool active=false;}s_ranged;
unsigned cancels=0,shots=0,swords=0,shields=0,projectileTicks=0;
void cancel_ranged(){++cancels;s_ranged.active=false;}
void update_ranged(daAlink_c*){++shots;}bool update_shield_attack(daAlink_c*){++shields;return false;}
bool readySword=false,readyRanged=false;
void fire_items(daAlink_c*){++shots;if(readyRanged && s_swordAction==SwordAction::None)s_ranged.active=true;}
void update_sword(daAlink_c*){++swords;if(readySword)s_swordAction=SwordAction::Normal;}
struct Attack{bool active=false,aimPending=false;unsigned weapon=unsigned(Upgrade::Bow),life=100;float radius=1;cXyz position,velocity;Collider collider;
 struct{void Move(){}}status;};std::array<Attack,1>s_attacks;
void update_item_attack(Attack&,daAlink_c*){++projectileTicks;}void stop_attack(Attack& a){a.active=false;}
void update_wave_effects(Attack&,daAlink_c*){}
void update_parallel_pulse(Attack&,daAlink_c*){}
void update_parallel_skills(daAlink_c*){++shields;}
struct dBgS_LinChk{void Set(cXyz*,const cXyz*,daAlink_c*){}};
struct BG{bool LineCross(dBgS_LinChk*){return false;}}bg;BG& dComIfG_Bgsp(){return bg;}
struct CC{void Set(Collider*){}}cc;CC* dComIfG_Ccsp(){return &cc;}
'''
main=r'''
int main(){
 // Normal sword and item releases are called on the same frame.
 {daAlink_c link;readySword=readyRanged=true;s_ranged.active=false;
  update_weapons(&link);assert(swords==1&&s_swordAction==SwordAction::Normal&&s_ranged.active);
  readySword=readyRanged=false;s_ranged.active=false;s_swordAction=SwordAction::None;}

 for(auto proc:{daAlink_c::PROC_CUT_LARGE_JUMP,daAlink_c::PROC_CUT_LARGE_JUMP_LAND}){
  daAlink_c link;link.mProcID=proc;link.mLinkAcch.ground=proc==daAlink_c::PROC_CUT_LARGE_JUMP_LAND;
  s_swordAction=SwordAction::Jump;s_cooldowns.fill(5);shots=swords=shields=0;
  assert(auto_attacks_suspended(&link));update_weapons(&link);
  assert(shots==2&&!swords&&shields==1&&s_cooldowns[0]==4&&s_swordAction==SwordAction::Jump);
  assert(link.mAtCps[0].active); // Preserve only the owned Jump Strike.
  s_jumpAbilities.charge=GaleCharge::Ready;update_weapons(&link);
  assert(s_swordAction==SwordAction::None&&!link.mAtCps[0].active);
  s_jumpAbilities={};link.mProcID=daAlink_c::PROC_WAIT;link.mLinkAcch.ground=true;
  s_swordAction=SwordAction::Jump;update_weapons(&link);
  assert(s_swordAction==SwordAction::None&&swords==1&&s_cooldowns[0]==3);
 }

 for(int context=0;context<9;++context){
  daAlink_c link;s_jumpAbilities={};s_cooldowns.fill(5);shots=swords=shields=projectileTicks=0;
  s_ranged.active=true;s_wavePending=s_trackMelee=true;s_swordAction=SwordAction::Normal;s_attacks[0].active=true;
  if(context==0)link.mLinkAcch.ground=false; // Walking off a ledge / manual jump.
  if(context==1)link.flags=2; // Native auto-jump already initialized before ground refresh.
  if(context==2)s_jumpAbilities.charge=GaleCharge::Airborne; // First Gale jump, including landing.
  if(context==3)s_jumpAbilities.charge=GaleCharge::Ready; // Grounded charge.
  if(context==4)s_jumpAbilities.cucco=7; // Glide requested, carrier still loading.
  if(context==5){s_jumpAbilities.cucco=7;link.mLinkAcch.ground=false;} // Gliding.
  if(context==6)link.flags=0x40; // Other native aerial action.
  if(context==7)link.mProcID=daAlink_c::PROC_SCREAM_WAIT;
  if(context==8)link.scream=true; // Flag precedes native pose initialization.
  assert(auto_attacks_suspended(&link));update_weapons(&link);
  assert(!shots&&!swords&&!shields&&projectileTicks==1&&s_cooldowns[0]==5);
  assert(!s_ranged.active&&!s_wavePending&&!s_trackMelee&&s_swordAction==SwordAction::None);
  for(const auto& collider:link.mAtCps){assert(!collider.active);}assert(!link.mAtSph.active);
  // Resume exactly once after landing/ability cleanup; no stacked cooldown ticks.
  link.scream=false;link.mProcID=daAlink_c::PROC_WAIT;link.flags=0;link.mLinkAcch.ground=true;s_jumpAbilities={};update_weapons(&link);
  assert(shots==2&&swords==1&&shields==1&&s_cooldowns[0]==4&&projectileTicks==2);
 }
 daAlink_c link;link.mLinkAcch.ground=false;s_swordAction=SwordAction::None;
 update_weapons(&link);assert(link.mAtCps[0].active); // Never erase a manual jump attack's collider.
 link.mLinkAcch.ground=true;s_jumpAbilities.charge=GaleCharge::Ready;owns=false;
 assert(!auto_attacks_suspended(&link)); // Another actor/session cannot pause this Link.
 owns=true;s_jumpAbilities.charge=GaleCharge::Idle;s_jumpAbilities.cucco=7;s_jumpAbilities.retiring=true;
 assert(!auto_attacks_suspended(&link));
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 jump_source=(root/'src/survivors_jump_strike.inc').read_text()
 def function(name):
  start=jump_source.index('bool '+name+'(');end=jump_source.index('\n}',start)+2
  return jump_source[start:end]
 cpp.write_text(fixture+busy+function('jump_strike_pose')+function('update_jump_strike')+(root/'src/survivors_combat_pause.inc').read_text()+schedule+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors combat pause: jumps, Gale charge/landing, glide, pending releases, cooldowns and resume passed')

"""Exercise arena awareness hooks against native detection, without faking distances."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1] if len(sys.argv)>1 else root/'dusklight')
def function(path,signature):
    text=path.read_text();start=text.index(signature);brace=text.index('{',start);depth=0
    for end in range(brace,len(text)):
        depth+=(text[end]=='{')-(text[end]=='}')
        if not depth:return text[start:end+1]
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using BOOL=int;using f32=float;using s16=short;using ProfileName=int;
constexpr BOOL TRUE=1;constexpr int fpcNm_E_OC_e=1,fpcNm_E_BA_e=2,fpcNm_E_MS_e=3,fpcNm_E_RD_e=4;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
struct fopAc_ac_c{unsigned id=0;int health=20;float distance=100000,speedF=0;
 struct{struct{float y=0;}pos;}current;struct{s16 y=0;}shape_angle;};
using fopEn_enemy_c=fopAc_ac_c;
struct Player:fopAc_ac_c{bool checkSwimUp(){return true;}}player;
fopAc_ac_c* dComIfGp_getPlayer(int){return &player;}Player* daPy_getPlayerActorClass(){return &player;}
bool blocked=false,paused=false,arena=true;bool dComIfGp_event_runCheck(){return paused;}
float fopAcM_searchPlayerDistance(fopAc_ac_c* a){return a->distance;}
s16 fopAcM_searchPlayerAngleY(fopAc_ac_c*){return 0;}
bool fopAcM_otherBgCheck(fopAc_ac_c*,fopAc_ac_c*){return blocked;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
struct daE_OC_c:fopAc_ac_c{struct{bool link=false;void SetLink(){link=true;}}mAcch;float mPlayerRange=1000,mMoveRange=1000;unsigned field_0x6e3=2;int action=0,mode=7;
 bool searchPlayer();int getActionMode(){return action;}void setActionMode(int a,int m){action=a;mode=m;}};
struct e_ms_class:fopAc_ac_c{float mPlayerDistance=100000;int mAction=0,mMode=7;};
struct e_ba_class{fopEn_enemy_c mEnemy;
 enum{ACT_ROOF,ACT_FIGHT_FLY,ACT_FIGHT,ACT_ATTACK,ACT_RETURN,ACT_FLY,ACT_PATH_FLY,ACT_CHANCE=10,ACT_WOLFBITE=13,ACT_WIND,ACT_APPEAR};
 float mPlayerDistanceXZ=100000,mFightFlyDistance=1000;s16 mPlayerAngleY=0;
 bool mIsDying=false;int mAction=ACT_FLY,mMode=7;};
bool other_bg_check(e_ba_class*,fopAc_ac_c*){return blocked;}
struct e_rd_class{fopEn_enemy_c enemy;float attack_range=1000;int action=0,mode=7,weapon_type=1;};
struct{float plyr_srch_min_radius=300;}l_HIO;
struct Enemy{unsigned id;ProfileName profile;bool initialized=true,retiring=false,credited=false,dormant=false;};
Enemy enemies[]={{1,fpcNm_E_OC_e},{2,fpcNm_E_BA_e},{3,fpcNm_E_MS_e},{4,fpcNm_E_RD_e}};
Enemy* tracked(unsigned id){for(auto& e:enemies)if(e.id==id)return &e;return nullptr;}
bool in_arena(){return arena;}
unsigned s_map=0;
struct ProcessFrame{unsigned id;};std::vector<ProcessFrame>s_processFrames;
struct dBgS_LinChk{bool link=false,cam=false,arrow=false,bomb=false,boom=false;
 void SetLink(){link=true;}bool ChkCam(){return cam;}bool ChkArrow(){return arrow;}bool ChkBomb(){return bomb;}bool ChkBoomerang(){return boom;}};
Run s_run;bool playable(){return arena&&!paused;}
'''
native=(function(sdk/'src/d/actor/d_a_e_oc.cpp','bool daE_OC_c::searchPlayer()')+'\n'+
 function(sdk/'src/d/actor/d_a_e_ba.cpp','static BOOL pl_check(').replace('pl_check(', 'native_bat_check(')+'\n'+
 function(sdk/'src/d/actor/d_a_e_ms.cpp','static int pl_check(').replace('pl_check(', 'native_rat_check('))
main=r'''
bool detect(daE_OC_c& bok){bool result=false;std::any args[]={&bok};
 return before_bokoblin_aggro(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL?result:bok.searchPlayer();}
int detect(e_ms_class& rat,float range){int result=0;std::any args[]={&rat,range};
 return before_rat_aggro(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL?result:native_rat_check(&rat,range);}
BOOL detect(e_ba_class& bat,float range){BOOL result=0;std::any args[]={&bat,range,s16(1)};
 return before_keese_aggro(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL?result:native_bat_check(&bat,range,1);}
int main(){
 s_run.start(1);s_map=1;s_processFrames={{1}};
 dBgS_LinChk query;std::any lineArgs[]={&query};
 after_enemy_navigation_line(nullptr,lineArgs,nullptr,nullptr);assert(query.link);
 // Only the tracked field Bokoblin's generic navigation probes are changed.
 for(unsigned reason=0;reason<9;++reason){query={};arena=true;s_map=1;s_processFrames={{1}};s_run.phase=Phase::Running;
  if(reason==0)arena=false;
  if(reason==1)s_map=2;
  if(reason==2)s_processFrames.clear();
  if(reason==3)s_processFrames={{99}};
  if(reason==4)s_processFrames={{2}};
  if(reason==5)query.cam=true;
  if(reason==6)query.arrow=true;
  if(reason==7)query.bomb=true;
  if(reason==8)s_run.phase=Phase::Upgrade;
  after_enemy_navigation_line(nullptr,lineArgs,nullptr,nullptr);assert(!query.link);
 }
 arena=true;s_map=1;s_run.phase=Phase::Running;s_processFrames.clear();
 daE_OC_c fieldBok;fieldBok.id=1;engage_enemy(&fieldBok,enemies[0]);assert(fieldBok.mAcch.link);
 daE_OC_c storyBok;storyBok.id=99;engage_enemy(&storyBok,enemies[0]);assert(!storyBok.mAcch.link);
 s_map=0;

 s_run.start(1);e_rd_class bulblin;bulblin.enemy.id=4;
 for(int weapon:{1,2}){bulblin.weapon_type=weapon;bulblin.action=0;bulblin.mode=7;
  engage_enemy(&bulblin.enemy,enemies[3]);assert(bulblin.action==(weapon==1?3:5)&&bulblin.mode==0);}
 for(float range:{1000.f,1050.f,500.f}){BOOL result=0;std::any args[]={&bulblin,range,s16(1)};
  assert((before_bulblin_aggro(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL)==(range!=500));
  if(range!=500)assert(result);}
 for(int state:{3,4,5,6,20,21}){bulblin.action=state;engage_enemy(&bulblin.enemy,enemies[3]);assert(bulblin.action==state);}

 s_run.start(1);daE_OC_c bok;bok.id=1;e_ms_class rat;rat.id=3;e_ba_class bat;bat.mEnemy.id=2;bat.mEnemy.current.pos.y=180;
 // Native awareness fails far away. Hooks acquire/retain targets even behind Link or walls.
 for(float distance:{1500.f,3000.f,100000.f}){
  bok.distance=rat.mPlayerDistance=bat.mPlayerDistanceXZ=distance;
  assert(!bok.searchPlayer()&&!native_rat_check(&rat,1000)&&!native_bat_check(&bat,1000,1));
  for(bool wall:{false,true}){blocked=wall;
   assert(detect(bok)&&detect(rat,1000)&&detect(rat,1100));
   assert(detect(bat,1000)&&detect(bat,1050));
   assert(!detect(bat,250)); // Still chase; do not enter hover/attack while far away.
  }
 }
 enemies[0].dormant=true;bok.distance=10;bok.action=0;
 assert(!detect(bok));engage_enemy(&bok,enemies[0]);assert(bok.action==0);
 enemies[0].dormant=false;engage_enemy(&bok,enemies[0]);assert(bok.action==3);bok.distance=100000;
 blocked=false;bat.mPlayerDistanceXZ=200;assert(detect(bat,250));
 blocked=true;assert(!detect(bat,250));blocked=false;
 // Native real distance is untouched. Chase entry never resets active attacks/staggers/death.
 engage_enemy(&bok,enemies[0]);assert(bok.action==3&&bok.mode==0&&!bok.mMoveRange&&!bok.field_0x6e3&&bok.distance==100000);
 engage_enemy(&rat,enemies[2]);assert(rat.mAction==1&&rat.mMode==0&&rat.mPlayerDistance==100000);
 for(int state:{e_ba_class::ACT_ROOF,e_ba_class::ACT_RETURN,e_ba_class::ACT_FLY,e_ba_class::ACT_PATH_FLY}){
  bat.mAction=state;bat.mMode=7;engage_enemy(&bat.mEnemy,enemies[1]);assert(bat.mAction==e_ba_class::ACT_FIGHT_FLY&&bat.mMode==0);
 }
 for(int state:{1,2,3,10,13,14,15}){bat.mAction=state;bat.mMode=7;engage_enemy(&bat.mEnemy,enemies[1]);assert(bat.mAction==state&&bat.mMode==7);}
 for(int state:{1,2,3,4,5,10}){rat.mAction=state;rat.mMode=7;engage_enemy(&rat,enemies[2]);assert(rat.mAction==state&&rat.mMode==7);}
 for(int state:{3,4,5,6,9,10,13,14}){bok.action=state;bok.mode=7;engage_enemy(&bok,enemies[0]);assert(bok.action==state&&bok.mode==7);}
 bat.mIsDying=true;bat.mAction=e_ba_class::ACT_FLY;bat.mPlayerDistanceXZ=100000;
 engage_enemy(&bat.mEnemy,enemies[1]);assert(bat.mAction==e_ba_class::ACT_FLY&&!detect(bat,1000));bat.mIsDying=false;
 // Story actors, menus, retired actors and deaths retain vanilla detection/AI.
 for(int gate=0;gate<7;++gate){
  bok.action=0;bok.mode=7;rat.mAction=0;rat.mMode=7;bat.mAction=e_ba_class::ACT_FLY;bat.mMode=7;
  arena=gate!=0;paused=gate==1;s_run.phase=gate==2?Phase::Upgrade:Phase::Running;
  for(auto& e:enemies){e.initialized=gate!=3;e.retiring=gate==4;e.credited=gate==5;}
  bok.health=rat.health=bat.mEnemy.health=gate==6?0:20;
  assert(!detect(bok)&&!detect(rat,1000)&&!detect(bat,1000));
  engage_enemy(&bok,enemies[0]);engage_enemy(&rat,enemies[2]);engage_enemy(&bat.mEnemy,enemies[1]);
  assert(bok.action==0&&rat.mAction==0&&bat.mAction==e_ba_class::ACT_FLY);
 }
 arena=true;paused=false;s_run.phase=Phase::Running;bok.health=rat.health=bat.mEnemy.health=20;
 bok.id=rat.id=bat.mEnemy.id=99;
 assert(!detect(bok)&&!detect(rat,1000)&&!detect(bat,1000));
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+'void engage_extended_enemy(fopAc_ac_c*,const Enemy&){}\n'+native+(root/'src/survivors_aggro.inc').read_text()+(root/'src/survivors_navigation.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors aggro: unlimited detection, native close attack range, chase entry, recovery/death and mode isolation passed')

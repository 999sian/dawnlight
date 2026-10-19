"""Run options: native damage/movement boundaries, non-compounding camera, UI callbacks."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include "survivors_rules.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
using namespace dawnlight::survivors;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}
 template<class T>T& arg_ref(void* p,int i){return std::any_cast<T&>(static_cast<std::any*>(p)[i]);}}
struct cXyz{float x=0,y=0,z=0;
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}
 cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}
 float abs2()const{return x*x+y*y+z*z;}float abs2(cXyz b)const{return (*this-b).abs2();}
 float abs()const{return std::sqrt(abs2());}};
struct fopAc_ac_c{int id=2,health=20;cXyz speed{2,3,4};struct{cXyz pos;}current,old;};
int fopAcM_GetID(fopAc_ac_c* a){return a->id;}
struct Target{fopAc_ac_c* actor=nullptr;bool ChkTgHit(){return actor;}auto* GetTgHitAc(){return actor;}};
struct daAlink_c:fopAc_ac_c{Target mTgCyls[3];};
struct daE_OC_c:fopAc_ac_c{int mActionMode=3,mOcState=4,animation=0x1e;bool checkBck(int n){return animation==n;}};
struct e_ms_class:fopAc_ac_c{int mAction=1,mMode=2;};
struct e_ba_class:fopAc_ac_c{enum{ACT_FIGHT_FLY=1,ACT_RETURN=4,ACT_FLY=5,ACT_PATH_FLY=6};bool mIsDying=false;float mKnockbackSpeed=0;int mAction=2;};
constexpr int fpcNm_B_TN_e=5,fpcNm_E_YC_e=6,fpcNm_E_HZELDA_e=8,fpcNm_E_KR_e=9;
constexpr int fpcNm_E_OC_e=1,fpcNm_E_MS_e=2,fpcNm_E_BA_e=3,fpcNm_E_RD_e=4;
struct e_rd_class{fopAc_ac_c enemy;int action=3,mode=1,anm=0x32;float jump_z=0,field_0x9f0=0;};
bool arrowActive=false;bool active_enemy_arrow(int id){return arrowActive&&id==7;}
struct Enemy{bool elite=false;bool initialized=true,retiring=false,credited=false,dormant=false;int profile=1;unsigned stagger=0;}enemy;
bool faster_boss(const Enemy& e){return e.elite;}
unsigned enemy_damage_factor(int){return enemy.elite?2:1;}
bool owned=true,active=true,paused=false,transition=false,event=false,aim=false,blocked=false;
Enemy* tracked(int id){return owned&&id==2?&enemy:nullptr;}
void suppress_player_reaction(daAlink_c*){}
bool in_arena(){return active;}bool playable(){return active&&!paused;}bool transitioning(){return transition;}
bool dComIfGp_event_runCheck(){return event;}
void* s_menuDocument=nullptr;void remember_menu_document(bool){}
Records s_records;bool s_storeFailed=false,s_progressSaved=false,s_hubSaveRequested=false;
struct Save{ModResult set_blob(ModContext*,const char*,const void*,size_t){return MOD_OK;}}save;auto* svc_save=&save;
Run s_run;RunOptions s_options;int s_player=1;bool s_restart=false;
enum class HubMenu{None,Talents,Run};unsigned s_hubClosingWindow=0;HubMenu s_hubMenu=HubMenu::Run;
void hub_menu_closed(){} void load_option_config(){} void store_option_config(){}
bool settings_editable(){return s_run.phase==Phase::Ready&&!s_restart;}

struct dBgS_Acch{fopAc_ac_c* actor;bool line=false;auto* getMyAc(){return actor;}void OnLineCheck(){line=true;}};
struct daB_TN_c:fopAc_ac_c{dBgS_Acch mAcch{this};};
struct dBgS_CamLinChk{cXyz cross{0,0,150};cXyz GetCross(){return cross;}};
struct Direction{cXyz v;void Val(cXyz a){v=a;}};
struct dCamera_c{cXyz mEye{0,0,100},mCenter{};Direction mDirection;int mPadID=0;
 static bool isAimActive(){return aim;}bool lineBGCheck(cXyz*,cXyz*,dBgS_CamLinChk*,int){return blocked;}};
void restore_sword_aim_camera(dCamera_c*){}bool update_sword_aim_camera(dCamera_c*){return false;}
void resolve_manual_projectiles(dCamera_c*){}
ModContext* mod_ctx=nullptr;
std::vector<UiControlDesc> controls;
UiWindowHandle s_window=0,s_refreshChoices=0;
bool ui_visible(){return false;}
std::vector<std::string> tabNames;
ModResult build_window(ModContext*,UiWindowHandle,UiElementHandle,UiElementHandle,void*,ModError*){return MOD_OK;}
const char* kResultsStyle="results";
ModResult update_arena_tab(ModContext*,void*,ModError*){return MOD_OK;}
ModResult update_talent_tab(ModContext*,void*,ModError*){return MOD_OK;}
ModResult build_talents(ModContext*,UiWindowHandle,UiElementHandle,UiElementHandle,void*,ModError*){return MOD_OK;}
const char* kTreasureStyle="treasure";const char* kArenaStyle="arena";
ChestReward s_chosenTreasure=ChestReward::Count;
void dismiss_treasure_choice(){}
struct Log{void warn(ModContext*,const char*){assert(false);}}logger;auto* svc_log=&logger;
struct Service{
 ModResult window_push(ModContext*,const UiWindowDesc* desc,UiWindowHandle* handle){
  tabNames.clear();for(size_t i=0;i<desc->tab_count;++i){assert(desc->tabs[i].build);tabNames.push_back(desc->tabs[i].title);}
  *handle=1;return MOD_OK;
 }

 ModResult elem_set_class(ModContext*,UiElementHandle,const char*,bool){return MOD_OK;}
 ModResult pane_add_control(ModContext*,UiElementHandle,const UiControlDesc* c,UiElementHandle*){controls.push_back(*c);return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle,const char*,UiElementHandle*){return MOD_OK;}
}service;auto* svc_ui=&service;
ModResult build_reset_scores(UiElementHandle){return MOD_OK;}
'''
main=r'''
int main(){
 s_run.start(1);s_run.phase=Phase::Ready;open_window();
 std::vector<std::string> expected{"Maps","Difficulty","Settings"};if(debug_enabled)expected.push_back("Debug");assert(tabNames==expected);
 s_window=0;s_hubMenu=HubMenu::None;open_window();assert(!s_window);
 s_hubMenu=HubMenu::Talents;open_window();assert((tabNames==std::vector<std::string>{"Talents","Settings"}));
 s_window=0;s_run.phase=Phase::Upgrade;open_window();assert(tabNames.size()==1&&tabNames[0]=="Choose an upgrade");
 s_run.start(1);s_run.options.enemy_speed=3;
 daE_OC_c bok;bok.old.pos={10,0,10};bok.current.pos={12,3,14};dBgS_Acch acch{&bok};std::any movement[]={&acch};
 before_enemy_speed(nullptr,movement,nullptr,nullptr);assert(bok.current.pos.abs2({16,3,22})==0&&acch.line);
 for(int reason=0;reason<7;++reason){
  active=owned=true;paused=false;enemy={};bok.health=20;bok.mActionMode=3;bok.current.pos={12,3,14};
  if(reason==0)active=false;if(reason==1)owned=false;if(reason==2)paused=true;
  if(reason==3)enemy.dormant=true;if(reason==4)enemy.retiring=true;if(reason==5)bok.health=0;if(reason==6)bok.mActionMode=6;
  before_enemy_speed(nullptr,movement,nullptr,nullptr);assert(bok.current.pos.abs2({12,3,14})==0);
 }
 // Boss movement is 1.5x before correction and composes with run difficulty.
 active=owned=true;paused=false;enemy={};enemy.elite=true;bok.health=20;bok.mActionMode=3;bok.mOcState=4;bok.animation=0x1e;
 for(unsigned difficulty=1;difficulty<=3;++difficulty){s_run.options.enemy_speed=difficulty;bok.current.pos={12,3,14};
  before_enemy_speed(nullptr,movement,nullptr,nullptr);assert(bok.current.pos.abs2({10+3.f*difficulty,3,10+6.f*difficulty})==0);}
 enemy.elite=false;s_run.options.enemy_speed=3;
 // Attack root motion and collision displacement are never multiplied.
 active=owned=true;paused=false;enemy={};bok.health=20;
 for(int action=0;action<=16;++action){
  bok.mActionMode=action;bok.mOcState=4;bok.animation=0x1e;bok.current.pos={100,3,200};
  before_enemy_speed(nullptr,movement,nullptr,nullptr);
  assert(bok.current.pos.abs2(action==3?cXyz{104,3,208}:cXyz{100,3,200})==0);
 }
 bok.mActionMode=3;bok.mOcState=0;bok.animation=0x1c;bok.current.pos={100,3,200};
 before_enemy_speed(nullptr,movement,nullptr,nullptr);assert(bok.current.pos.abs2({100,3,200})==0);
 bok.mActionMode=1;bok.mOcState=1;bok.animation=0x1d;
 before_enemy_speed(nullptr,movement,nullptr,nullptr);assert(bok.current.pos.abs2({104,3,208})==0);
 e_ms_class rat;acch.actor=&rat;enemy.profile=fpcNm_E_MS_e;
 for(int action=0;action<=10;++action)for(int mode=0;mode<=6;++mode){
  rat.mAction=action;rat.mMode=mode;rat.current.pos={100,3,200};
  before_enemy_speed(nullptr,movement,nullptr,nullptr);
  const bool walking=(action==0&&(mode==1||mode==6))||(action==1&&mode==2);
  assert(rat.current.pos.abs2(walking?cXyz{104,3,208}:cXyz{100,3,200})==0);
 }
 e_ba_class bat;acch.actor=&bat;enemy.profile=fpcNm_E_BA_e;
 for(int action=0;action<=15;++action){
  bat.mAction=action;bat.current.pos={100,3,200};
  before_enemy_speed(nullptr,movement,nullptr,nullptr);
  const bool flying=action==1||action==4||action==5||action==6;
  assert(bat.current.pos.abs2(flying?cXyz{104,3,208}:cXyz{100,3,200})==0);
 }
 e_rd_class bulblin;acch.actor=&bulblin.enemy;enemy.profile=fpcNm_E_RD_e;
 for(int action=0;action<=29;++action)for(int mode=0;mode<=5;++mode){
  bulblin.action=action;bulblin.mode=mode;bulblin.enemy.current.pos={100,3,200};
  before_enemy_speed(nullptr,movement,nullptr,nullptr);
  const bool walking=(action==3&&mode>=1&&mode<=3)||(action==5&&(mode==1||mode==2));
  assert(bulblin.enemy.current.pos.abs2(walking?cXyz{104,3,208}:cXyz{100,3,200})==0);
 }
 bulblin.action=3;bulblin.mode=1;bulblin.jump_z=20;bulblin.enemy.current.pos={100,3,200};
 before_enemy_speed(nullptr,movement,nullptr,nullptr);assert(bulblin.enemy.current.pos.x==100);
 enemy={};acch.actor=&bok;
 daAlink_c link;link.id=1;link.mTgCyls[0].actor=&bok;s_run.options.enemy_damage=10;
 std::any damage[]={&link,2,0,1,0};before_enemy_damage(nullptr,damage,nullptr,nullptr);assert(std::any_cast<int>(damage[1])==20);
 for(int reason=0;reason<4;++reason){
  owned=true;damage[1]=2;damage[4]=0;link.id=1;
  if(reason==0)owned=false;if(reason==1)damage[1]=-4;if(reason==2)damage[4]=1;if(reason==3)link.id=5;
  const int initial=std::any_cast<int>(damage[1]);before_enemy_damage(nullptr,damage,nullptr,nullptr);assert(std::any_cast<int>(damage[1])==initial);
 }
 owned=true;link.id=1;fopAc_ac_c arrow;arrow.id=7;link.mTgCyls[0].actor=&arrow;
 arrowActive=true;damage[1]=2;before_enemy_damage(nullptr,damage,nullptr,nullptr);assert(std::any_cast<int>(damage[1])==20);
 arrowActive=false;damage[1]=2;before_enemy_damage(nullptr,damage,nullptr,nullptr);assert(std::any_cast<int>(damage[1])==2);
 dCamera_c camera;std::any cam[]={&camera};
 for(int frame=0;frame<100;++frame){
  before_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==100);
  after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==200);
 }
 before_run_camera(nullptr,cam,nullptr,nullptr);s_run.options.camera_distance=500;
 after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==500);
 before_run_camera(nullptr,cam,nullptr,nullptr);blocked=true;
 after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==145);
 before_run_camera(nullptr,cam,nullptr,nullptr);blocked=false;event=true;
 after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==100);event=false;
 s_run.options.camera_distance=100;after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==100);
 s_run.phase=Phase::Ready;s_options.camera_distance=300;after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==300);
 before_run_camera(nullptr,cam,nullptr,nullptr);active=false;after_run_camera(nullptr,cam,nullptr,nullptr);assert(camera.mEye.z==100);active=true;
 assert(build_difficulty(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 assert(build_run_settings(nullptr,0,1,2,nullptr,nullptr)==MOD_OK&&controls.size()==19);
 assert(controls[2].max==3);
 assert(std::string(controls[8].label)=="Allow Cheats");
 for(unsigned i=0;i<6;++i){auto& c=controls[i];assert(c.kind==UI_CONTROL_NUMBER&&c.get&&c.set);
  UiControlValue value=UI_CONTROL_VALUE_INIT;value.int_value=10000;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.int_value==c.max);
  value.int_value=-10000;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.int_value==c.min);
  s_run.phase=Phase::Running;value.int_value=c.max;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.int_value==c.min);s_run.phase=Phase::Ready;
 }
 for(unsigned i=6;i<9;++i){auto& c=controls[i];assert(c.kind==UI_CONTROL_TOGGLE);
  UiControlValue value=UI_CONTROL_VALUE_INIT;c.get(nullptr,c.user_data,&value);assert(value.bool_value==(i==7));
  value.bool_value=!(i==7);c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.bool_value==!(i==7));
  s_run.phase=Phase::Running;value.bool_value=(i==7);c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.bool_value==!(i==7));
  s_run.phase=Phase::Ready;s_restart=true;value.bool_value=(i==7);c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.bool_value==!(i==7));s_restart=false;
 }
 for(unsigned i=9;i<19;++i){auto& c=controls[i];assert(c.kind==UI_CONTROL_TOGGLE&&c.user_data);
  UiControlValue value=UI_CONTROL_VALUE_INIT;c.get(nullptr,c.user_data,&value);assert(value.bool_value);
  value.bool_value=false;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(!value.bool_value);
  assert(!s_options.manual_aim[i-9]);for(unsigned j=i-8;j<10;++j)assert(s_options.manual_aim[j]);
  value.bool_value=true;s_run.phase=Phase::Running;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(!value.bool_value);
  s_run.phase=Phase::Ready;s_restart=true;value.bool_value=true;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(!value.bool_value);s_restart=false;
 }
 s_run.start(1,s_options);for(bool enabled:s_run.options.manual_aim)assert(!enabled);
 s_options.manual_aim.fill(true);for(bool enabled:s_run.options.manual_aim)assert(!enabled);
 assert(!s_run.options.manual_enabled(Upgrade::Power)&&!s_run.options.manual_enabled(ManualAbility::Count));
 s_run.start(1,s_options);assert(s_run.options.switch_attack_lock&&!s_run.options.gyro_aim);
 s_run.phase=Phase::Ready;
 controls.clear();s_options.debug={4,10,1,1,5};s_run.options.debug=s_options.debug;
 assert(build_debug(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 if(debug_enabled){
  assert(controls.size()==6&&std::string(controls[0].label)=="Minimum Rarity");
  for(unsigned i=0;i<5;++i){auto& c=controls[i];UiControlValue value=UI_CONTROL_VALUE_INIT;
   assert(c.kind==(i==1?UI_CONTROL_NUMBER:UI_CONTROL_SELECT));
   value.int_value=0;c.set(nullptr,c.user_data,&value);c.get(nullptr,c.user_data,&value);assert(value.int_value==(i==1?1:0));
   value.int_value=i==1?10:int64_t(c.option_count-1);c.set(nullptr,c.user_data,&value);
   s_run.phase=Phase::Running;assert(c.is_disabled(nullptr,c.user_data));
   UiControlValue denied=UI_CONTROL_VALUE_INIT;c.set(nullptr,c.user_data,&denied);c.get(nullptr,c.user_data,&denied);assert(value.int_value==denied.int_value);s_run.phase=Phase::Ready;
  }
  assert(s_options.debug.run_minutes==5&&s_options.debug.player_damage==10);
 }else{
  assert(controls.empty()&&s_options.debug.run_minutes==20&&s_options.debug.player_damage==1);
  assert(s_options.debug.minimum_rarity==0&&!s_options.debug.infinite_rerolls&&!s_options.debug.infinite_banishes);
  assert(s_run.options.debug.run_minutes==20&&s_run.options.debug.player_damage==1);
 }

}
'''
core=(root/'src/survivors.cpp').read_text()
window=core[core.index('void open_window() {'):core.index('ModResult activated(')]
ui=(root/'src/survivors_maps_ui.inc').read_text().split('struct RunOptionControl',1)[1]
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+'bool extended_locomotion(fopAc_ac_c*,int){return false;}\n'+(root/'src/survivors_options.inc').read_text()+'struct RunOptionControl'+ui+(root/'src/survivors_debug_ui.inc').read_text()+window+main)
 for enabled in [0,1]:
  subprocess.run([os.environ.get('CXX','c++'),f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors options: native movement/damage scoping, stable camera zoom/collision and UI boundaries passed')

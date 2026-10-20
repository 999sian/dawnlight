"""Production equipment storage, inventory UI, failures and hub input using SDK descriptors."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
runtime=(root/'src/survivors_gear_runtime.inc').read_text()
ui=(root/'src/survivors_gear_ui.inc').read_text()
# Use the host's real validation, so descriptor errors cannot hide behind mocks.
host=(root/'dusklight/src/dusk/mods/svc/ui.cpp').read_text()
a=host.index('constexpr size_t kUiControlStringSetModeSize =')
validator=host[a:host.index('constexpr size_t kUiControlOptionsSize =',a)]
a=host.index('bool valid_color_preset(')
validator+=host[a:host.index('bool copy_list_items(',a)]
fixture=r'''
#include <cassert>
#include <cmath>
#include <climits>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "survivors_rules.hpp"
#include "mods/svc/ui.h"
namespace ui {const char* material_icon(const char*){return "delete";}}
// VALIDATOR
using namespace dawnlight::survivors;
using s16=int16_t;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){} cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}float absXZ()const{return std::hypot(x,z);}};
struct daAlink_c{struct{cXyz pos;}current;bool checkWolf(){return false;}}player;
unsigned fopAcM_GetID(daAlink_c*){return 1;}
struct dBgS_LinChk{void Set(cXyz*,cXyz*,daAlink_c*){}};
bool wallBlocked=false;struct Bg{bool LineCross(dBgS_LinChk*){return wallBlocked;}}bg;Bg& dComIfG_Bgsp(){return bg;}
constexpr int BUTTON_STATUS_OPEN=1,PAD_1=0,PAD_BUTTON_A=1;
void dComIfGp_setDoStatusForce(int,int){}
namespace mDoCPd_c{struct Pad{unsigned mPressedButtonFlags=0;}pad;bool getTrigA(int){return pad.mPressedButtonFlags&PAD_BUTTON_A;}Pad& getCpadInfo(int){return pad;}}
Run s_run;bool s_active=true,s_gameStarted=true,s_loaded=true,s_hub=true,s_restart=false,s_hubDeparture=false,s_progressSaving=false,s_progressSaved=true,s_hubSaveRequested=false;
cXyz chestActorPosition;bool chestActorReady=false;bool survivors_equipment_chest_anchor(cXyz&);
bool modal=false,transition=false,visible=false;unsigned s_player=1;
bool save_modal_active(){return modal;}bool transitioning(){return transition;}bool ui_visible(){return visible;}bool playable(){return !modal&&!transition&&!visible;}
ModContext* mod_ctx=nullptr;
struct Log{void warn(ModContext*,const char*){}}logService;auto* svc_log=&logService;
std::map<std::string,std::vector<uint8_t>> blobs;bool failSave=false;unsigned writes=0;
struct Save{
 ModResult get_blob(ModContext*,const char* key,void* out,size_t* size){auto it=blobs.find(key);if(it==blobs.end())return MOD_UNAVAILABLE;if(*size<it->second.size())return MOD_INVALID_ARGUMENT;std::copy(it->second.begin(),it->second.end(),static_cast<uint8_t*>(out));*size=it->second.size();return MOD_OK;}
 ModResult set_blob(ModContext*,const char* key,const void* data,size_t size){if(failSave)return MOD_UNAVAILABLE;++writes;auto* p=static_cast<const uint8_t*>(data);blobs[key]={p,p+size};return MOD_OK;}
}save;auto* svc_save=&save;
UiService api{};const UiService* svc_ui=&api;
std::map<UiElementHandle,std::string> rml,labels;std::map<UiElementHandle,UiControlDesc> controls;std::map<UiElementHandle,UiElementHandle> parents;
UiElementHandle serial=10;UiWindowClosedFn onClosed=nullptr;bool failPush=false,failDialog=false;
std::array<UiTabDesc,2> tabs{};
std::array<UiDialogAction,2> dialogActions{};
UiDialogActionFn dialogDismiss=nullptr;
UiDialogHandle openDialog=0;std::string dialogBody,dialogTitle;
void activate_tab(unsigned i){controls.clear();rml.clear();labels.clear();parents.clear();assert(tabs[i].build(nullptr,99,1,2,tabs[i].user_data,nullptr)==MOD_OK);}
void dialog_action(unsigned i){const auto h=openDialog;assert(h);auto a=dialogActions[i];if(a.is_disabled&&a.is_disabled(nullptr,a.user_data))return;a.on_pressed(nullptr,h,a.user_data);if(!a.keep_open&&openDialog==h)openDialog=0;}
void dismiss_dialog(){auto h=openDialog;assert(h);dialogDismiss(nullptr,h,nullptr);openDialog=0;}
std::string number(unsigned n){return std::to_string(n);}
std::string treasure_rml_escape(const std::string& s){return s;}
void initialize_ui(){
 api.pane_add_row=[](ModContext*,UiElementHandle p,const UiRowDesc*,UiElementHandle* out){*out=++serial;parents[*out]=p;return MOD_OK;};
 api.pane_add_rml=[](ModContext*,UiElementHandle p,const char* text,UiElementHandle* out){auto id=++serial;rml[id]=text;parents[id]=p;if(out)*out=id;return MOD_OK;};
 api.pane_add_control=[](ModContext*,UiElementHandle p,const UiControlDesc* d,UiElementHandle* out){assert(valid_control_desc(*d));auto id=++serial;controls[id]=*d;labels[id]=d->label;parents[id]=p;if(out)*out=id;return MOD_OK;};
 api.elem_set_class=[](ModContext*,UiElementHandle,const char*,bool){return MOD_OK;};
 api.elem_set_rml=[](ModContext*,UiElementHandle id,const char* text){rml[id]=text;return MOD_OK;};
 api.control_set_tooltip=[](ModContext*,UiElementHandle,const char*){return MOD_OK;};
 api.window_push=[](ModContext*,const UiWindowDesc* d,UiWindowHandle* out){if(failPush)return MOD_UNAVAILABLE;assert(d->tab_count==2);std::copy_n(d->tabs,2,tabs.begin());*out=99;onClosed=d->on_closed;controls.clear();rml.clear();labels.clear();parents.clear();auto result=tabs[0].build(nullptr,*out,1,2,tabs[0].user_data,nullptr);visible=result==MOD_OK;return result;};
 api.dialog_push=[](ModContext*,const UiDialogDesc* d,UiDialogHandle* out){if(failDialog){*out=0;return MOD_UNAVAILABLE;}assert(d->struct_size==sizeof(UiDialogDesc)&&d->action_count==2&&d->on_dismiss);assert(d->actions[0].struct_size==sizeof(UiDialogAction)&&d->actions[1].struct_size==sizeof(UiDialogAction));assert(std::string(d->actions[0].label)=="Cancel"&&std::string(d->actions[1].label)=="Delete");dialogBody=d->body_rml;dialogTitle=d->title;std::copy_n(d->actions,2,dialogActions.begin());dialogDismiss=d->on_dismiss;*out=openDialog=++serial;return MOD_OK;};
 api.dialog_close=[](ModContext*,UiDialogHandle h){assert(h==openDialog);openDialog=0;return MOD_OK;};
 api.dialog_set_body=[](ModContext*,UiDialogHandle h,const char* body){assert(h==openDialog);dialogBody=body;return MOD_OK;};
 api.window_close=[](ModContext*,UiWindowHandle h){visible=false;if(onClosed)onClosed(nullptr,h,nullptr);return MOD_OK;};
}
'''
# SDK uses a named callback typedef; tolerate the exact installed SDK spelling.
fixture=fixture.replace('UiWindowClosedFn onClosed=nullptr;','void (*onClosed)(ModContext*,UiWindowHandle,void*)=nullptr;')
main=r'''
int main(){
 initialize_ui();s_run.phase=Phase::Ready;load_gear();assert(s_gearWritable&&s_gear.weapon==GearInventory::none);
 GearItem sword{GearKind::Sword,2,3,4},tunic{GearKind::Tunic,5,1,0};
 GearInventory next;next.add(sword);next.add(tunic);assert(store_gear(next));
 auto stored=blobs[kGearBlob];s_gear={};load_gear();assert(s_gear==next);
 // Production loader accepts both 92-byte legacy formats with a larger
 // destination buffer and maps the old unequipped sentinel correctly.
 for(uint8_t version:{1,2}) {
  std::vector<uint8_t> legacy(92);std::copy_n(stored.begin(),88,legacy.begin());legacy[4]=version;legacy[5]=legacy[6]=20;
  uint32_t hash=2166136261u;for(unsigned i=0;i<88;++i)hash=(hash^legacy[i])*16777619u;
  for(unsigned i=0;i<4;++i)legacy[88+i]=uint8_t(hash>>(8*i));
  blobs[kGearBlob]=legacy;load_gear();assert(s_gearWritable&&s_gear==next);
  assert(store_gear(s_gear)&&blobs[kGearBlob].size()==172&&blobs[kGearBlob][4]==3);
 }

 blobs[kGearBlob][3]^=1;load_gear();assert(!s_gearWritable&&!store_gear(next));assert(blobs[kGearBlob][3]!=stored[3]);
 blobs[kGearBlob]=stored;load_gear();failSave=true;auto bad=next;bad.erase(0);assert(!store_gear(bad)&&s_gear==next);failSave=false;
 open_gear_window();assert(s_gearWindow&&visible&&controls.size()==21);
 for(unsigned i=0;i<20;++i){assert(s_gearCards[i]&&s_gearButtons[i]);assert(labels[s_gearButtons[i]]=="Slot "+std::to_string(i+1));if(i%5)assert(parents[parents[s_gearButtons[i]]]==parents[parents[s_gearButtons[i-1]]]);}
 select_gear(nullptr,&s_gearSlots[0]);select_gear(nullptr,&s_gearSlots[1]);assert(s_gear.weapon==0&&s_gear.tunic==1);
 assert(rml[s_gearCards[0]].find("Equipped")!=std::string::npos);
 // Real control callbacks model touch/mouse activation, including repeat
 // activation of a selected card. Unequipping keeps the inventory item.
 auto tap=[&](unsigned i){const auto c=controls.at(s_gearButtons[i]);assert(!c.is_disabled(nullptr,c.user_data));c.on_pressed(nullptr,c.user_data);};
 tap(0);assert(s_gear.weapon==40&&s_gear.items[0]==sword&&s_gear.tunic==1);
 assert(rml[s_gearDetails].find("Unequipped")!=std::string::npos);
 tap(0);assert(s_gear.weapon==0);failSave=true;tap(0);assert(s_gear.weapon==0);failSave=false;
 tap(1);assert(s_gear.tunic==40&&s_gear.items[1]==tunic);tap(1);assert(s_gear.tunic==1);
 // Every delete is modal and explicit. Selecting alone, Cancel, Escape/B,
 // dialog failure and failed save must leave both item and equipment intact.
 s_gearBanish=true;auto before=s_gear;auto beforeWrites=writes;tap(0);
 assert(s_gearDeleteDialog&&s_gear==before&&writes==beforeWrites&&dialogBody.find("Hero's Sword")!=std::string::npos&&dialogBody.find("equipped")!=std::string::npos);
 select_gear(nullptr,&s_gearSlots[1]);assert(s_gear==before);dialog_action(0);assert(!s_gearDeleteDialog&&!openDialog&&s_gear==before);
 tap(0);dismiss_dialog();assert(!s_gearDeleteDialog&&s_gear==before);
 failDialog=true;tap(0);assert(!s_gearDeleteDialog&&!openDialog&&s_gear==before);failDialog=false;
 tap(0);failSave=true;dialog_action(1);assert(s_gearDeleteDialog&&openDialog&&s_gear==before&&dialogBody.find("Nothing was deleted")!=std::string::npos);failSave=false;
 dialog_action(1);assert(!s_gearDeleteDialog&&!openDialog&&s_gear.weapon==40&&s_gear.items[0].kind==GearKind::Empty&&s_gear.tunic==1);
 // Lifecycle and stale-slot validation prevent a delayed confirmation from
 // deleting an item after run start, a reload or a window/page change.
 tap(1);s_run.phase=Phase::Running;confirm_delete_gear(nullptr,openDialog,nullptr);assert(s_gear.items[1]==tunic);s_run.phase=Phase::Ready;dismiss_dialog();
 tap(1);s_gear.items[1].first=4;confirm_delete_gear(nullptr,openDialog,nullptr);assert(s_gear.items[1].first==4);s_gear.items[1]=tunic;dismiss_dialog();
 tap(1);auto staleHandle=openDialog;auto callback=dialogActions[1].on_pressed;
 activate_tab(1);assert(!openDialog&&!s_gearDeleteDialog);callback(nullptr,staleHandle,nullptr);assert(s_gear.items[1]==tunic);
 for(unsigned i=0;i<40;++i)assert((bool)s_gearButtons[i]==(i>=20));
 assert(controls.size()==21&&labels[s_gearButtons[20]]=="Slot 21"&&labels[s_gearButtons[39]]=="Slot 40");
 s_gear.items[20]=sword;s_gear.items[39]=tunic;s_gearBanish=false;refresh_gear_ui();
 tap(20);assert(s_gear.weapon==20&&s_gear.tunic==1);tap(20);assert(s_gear.weapon==40&&s_gear.items[20]==sword);tap(20);
 tap(39);assert(s_gear.tunic==39);activate_tab(0);tap(1);assert(s_gear.tunic==1&&s_gear.weapon==20);
 activate_tab(1);assert(rml[s_gearCards[20]].find("Equipped")!=std::string::npos);
 s_gearBanish=true;tap(39);assert(s_gear.items[39]==tunic);dialog_action(1);assert(s_gear.items[39]==GearItem{}&&s_gear.weapon==20&&s_gear.tunic==1);
 auto persisted=s_gear;assert(store_gear(persisted));load_gear();assert(s_gear==persisted);
 activate_tab(0);
 auto saved=s_gear;s_run.phase=Phase::Running;select_gear(nullptr,&s_gearSlots[1]);assert(s_gear==saved);s_run.phase=Phase::Ready;
 s_gearBanish=true;tap(1);auto closeHandle=openDialog;auto closeCallback=dialogActions[1].on_pressed;
 onClosed(nullptr,s_gearWindow,nullptr);visible=false;closeCallback(nullptr,closeHandle,nullptr);assert(!openDialog&&s_gear.items[1]==tunic);assert(!s_gearWindow&&s_hubSaveRequested);
 s_hubSaveRequested=false;failPush=true;open_gear_window();assert(!s_gearWindow);failPush=false;
 chestActorPosition={0,0,100};mDoCPd_c::pad.mPressedButtonFlags=1;
 assert(!hub_gear_interaction(&player)&&!s_gearWindow);chestActorReady=true;
 wallBlocked=true;assert(!hub_gear_interaction(&player)&&!s_gearWindow);wallBlocked=false;
 assert(hub_gear_interaction(&player)&&s_gearWindow&&!mDoCPd_c::pad.mPressedButtonFlags);close_gear_window();
 chestActorPosition={0,0,500};assert(!hub_gear_interaction(&player));
 s_run.start(123);s_run.debug_run=true;auto rng=s_run.random;drop_boss_gear();assert(s_run.random==rng);
 // The explicit override awards equipment in Debug, without changing ranked gates.
 s_run.options.debug.max_equipment_drop_rate=1;
 for(unsigned i=0;i<100;++i){s_gear={};s_gearNoticeTicks=0;drop_boss_gear();assert(s_gearNoticeTicks&&s_gear.items[0].kind!=GearKind::Empty);}
 assert(s_run.earned_meta_xp()==0);
 s_run.options.debug.max_equipment_drop_rate=0;
 unsigned drops=0,swords=0,tunics=0;std::array<unsigned,unsigned(GearKind::Count)> kinds{};
 s_run.debug_run=false;s_run.options.debug.max_equipment_drop_rate=1;for(unsigned i=0;i<30000;++i){s_gear={};s_gearNoticeTicks=0;drop_boss_gear();if(s_gearNoticeTicks){++drops;assert(s_gearNoticeTicks==120);++kinds[unsigned(s_gear.items[0].kind)];if(s_gear.items[0].is_weapon())++swords;else ++tunics;}}
 assert(drops>5600&&drops<6400&&swords>2700&&swords<3300&&tunics>2700&&tunics<3300);
 for(unsigned k=1;k<unsigned(GearKind::Count);++k)assert(kinds[k]>0);
 // Every weapon has its English variant name, family icon and translated
 // numeric values in the actual rendered grid and detail text.
 const std::array<std::pair<GearKind,const char*>,10> named{{
  {GearKind::Sword,"Hero's Sword"},{GearKind::Greatsword,"Greatsword"},
  {GearKind::HerosBow,"Hero's Bow"},{GearKind::SharpshooterBow,"Sharpshooter Bow"},
  {GearKind::StormBoomerang,"Storm Boomerang"},{GearKind::HurricaneBoomerang,"Hurricane Boomerang"},
  {GearKind::BombBag,"Bomb Bag"},{GearKind::SiegeBombs,"Siege Bombs"},
  {GearKind::IronBall,"Iron Ball"},{GearKind::BreakerBall,"Breaker Ball"}}};
 s_run.phase=Phase::Ready;open_gear_window();
 for(const auto& [kind,name]:named) {
  GearItem item{kind,5,5,5};if(kind==GearKind::SharpshooterBow)item.third=2;if(kind==GearKind::HurricaneBoomerang)item.second=2;
  s_gear={};s_gear.add(item);refresh_gear_ui();select_gear(nullptr,&s_gearSlots[0]);
  assert(s_gear.weapon==0&&rml[s_gearCards[0]].find(name)!=std::string::npos);
  assert(rml[s_gearCards[0]].find(std::string("item://item/")+gear_icon(item))!=std::string::npos);
  const auto description=gear_description(item);assert(description.find(name)!=std::string::npos&&description.find("+5 ")!=std::string::npos);
  if(kind==GearKind::SharpshooterBow)assert(description.find("+2 additional pierced targets")!=std::string::npos&&description.find("+25% arrow range")!=std::string::npos);
  if(kind==GearKind::BombBag)assert(description.find("+10% explosion radius")!=std::string::npos);
 }
 close_gear_window();
 for(auto& item:s_gear.items)item=sword;auto full=s_gear;while(true){s_gearNoticeTicks=0;drop_boss_gear();if(s_gearNoticeTicks)break;}
 assert(s_gear==full&&std::string(s_gearNotice)=="Equipment inventory full!");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture.replace('// VALIDATOR',validator)+runtime+'\nbool survivors_equipment_chest_anchor(cXyz& p){p=chestActorPosition;return chestActorReady;}\n'+ui+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors equipment UI: touch unequip, two pages, safe deletion confirmation, native descriptors, grid slots, equip/delete, save rollback, hub input and boss drops passed')

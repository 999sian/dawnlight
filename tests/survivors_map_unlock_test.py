"""Map locks, live preview refresh and transactional debug score reset."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
available=re.search(r'bool map_available\(unsigned map\) \{.*?\n\}',core,re.S).group()
fixture=r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
using namespace dawnlight::survivors;
Records s_records;Run s_run;RunOptions s_options;
bool s_restart=false,s_progressSaving=false,s_hubDeparture=false,s_arenaMeshReady=true;
bool s_storeFailed=false,s_progressSaved=true,s_hubSaveRequested=false,failSave=false;
unsigned s_requestedMap=0;UiWindowHandle s_window=1,s_refreshChoices=0;
ModContext* mod_ctx=nullptr;constexpr const char* kBlob="test";
std::vector<UiControlDesc> controls;
std::unordered_map<UiElementHandle,std::string> text;
std::unordered_map<UiElementHandle,bool> visible;
UiElementHandle nextHandle=1;
struct Service {
 ModResult elem_set_class(ModContext*,UiElementHandle,const char*,bool){return MOD_OK;}
 ModResult elem_set_text(ModContext*,UiElementHandle h,const char* s){text[h]=s;return MOD_OK;}
 ModResult elem_set_rml(ModContext* c,UiElementHandle h,const char* s){return elem_set_text(c,h,s);}
 ModResult elem_set_visible(ModContext*,UiElementHandle h,bool v){visible[h]=v;return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle,const char* s,UiElementHandle* h){if(h){*h=nextHandle++;text[*h]=s;}return MOD_OK;}
 ModResult pane_add_rml(ModContext* c,UiElementHandle p,const char* s,UiElementHandle* h){return pane_add_text(c,p,s,h);}
 ModResult pane_add_row(ModContext*,UiElementHandle,const UiRowDesc*,UiElementHandle* h){*h=nextHandle++;return MOD_OK;}
 ModResult pane_add_control(ModContext*,UiElementHandle,const UiControlDesc* c,UiElementHandle* h){controls.push_back(*c);if(h)*h=nextHandle++;return MOD_OK;}
}ui;auto* svc_ui=&ui;
Records saved;unsigned writes=0;
struct Save {
 ModResult set_blob(ModContext*,const char*,const void* data,size_t size){
  ++writes;if(failSave)return MOD_INVALID_ARGUMENT;
  assert(saved.decode(std::span(static_cast<const uint8_t*>(data),size)));return MOD_OK;
 }
}save;auto* svc_save=&save;
bool settings_editable(){return s_run.phase==Phase::Ready&&!s_restart&&!s_progressSaving&&!s_hubDeparture;}
std::string number(unsigned n){return std::to_string(n);}
std::string treasure_rml_escape(const char* s){return s;}
void start_run(ModContext*,void*){}
void close_window(){s_window=0;}
'''
main=r'''
int main(){
 s_run.phase=Phase::Ready;
 for(unsigned map=0;map<arenas.size();++map){
  controls.clear();s_requestedMap=map;
  assert(build_arena_choices(1,2)==MOD_OK);
  auto start=controls.back();assert(std::string(start.label)=="Start run");
  assert(start.is_disabled(nullptr,nullptr)==(map!=0));
  assert(visible[s_mapLockHint]==(map!=0));
  if(map)assert(text[s_mapLockHint]=="Clear "+std::string(arenas[map-1].name)+" to unlock this map.");
  // A stale debug value cannot unlock maps in a disabled build.
  s_options.debug.unlock_all_maps=true;update_arena_tab(nullptr,nullptr,nullptr);
  assert(start.is_disabled(nullptr,nullptr)==(!debug_enabled&&map!=0));
  assert(visible[s_mapLockHint]==(!debug_enabled&&map!=0));
  s_options.debug.unlock_all_maps=false;update_arena_tab(nullptr,nullptr,nullptr);
  assert(start.is_disabled(nullptr,nullptr)==(map!=0));
  if(map){s_records.cleared_maps=1u<<(map-1);update_arena_tab(nullptr,nullptr,nullptr);}
  assert(!start.is_disabled(nullptr,nullptr)&&!visible[s_mapLockHint]);
  s_progressSaving=true;assert(start.is_disabled(nullptr,nullptr));s_progressSaving=false;
  s_records.cleared_maps=0;
 }
 s_records.high_scores.fill(1234);s_records.total_xp=5000;s_records.buy(Talent::MaxHealth);
 s_records.cleared_maps=7;s_records.runs=10;s_records.wins=3;
 update_arena_tab(nullptr,nullptr,nullptr);assert(text[s_mapPreview].find("1234")!=std::string::npos);
 controls.clear();assert(build_debug(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 const auto original=s_records.encode();
 if(debug_enabled){
  assert(controls.size()==7);auto toggle=controls[5];auto clear=controls[6];
  assert(toggle.kind==UI_CONTROL_TOGGLE&&std::string(toggle.label)=="Unlock all maps");
  UiControlValue value=UI_CONTROL_VALUE_INIT;toggle.get(nullptr,nullptr,&value);assert(!value.bool_value);
  value.bool_value=true;toggle.set(nullptr,nullptr,&value);assert(s_options.debug.unlock_all_maps);
  for(int reason=0;reason<4;++reason){
   if(reason==0)s_restart=true;if(reason==1)s_progressSaving=true;
   if(reason==2)s_hubDeparture=true;if(reason==3)s_run.phase=Phase::Running;
   assert(clear.is_disabled(nullptr,nullptr));clear.on_pressed(nullptr,nullptr);
   value.bool_value=false;toggle.set(nullptr,nullptr,&value);assert(s_options.debug.unlock_all_maps);
   assert(!writes&&s_records.encode()==original);
   s_restart=s_progressSaving=s_hubDeparture=false;s_run.phase=Phase::Ready;
  }
  value.bool_value=false;toggle.set(nullptr,nullptr,&value);
  failSave=true;clear.on_pressed(nullptr,nullptr);
  assert(s_storeFailed&&s_records.encode()==original&&!s_hubSaveRequested);
  failSave=false;clear.on_pressed(nullptr,nullptr);
  assert(!s_storeFailed&&s_hubSaveRequested&&!s_progressSaved&&writes==2);
  auto expected=s_records;expected.high_scores.fill(1234);assert(expected.encode()==original);
  for(auto score:s_records.high_scores)assert(!score);
  assert(saved.encode()==s_records.encode());
  update_arena_tab(nullptr,nullptr,nullptr);
  assert(text[s_mapPreview].find("High score: 0")!=std::string::npos);
 }else{
  assert(controls.empty());clear_high_scores(nullptr,nullptr);
  assert(!writes&&s_records.encode()==original);
 }
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+available+(root/'src/survivors_maps_ui.inc').read_text().split('struct RunOptionControl')[0]+(root/'src/survivors_debug_ui.inc').read_text()+main)
 for enabled in [0,1]:
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-Wno-attributes',f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-I',str(root/'src'),'-I',str(root/'include'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors map locks: preview, debug override, reset persistence and guards passed')

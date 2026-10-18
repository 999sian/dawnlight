"""Native UI callbacks: talent dependencies, bounded spending and save rollback."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <string>
#include <map>
#include <vector>
#include "survivors_items.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
using namespace dawnlight::survivors;
ModContext* mod_ctx=nullptr;Run s_run;Records s_records;
bool s_restart=false,s_storeFailed=false;constexpr char kBlob[]="records";
struct Save{bool fail=false;unsigned writes=0;std::vector<uint8_t> blob;
 ModResult set_blob(ModContext*,const char*,const void* data,size_t size){++writes;if(fail)return MOD_ERROR;
 auto* bytes=static_cast<const uint8_t*>(data);blob.assign(bytes,bytes+size);return MOD_OK;}
}save;auto* svc_save=&save;
struct UI{
 unsigned next=1;std::map<UiElementHandle,std::string> texts;std::vector<UiControlDesc> controls;
 ModResult elem_set_class(ModContext*,UiElementHandle,const char*,bool){return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle,const char* t,UiElementHandle* h){texts[next]=t;if(h)*h=next;++next;return MOD_OK;}
 ModResult elem_set_text(ModContext*,UiElementHandle h,const char* t){assert(texts.contains(h));texts[h]=t;return MOD_OK;}
 ModResult pane_add_control(ModContext*,UiElementHandle,const UiControlDesc* c,UiElementHandle*){controls.push_back(*c);return MOD_OK;}
}ui;auto* svc_ui=&ui;
bool s_resultsStored=false;unsigned s_map=3;
void clear_attacks(){}void retire_enemies(){}void load_records(){}
unsigned dComIfGs_getMaxLifeGauge(){return 12;}void dComIfGs_setLife(unsigned){}
void save_results(ModContext*,void*){}
ModResult button(UiElementHandle,const char*,UiPressedFn){return MOD_OK;}
'''
main=r'''
int main(){
 s_run.start(1);s_run.ticks=30000;finish(false);
 assert(s_resultsStored&&s_records.total_xp==2000&&s_records.high_scores[3]==2000&&s_records.available_points()==1);
 finish(false);finish(true);assert(s_records.runs==1&&s_records.total_xp==2000);
 s_records={};save={};
 s_run.phase=Phase::Ready;s_records.total_xp=meta_threshold(27);
 assert(build_talents(nullptr,0,1,2,nullptr,nullptr)==MOD_OK&&ui.controls.size()==11);
 auto press=[](Talent t){auto& c=ui.controls[unsigned(t)];c.on_pressed(nullptr,c.user_data);};
 auto disabled=[](Talent t){auto& c=ui.controls[unsigned(t)];return c.is_disabled(nullptr,c.user_data);};
 assert(disabled(Talent::SprintSpeed)&&disabled(Talent::Glider)&&disabled(Talent::Gale));
 press(Talent::Gale);assert(save.writes==0&&s_records.available_points()==27);
 save.fail=true;press(Talent::MaxHealth);assert(s_storeFailed&&s_records.available_points()==27&&s_records.talents.spent()==0);
 assert(ui.texts[s_talentSummary].find("Could not store")!=std::string::npos);
 save.fail=false;press(Talent::MaxHealth);assert(!s_storeFailed&&s_records.available_points()==26);
 assert(ui.texts[s_talentSummary].find("Unlock Points: 26")!=std::string::npos);
 assert(ui.texts[s_talentRows[0]].find("Rank 1 / 5")!=std::string::npos);
 Records loaded;assert(loaded.decode(save.blob)&&loaded.talents.rank(Talent::MaxHealth)==1);
 s_run.phase=Phase::Running;press(Talent::Magnet);assert(s_records.available_points()==26);
 s_run.phase=Phase::Ready;s_restart=true;press(Talent::Magnet);assert(s_records.available_points()==26);s_restart=false;
 press(Talent::Sprint);assert(!disabled(Talent::SprintSpeed)&&disabled(Talent::Sprint));
 press(Talent::ManualJump);assert(!disabled(Talent::Glider));press(Talent::Glider);assert(!disabled(Talent::Gale));
 press(Talent::Gale);assert(!disabled(Talent::GaleCharges));
 for(unsigned i=0;i<unsigned(Talent::Count);++i)while(!disabled(Talent(i)))press(Talent(i));
 assert(s_records.available_points()==0&&s_records.talents.spent()==27);
 assert(s_records.talents.rank(Talent::Rerolls)==2&&s_records.talents.rank(Talent::Banishes)==2);
 for(unsigned i=0;i<unsigned(Talent::Count);++i)assert(disabled(Talent(i)));
 assert(loaded.decode(save.blob)&&loaded.encode()==s_records.encode());
 // Rebuilding a tab obtains fresh handles; no stale text writes after navigation.
 ui.texts.clear();ui.controls.clear();assert(build_talents(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 assert(ui.texts[s_talentSummary].find("Unlock Points: 0")!=std::string::npos);
}
'''
core=(root/'src/survivors.cpp').read_text()
finish=core[core.index('void finish(bool won) {'):core.index('#include "survivors_midna.inc"')]
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+finish+(root/'src/survivors_talents.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors talents: native UI purchase, prerequisites, caps, persistence and failed-write rollback passed')

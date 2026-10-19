"""Native UI callbacks: talent dependencies, bounded spending and save rollback."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
bool s_victory=false,s_deathPending=false,s_deathDismiss=false;

void* s_talentDocument=nullptr;void remember_menu_document(bool){}
#include <cassert>
#include <string>
#include <map>
#include <vector>
#include "survivors_items.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
using namespace dawnlight::survivors;
ModContext* mod_ctx=nullptr;Run s_run;Records s_records;
bool s_restart=false,s_storeFailed=false,s_progressSaving=false,s_progressSaved=false;
UiDialogHandle s_talentDialog=0;constexpr char kBlob[]="records";
struct Save{bool fail=false;unsigned writes=0;std::vector<uint8_t> blob;
 ModResult set_blob(ModContext*,const char*,const void* data,size_t size){++writes;if(fail)return MOD_ERROR;
 auto* bytes=static_cast<const uint8_t*>(data);blob.assign(bytes,bytes+size);return MOD_OK;}
}save;auto* svc_save=&save;
struct UI{
 unsigned next=10;unsigned rowCount=0;bool rightHidden=false;UiDialogDesc dialog{};UiDialogAction action{};
 std::string title,body;std::map<UiElementHandle,UiElementHandle> parents;
 ModResult elem_set_visible(ModContext*,UiElementHandle h,bool v){assert(h==2&&!v);rightHidden=true;return MOD_OK;}
 ModResult pane_add_row(ModContext*,UiElementHandle h,const UiRowDesc*,UiElementHandle* out){assert(h==1);*out=next++;++rowCount;return MOD_OK;}
 ModResult dialog_push(ModContext*,const UiDialogDesc* d,UiDialogHandle* out){dialog=*d;action=d->actions[0];title=d->title;body=d->body_rml;*out=99;return MOD_OK;}
std::map<UiElementHandle,std::string> texts;std::vector<UiControlDesc> controls;
 ModResult elem_set_class(ModContext*,UiElementHandle,const char*,bool){return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle parent,const char* t,UiElementHandle* h){parents[next]=parent;texts[next]=t;if(h)*h=next;++next;return MOD_OK;}
 ModResult elem_set_text(ModContext*,UiElementHandle h,const char* t){assert(texts.contains(h));texts[h]=t;return MOD_OK;}
 ModResult pane_add_control(ModContext*,UiElementHandle,const UiControlDesc* c,UiElementHandle* h){controls.push_back(*c);if(h)*h=next++;return MOD_OK;}
}ui;auto* svc_ui=&ui;
bool s_resultsStored=false;unsigned s_map=3;
void clear_attacks(){}void retire_enemies(){}void load_records(){}
unsigned dComIfGs_getMaxLifeGauge(){return 12;}void dComIfGs_setLife(unsigned){}
void save_progress(ModContext*,void*){}
ModResult button(UiElementHandle,const char*,UiPressedFn){return MOD_OK;}
'''
main=r'''
int main(){
 s_progressSaved=true;s_run.start(1);s_run.ticks=30000;finish(false);assert(!s_progressSaved);
 assert(s_resultsStored&&s_records.total_xp==2000&&s_records.high_scores[3]==2000&&s_records.available_points()==1);
 finish(false);finish(true);assert(s_records.runs==1&&s_records.total_xp==2000);
 s_records={};save={};
 s_run.phase=Phase::Ready;s_records.total_xp=meta_threshold(73);
 assert(build_talents(nullptr,0,1,2,nullptr,nullptr)==MOD_OK&&ui.controls.size()==3*unsigned(Talent::Count)+2&&ui.rowCount==unsigned(Talent::Count)&&ui.rightHidden);
 auto press=[](Talent t){auto& c=ui.controls[3*unsigned(t)+2];c.on_pressed(nullptr,c.user_data);};
 auto disabled=[](Talent t){auto& c=ui.controls[3*unsigned(t)+2];return c.is_disabled(nullptr,c.user_data);};
 assert(disabled(Talent::SprintSpeed)&&disabled(Talent::Glider)&&disabled(Talent::Gale));
 press(Talent::Gale);assert(save.writes==0&&s_records.available_points()==73);
 save.fail=true;press(Talent::MaxHealth);assert(s_storeFailed&&s_records.available_points()==73&&s_records.talents.spent()==0);
 assert(ui.texts[s_talentSummary].find("Save failed")!=std::string::npos);
 save.fail=false;press(Talent::MaxHealth);assert(!s_storeFailed&&s_records.available_points()==72);
 assert(ui.texts[s_talentSummary].find("Unlock Points: 72")!=std::string::npos);
 assert(ui.texts[s_talentRows[0]].find("1 / 5")!=std::string::npos);
 Records loaded;assert(loaded.decode(save.blob)&&loaded.talents.rank(Talent::MaxHealth)==1);
 s_run.phase=Phase::Running;press(Talent::Magnet);assert(s_records.available_points()==72);
 s_run.phase=Phase::Ready;s_restart=true;press(Talent::Magnet);assert(s_records.available_points()==72);s_restart=false;
 press(Talent::Sprint);assert(!disabled(Talent::SprintSpeed)&&disabled(Talent::Sprint));
 press(Talent::ManualJump);assert(!disabled(Talent::Glider));press(Talent::Glider);assert(!disabled(Talent::Gale));
 press(Talent::Gale);assert(!disabled(Talent::GaleCharges));
 for(unsigned i=0;i<unsigned(Talent::Count);++i)while(!disabled(Talent(i)))press(Talent(i));
 assert(s_records.available_points()==0&&s_records.talents.spent()==73);
 assert(s_records.talents.rank(Talent::Rerolls)==2&&s_records.talents.rank(Talent::Banishes)==2);
 for(unsigned i=0;i<unsigned(Talent::Count);++i)assert(disabled(Talent(i)));
 assert(loaded.decode(save.blob)&&loaded.encode()==s_records.encode());
 // A perk name opens a modal description and never buys a rank.
 const auto before=s_records.encode();auto& name=ui.controls[0];name.on_pressed(nullptr,name.user_data);
 assert(s_talentDialog==99&&ui.title=="Max Health"&&ui.body.find("Rank 5 / 5")!=std::string::npos);
 assert(s_records.encode()==before);ui.action.on_pressed(nullptr,99,nullptr);assert(!s_talentDialog);
 name.on_pressed(nullptr,name.user_data);ui.dialog.on_dismiss(nullptr,99,nullptr);assert(!s_talentDialog);
 auto minus=[](Talent t){auto& c=ui.controls[3*unsigned(t)+1];c.on_pressed(nullptr,c.user_data);};
 save.fail=true;minus(Talent::ManualJump);assert(s_records.encode()==before&&s_storeFailed);
 save.fail=false;minus(Talent::ManualJump);
 assert(!s_records.talents.rank(Talent::Glider)&&!s_records.talents.rank(Talent::Gale)&&!s_records.talents.rank(Talent::GaleCharges));
 assert(s_records.available_points()==5&&s_records.talents.valid());
 minus(Talent::SprintSpeed);assert(s_records.talents.rank(Talent::SprintSpeed)==1&&s_records.available_points()==6);
 minus(Talent::Sprint);assert(!s_records.talents.rank(Talent::SprintSpeed)&&s_records.available_points()==8);
 const auto partial=s_records.encode();s_progressSaving=true;
 press(Talent::ManualJump);minus(Talent::MaxHealth);reset_talents(nullptr,nullptr);
 assert(s_records.encode()==partial&&disabled(Talent::ManualJump));s_progressSaving=false;
 save.fail=true;reset_talents(nullptr,nullptr);assert(s_records.encode()==partial);
 save.fail=false;s_records.high_scores[2]=12345;const auto xp=s_records.total_xp;
 ui.controls[3*unsigned(Talent::Count)].on_pressed(nullptr,nullptr);
 assert(s_records.available_points()==73&&!s_records.talents.spent()&&s_records.total_xp==xp&&s_records.high_scores[2]==12345);
 assert(loaded.decode(save.blob)&&loaded.encode()==s_records.encode());
 s_progressSaved=true;update_talent_tab(nullptr,nullptr,nullptr);
 assert(ui.texts[s_talentSummary].find("Progress saved")!=std::string::npos);
 s_progressSaving=true;update_talent_tab(nullptr,nullptr,nullptr);
 assert(ui.texts[s_talentSummary].find("Saving...")!=std::string::npos);s_progressSaving=false;
 // Rebuilding a tab obtains fresh handles; no stale text writes after navigation.
 ui.texts.clear();ui.controls.clear();assert(build_talents(nullptr,0,1,2,nullptr,nullptr)==MOD_OK);
 assert(ui.texts[s_talentSummary].find("Unlock Points: 73")!=std::string::npos);
}
'''
core=(root/'src/survivors.cpp').read_text()
finish=core[core.index('void finish(bool won) {'):core.index('#include "survivors_ending.inc"')]
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+finish+(root/'src/survivors_talents.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors talents: native UI list, descriptions, purchase/refund/reset, dependencies and persistence passed')

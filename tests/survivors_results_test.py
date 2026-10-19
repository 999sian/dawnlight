"""Actual result builders: ordering, DPS, escaping, empty runs and navigation."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
#include "mods/svc/ui.h"
#define svc_ui test_ui
using namespace dawnlight::survivors;
Run s_run;Records s_records;unsigned s_map=0;bool s_storeFailed=false;
ModContext* mod_ctx=nullptr;
std::string number(unsigned n){return std::to_string(n);}
std::string treasure_rml_escape(const std::string& in){std::string out;for(char c:in)out+=c=='&'?"&amp;":std::string(1,c);return out;}
unsigned treasure_item(ChestReward,const Run&){return 0x43;}
const char* reward_name(ChestReward,const Run&){return "Ball & Chain";}
void save_results(ModContext*,void*){}void replay(ModContext*,void*){}
std::vector<std::string> texts,classes,labels;std::vector<UiPressedFn> callbacks;
struct Service{
 ModResult elem_set_class(ModContext*,UiElementHandle,const char* s,bool){classes.emplace_back(s);return MOD_OK;}
 ModResult pane_add_rml(ModContext*,UiElementHandle,const char* s,UiElementHandle* h){if(h)*h=4;texts.emplace_back(s);return MOD_OK;}
 ModResult pane_add_text(ModContext*,UiElementHandle,const char* s,UiElementHandle* h){if(h)*h=4;texts.emplace_back(s);return MOD_OK;}
 ModResult pane_add_row(ModContext*,UiElementHandle,const UiRowDesc*,UiElementHandle* row){*row=3;return MOD_OK;}
}ui;auto* svc_ui=&ui;
ModResult button(UiElementHandle,const char* label,UiPressedFn callback){labels.emplace_back(label);callbacks.push_back(callback);return MOD_OK;}
'''
main=r'''
int main(){
 s_run.start(1);s_run.phase=Phase::Victory;s_run.ticks=3600;s_run.kills=200;
 s_run.ranks[unsigned(Upgrade::Bow)]=12;s_run.ranks[unsigned(Upgrade::FireEdge)]=1;
 s_run.damage.dealt[unsigned(DamageSource::Bow)]=900;s_run.damage.dealt[unsigned(DamageSource::FireEdge)]=100;
 assert(build_results(1,2)==MOD_OK&&texts.size()==3&&classes.size()==5);
 auto table=texts[0];assert(table.find("VICTORY")!=std::string::npos&&table.find("02:00")!=std::string::npos);
 assert(table.find("Hero's Bow")<table.find("Sword Attack"));
 assert(table.find("90.0%")!=std::string::npos&&table.find("7.5")!=std::string::npos&&table.find("1,000")!=std::string::npos);
 assert(table.find("Fire Edge")==std::string::npos&&texts[1].find("Power-ups")==std::string::npos);
 assert(texts[1].find("Ball &amp; Chain")!=std::string::npos);
 assert((labels==std::vector<std::string>{"Save records","Return to hub"}));
 assert(callbacks[0]==save_results&&callbacks[1]==replay);
 s_run.dark_link=4;s_run.damage.dealt[unsigned(DamageSource::DarkLink)]=250;
 table=results_damage_rml();assert(table.find("Dark Link")!=std::string::npos&&table.find("1,250")!=std::string::npos);
 s_run.start(2);s_run.phase=Phase::Defeat;table=results_damage_rml();
 assert(table.find("DEFEAT")!=std::string::npos&&table.find("nan")==std::string::npos&&table.find("inf")==std::string::npos);
 assert(table.find("No damage dealt this run.")!=std::string::npos&&table.find("00:00")!=std::string::npos);
 s_storeFailed=true;texts.clear();assert(build_results(1,2)==MOD_OK);
 assert(texts.back().find("could not be stored")!=std::string::npos);
 assert(result_number(1234567890)=="1,234,567,890");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_results.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-I',str(root/'src'),'-I',str(root/'include'),'-I',str(root/'dusklight/sdk/include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Results: damage order, totals, DPS, loadout, empty runs and navigation passed')

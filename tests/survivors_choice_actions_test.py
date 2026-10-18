"""Run-wide reroll/banish actions through production callbacks and the SDK UI ABI."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'dusklight'
fixture = r'''
#include <cassert>
#include <string>
#include <vector>
#include "survivors_items.hpp"
#include "mods/svc/ui.h"
using namespace dawnlight::survivors;
Run s_run;unsigned rupees=500,s_selectedChest=0;
ChestReward s_chosenTreasure=ChestReward::Count;
struct Chest {bool free=false;std::array<ChestReward,3> offers;std::array<Rarity,3> rarities{};};
std::array<Chest,2>s_chests;
std::array<unsigned,3>s_choices{0,1,2};
UiWindowHandle s_window=7,s_refreshChoices=0;
void close_window(){s_window=0;}
unsigned dComIfGs_getRupee(){return rupees;}
void dComIfGs_setRupee(unsigned n){rupees=n;}
int potion_slot(){return 11;}
unsigned treasure_item(ChestReward,const Run&){return 0x43;}
struct Button{std::string label;UiPressedFn pressed;void* data;bool disabled;};
std::vector<Button> buttons;
ModResult row(ModContext*,UiElementHandle,const UiRowDesc* desc,UiElementHandle* out){
 assert(desc->struct_size==sizeof(UiRowDesc)&&!desc->wrap);*out=2;return MOD_OK;
}
ModResult control(ModContext*,UiElementHandle,const UiControlDesc* desc,UiElementHandle* out){
 if(out)*out=3;
 assert(desc->kind==UI_CONTROL_BUTTON);
 buttons.push_back({desc->label,desc->on_pressed,desc->user_data,desc->is_disabled&&desc->is_disabled(nullptr,desc->user_data)});
 return MOD_OK;
}
ModResult css(ModContext*,UiElementHandle,const char*,bool){return MOD_OK;}
std::string status;
ModResult rml(ModContext*,UiElementHandle,const char* text,UiElementHandle* out){status=text;if(out)*out=4;return MOD_OK;}
UiService service{};
extern "C" {ModContext* mod_ctx=nullptr;const UiService* svc_ui=&service;}
'''
main = r'''
void fresh(){s_run.start(10);s_run.phase=Phase::Upgrade;s_run.roll_upgrades();s_window=7;s_refreshChoices=0;
 rupees=500;s_chests={};s_selectedChest=0;s_chosenTreasure=ChestReward::Count;buttons.clear();}
int main(){
 service.pane_add_row=row;service.pane_add_control=control;service.pane_add_rml=rml;service.elem_set_class=css;
 fresh();const auto level=s_run.level,xp=s_run.xp;
 assert(add_choice_header(1)==MOD_OK&&buttons.size()==1);
 assert(buttons[0].label=="Reroll: Free"&&!buttons[0].disabled);
 buttons[0].pressed(nullptr,buttons[0].data);
 assert(s_run.free_rerolls==0&&rupees==500&&s_refreshChoices==7&&s_window==0);
 assert(s_run.phase==Phase::Upgrade&&s_run.level==level&&s_run.xp==xp);
 reroll_choices(nullptr,nullptr);assert(rupees==500); // A stale click cannot charge twice.
 s_window=8;s_run.phase=Phase::TreasureChoice;
 s_chests[0].offers={ChestReward::Bow,ChestReward::Shield,ChestReward::MasterSword};
 rupees=69;buttons.clear();assert(add_choice_header(1)==MOD_OK&&buttons[0].disabled&&buttons[0].label=="Reroll: 50 rupees");
 reroll_choices(nullptr,nullptr);assert(rupees==69&&s_window==8&&s_run.paid_rerolls==0);
 rupees=70;reroll_choices(nullptr,nullptr);
 assert(rupees==20&&s_run.paid_rerolls==1&&s_run.phase==Phase::TreasureChoice&&s_refreshChoices==8);
 s_window=9;s_chests[0].free=true;rupees=100;reroll_choices(nullptr,nullptr);
 assert(rupees==0&&s_run.paid_rerolls==2&&s_run.reroll_cost()==150);
 fresh();s_run.phase=Phase::TreasureChoice;s_run.ranks[unsigned(Upgrade::Bow)]=4;
 s_chests[0].offers={ChestReward::Bow,ChestReward::Shield,ChestReward::MasterSword};
 unsigned bow=0;banish_choice(nullptr,&bow);
 assert(s_chests[0].offers[0]==ChestReward::Count&&s_chests[0].offers[1]==ChestReward::Shield);
 assert(s_run.banishes==0&&s_run.rank(Upgrade::Bow)==4&&s_run.phase==Phase::TreasureChoice);
 assert(!s_run.chest_eligible(ChestReward::Bow)&&!s_run.eligible(Upgrade::Bow));
 s_window=8;s_run.phase=Phase::Upgrade;s_run.roll_upgrades();buttons.clear();
 assert(add_choice_header(1)==MOD_OK&&buttons.size()==1);
 assert(status.find("&#xE14B;</icon>x0")!=std::string::npos);
 fresh();s_run.ranks[unsigned(Upgrade::Power)]=6;s_run.offers.fill(Upgrade::Power);
 unsigned choice=0;banish_choice(nullptr,&choice);
 assert(s_run.rank(Upgrade::Power)==6&&s_run.banishes==0);
 for(auto u:s_run.offers)assert(u==Upgrade::Count);
 s_run.banishes=2;s_run.offers[0]=Upgrade::Magnet;banish_choice(nullptr,&choice);
 assert(s_run.banishes==2&&s_run.offers[0]==Upgrade::Magnet); // Closed window rejects stale clicks.
 s_run.offers.fill(Upgrade::Count);
 buttons.clear();assert(add_choice_actions(1,false)==MOD_OK&&buttons.back().label=="Continue");
 buttons.back().pressed(nullptr,nullptr);assert(s_run.phase==Phase::Running);
 fresh();s_chosenTreasure=ChestReward::Bow;s_run.phase=Phase::TreasureChoice;
 reroll_choices(nullptr,nullptr);assert(rupees==500&&s_run.free_rerolls==1&&s_window==7);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; binary=Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_choice_actions.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes',
                    '-I',str(root/'src'),'-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors choice actions: shared budget, paid rerolls, reserved price, banish and empty choices passed')

"""Build production reward cards through the real SDK UI ABI and inspect activation targets."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'dusklight'
item_header = (sdk / 'include/d/d_item_data.h').read_text()
start = item_header.rfind('enum {', 0, item_header.index('dItemNo_HEART_e'))
item_enum = item_header[start:item_header.index('};', start) + 2]
treasure = (root / 'src/survivors_treasure.inc').read_text().split('// Native DEFAULT_GETITEM')[0]
inventory = (root / 'src/survivors_inventory.inc').read_text()
changes = inventory[inventory.index('std::vector<ItemChange> treasure_changes'):inventory.index('bool prepare_treasure_message')]
actions=(root/'src/survivors_choice_actions.inc').read_text()
header=actions[actions.index('ModResult add_choice_header('):actions.index('ModResult add_choice_actions(')]
source = (root / 'src/survivors_choice_ui.inc').read_text() + header + (root / 'src/survivors_chest_ui.inc').read_text()
fixture = r'''
#include <cassert>
#include <map>
#include <string>
#include "survivors_items.hpp"
#include "mods/svc/ui.h"
using namespace dawnlight::survivors;
using u8=uint8_t;
// ITEM_ENUM
constexpr int SLOT_11=11,SLOT_14=14;
std::array<int,4> bottles;
int dComIfGs_getItem(int slot,bool){return bottles.at(slot-SLOT_11);}
int potion_slot(){for(int i=0;i<4;++i)if(bottles[i]==dItemNo_NONE_e||bottles[i]==dItemNo_EMPTY_BOTTLE_e)return i+11;return -1;}
unsigned potion_capacity(){unsigned count=0;for(auto item:bottles)count+=item==dItemNo_NONE_e||item==dItemNo_EMPTY_BOTTLE_e;return count;}
ModResult add_choice_actions(UiElementHandle,bool){return MOD_OK;}
Run s_run;unsigned rupees=500,selected=99,banished=99;
void reroll_choices(ModContext*,void*){}
void banish_choice(ModContext*,void* data){banished=*static_cast<unsigned*>(data);}
unsigned dComIfGs_getRupee(){return rupees;}
struct Chest {bool free=false;std::array<ChestReward,3> offers;std::array<Rarity,3> rarities{};};
std::array<Chest,2>s_chests;unsigned s_selectedChest=0;
std::array<unsigned,3>s_choices{0,1,2};
void select_treasure(ModContext*,void* data){selected=*static_cast<unsigned*>(data);}
struct Element {
 UiElementHandle parent=0;std::string kind,text,css;bool visible=true;
 UiPressedFn pressed=nullptr;void* data=nullptr;bool disabled=false;
};
std::map<UiElementHandle,Element> elements;UiElementHandle next=3;bool failControl=false;
ModResult add(UiElementHandle parent,const char* kind,const char* text,UiElementHandle* out){
 assert(elements.count(parent));auto id=next++;elements[id]={parent,kind,text,"",true,nullptr,nullptr};
 if(out){*out=id;}return MOD_OK;
}
ModResult row(ModContext*,UiElementHandle parent,const UiRowDesc* desc,UiElementHandle* out){
 assert(desc->struct_size==sizeof(UiRowDesc)&&!desc->wrap);
 return add(parent,"row","",out);
}
ModResult rml(ModContext*,UiElementHandle parent,const char* text,UiElementHandle* out){return add(parent,"rml",text,out);}
ModResult text(ModContext*,UiElementHandle parent,const char* text,UiElementHandle* out){return add(parent,"text",text,out);}
ModResult control(ModContext*,UiElementHandle parent,const UiControlDesc* desc,UiElementHandle* out){
 if(failControl)return MOD_UNAVAILABLE;
 assert(desc->struct_size==sizeof(UiControlDesc)&&desc->kind==UI_CONTROL_BUTTON&&desc->on_pressed);
 auto result=add(parent,"button",desc->label,out);elements.at(*out).pressed=desc->on_pressed;elements.at(*out).data=desc->user_data;elements.at(*out).disabled=desc->is_disabled&&desc->is_disabled(nullptr,desc->user_data);return result;
}
ModResult css(ModContext*,UiElementHandle element,const char* name,bool active){assert(active);elements.at(element).css=name;return MOD_OK;}
ModResult visible(ModContext*,UiElementHandle element,bool value){elements.at(element).visible=value;return MOD_OK;}
UiService service{};
extern "C" {ModContext* mod_ctx=nullptr;const UiService* svc_ui=&service;}
'''
main = r'''
void fresh(){
 elements.clear();elements[1].kind=elements[2].kind="pane";next=3;
 s_run.start(123);s_run.phase=Phase::TreasureChoice;s_selectedChest=0;selected=99;rupees=500;
 bottles.fill(dItemNo_NONE_e);failControl=false;s_chests[0]={};s_chests[0].rarities={};
 s_chests[0].offers={ChestReward::Bow,ChestReward::Shield,ChestReward::MasterSword};
}
int main(){
 (void)kTreasureStyle; // Installed by open_window in-game; this fixture exercises the card builder.
 service.pane_add_row=row;service.pane_add_rml=rml;service.pane_add_text=text;
 service.pane_add_control=control;service.elem_set_class=css;service.elem_set_visible=visible;
 fresh();s_run.shield=1;s_run.master_sword=4;
 const auto rng=s_run.random;
 assert(build_treasure_choices(1,2)==MOD_OK&&!elements.at(2).visible);
 std::vector<UiElementHandle> cards,buttons;std::vector<std::string> texts;
 for(const auto& [id,e]:elements){
   if(e.css=="survivors-treasure-card")cards.push_back(id);
   if(e.css=="survivors-treasure-pick")buttons.push_back(id);
   if(e.kind=="rml"&&e.text.find("survivors-treasure-copy")!=std::string::npos)texts.push_back(e.text);
 }
 assert(cards.size()==3&&buttons.size()==3&&texts.size()==3);
 unsigned bans=0;bool counts=false;UiElementHandle toolbar=0,reroll=0,loadout=0;
 for(const auto& [id,e]:elements){
   if(e.css=="survivors-card-banish"){
     assert(e.parent==cards[bans]&&e.text=="\uE14B");
     selected=99;e.pressed(nullptr,e.data);assert(banished==bans&&selected==99);++bans;
   }
   if(e.css=="survivors-choice-toolbar")toolbar=id;
   if(e.css=="survivors-reroll"){reroll=id;assert(e.text=="Reroll: Free");}
   if(e.css=="survivors-choice-loadout")loadout=id;
   counts|=e.text.find("&#xE5D5;</icon>x1")!=std::string::npos&&e.text.find("&#xE14B;</icon>x1")!=std::string::npos;
 }
 assert(bans==3&&counts&&toolbar&&reroll<loadout&&elements.at(reroll).parent==toolbar&&elements.at(loadout).parent==toolbar);
 assert(toolbar<elements.at(cards[0]).parent); // Toolbar before the choice row.
 const std::string style=kTreasureStyle;
 const auto delta=style.substr(style.find(".survivors-treasure-delta"),140);
 assert(delta.find("white-space: normal")!=std::string::npos&&delta.find("word-break: break-word")!=std::string::npos);
 assert(style.find("z-index: 3")!=std::string::npos);
 assert(elements.at(cards[0]).parent==elements.at(cards[1]).parent&&elements.at(cards[1]).parent==elements.at(cards[2]).parent);
 const std::array<unsigned,3> icons{dItemNo_BOW_e,dItemNo_WOOD_SHIELD_e,dItemNo_LIGHT_SWORD_e};
 for(unsigned i=0;i<3;++i){
   auto& button=elements.at(buttons[i]);assert(button.parent==cards[i]&&button.css=="survivors-treasure-pick");
   char icon[40];std::snprintf(icon,sizeof(icon),"item://item/%02x",icons[i]);
   assert(texts[i].find(icon)!=std::string::npos&&texts[i].find("survivors-treasure-delta")!=std::string::npos);
   // Controller Confirm and touch Click share the native button callback.
   button.pressed(nullptr,button.data);assert(selected==i);
 }
 assert(texts[1].find("5% -&gt; 10%")!=std::string::npos);
 assert(texts[2].find("Light Sword")!=std::string::npos);
 assert(s_run.random==rng&&s_run.shield==1&&s_run.master_sword==4&&s_run.rank(Upgrade::Bow)==0&&rupees==500);
 assert(treasure_rml_escape("<&\">")=="&lt;&amp;&quot;&gt;");
 // Free elite chest uses the same three native controller/touch buttons,
 // even with an empty wallet. Rendering does not commit any reward/payment.
 fresh();s_chests[0].free=true;rupees=0;
 assert(build_treasure_choices(1,2)==MOD_OK);unsigned freeCards=0;bool freeLabel=false;
 for(const auto& [id,e]:elements){
   if(e.css=="survivors-treasure-pick"){++freeCards;e.pressed(nullptr,e.data);assert(selected<3);}
   freeLabel=freeLabel || e.text.find("survivors-equipment")!=std::string::npos;
   assert(e.text.find("20 rupees")==std::string::npos);
 }
 assert(freeCards==3&&freeLabel&&rupees==0&&s_run.chests_opened==0);
 // Every reward/tier uses its real native icon, including final evolutions.
 for(unsigned n=0;n<unsigned(ChestReward::Count);++n){
   fresh();auto reward=static_cast<ChestReward>(n);s_run.shield=6;s_run.master_sword=4;
   Run preview=s_run;preview.phase=Phase::Treasure;unsigned money=500;
   assert(preview.buy_chest(money,reward,true));auto card=treasure_card_rml(reward,preview,Rarity::Common);
   char icon[40];std::snprintf(icon,sizeof(icon),"item://item/%02x",unsigned(treasure_item(reward,preview)));
   assert(card.find(icon)!=std::string::npos&&card.find(reward==ChestReward::OrdonSword ? "Dual Wield" : reward_name(reward,preview))!=std::string::npos);
   if(reward==ChestReward::RedPotion){assert(card.find("Bottles")!=std::string::npos&&card.find("Red potions")!=std::string::npos);}
 }
 // All rarity colors appear on both the name and icon border. Preview is pure.
 for(unsigned rarity=0;rarity<5;++rarity){
   fresh();s_chests[0].rarities.fill(Rarity(rarity));
   const auto rngBefore=s_run.random;assert(build_treasure_choices(1,2)==MOD_OK);
   unsigned cards=0;for(const auto& [id,e]:elements)if(e.kind=="rml"&&e.text.find("survivors-treasure-copy")!=std::string::npos){
     ++cards;const std::string color=rarity_colors[rarity];
     assert(e.text.find("border-color: "+color)!=std::string::npos&&e.text.find("color: "+color)!=std::string::npos);
     assert(e.text.find(std::string(rarity_names[rarity])+" x"+number(rarity+1))!=std::string::npos);
   }
   assert(cards==3&&s_run.random==rngBefore&&s_run.chests_opened==0&&rupees==500);
 }
 for(unsigned count=1;count<=2;++count){
   fresh();for(unsigned i=count;i<3;++i)s_chests[0].offers[i]=ChestReward::Count;
   assert(build_treasure_choices(1,2)==MOD_OK);unsigned actual=0;
   for(const auto& [id,e]:elements)actual+=e.css=="survivors-treasure-pick";
   assert(actual==count);
 }
 fresh();s_run.ranks[unsigned(Upgrade::Bow)]=30;
 assert(build_treasure_choices(1,2)==MOD_OK);unsigned actual=0;
 for(const auto& [id,e]:elements)actual+=e.css=="survivors-treasure-pick";
 assert(actual==2); // Skip a reward capped after a canceled visit.
 // Replacement warnings sit immediately under the name, before rarity/effects.
 for(bool shield:{false,true}){
   fresh();if(shield)s_run.shield=8;else s_run.dual_wield=8;
   const auto reward=shield?ChestReward::OrdonSword:ChestReward::Shield;
   Run preview=s_run;preview.phase=Phase::Treasure;unsigned money=500;
   assert(preview.buy_chest(money,reward));
   const auto card=treasure_card_rml(reward,preview,Rarity::Common);
   const auto warning=card.find(shield?"replaces Shield":"replaces Dual Wield");
   assert(warning!=std::string::npos&&card.find("survivors-treasure-name")<warning&&warning<card.find("survivors-treasure-effect"));
   s_run=preview;
   assert(treasure_card_rml(reward,preview,Rarity::Common).find("replaces ")==std::string::npos);
 }
 for(unsigned equipped=0;equipped<=4;++equipped){
   fresh();for(unsigned i=0;i<equipped;++i)s_run.ranks[unsigned(Upgrade::Bow)+i]=1;
   assert(build_treasure_choices(1,2)==MOD_OK);
   unsigned headers=0;
   for(const auto& [id,e]:elements)if(e.kind=="rml"&&e.text.find("survivors-equipment\"")!=std::string::npos){
     ++headers;unsigned slots=0,icons=0;
     for(size_t pos=0;(pos=e.text.find("survivors-equipment-slot",pos))!=std::string::npos;++pos)++slots;
     for(size_t pos=0;(pos=e.text.find("<img",pos))!=std::string::npos;++pos)++icons;
     assert(slots==4&&icons==equipped);
   }
   assert(headers==1);
 }
 fresh();s_run.red_potion=true; // Empty bottles retain their equipment icon.
 assert(build_treasure_choices(1,2)==MOD_OK);bool potionIcon=false;
 char potionUrl[40];std::snprintf(potionUrl,sizeof(potionUrl),"item://item/%02x",unsigned(dItemNo_RED_BOTTLE_e));
 for(const auto& [id,e]:elements)if(e.text.find("survivors-equipment\"")!=std::string::npos)potionIcon=e.text.find(potionUrl)!=std::string::npos;
 assert(potionIcon);
 fresh();s_run.banishes=0;s_run.free_rerolls=0;
 assert(build_treasure_choices(1,2)==MOD_OK);
 unsigned exhausted=0;
 for(const auto& [id,e]:elements){
   if(e.css=="survivors-card-banish"){assert(e.disabled);++exhausted;}
   if(e.css=="survivors-reroll")assert(e.text=="Reroll: 50 rupees");
 }
 assert(exhausted==3);
 fresh();failControl=true;assert(build_treasure_choices(1,2)==MOD_UNAVAILABLE&&s_run.chests_opened==0);
 fresh();s_selectedChest=2;assert(build_treasure_choices(1,2)==MOD_INVALID_ARGUMENT);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(fixture.replace('// ITEM_ENUM', item_enum) + treasure + changes + source + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-attributes',
                    '-I', str(root/'src'), '-I', str(sdk/'sdk/include'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Survivors chest cards: native icons, deltas, horizontal controls, activation and caps passed')

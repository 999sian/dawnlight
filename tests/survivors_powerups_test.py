"""Exercise level-up cards and the native Collection request bridge with the SDK UI ABI."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'dusklight'
common = (root/'src/survivors_choice_ui.inc').read_text()
source = (root/'src/survivors_powerups.inc').read_text()
# Verify the unused ID and texture index against the pinned game's actual tables.
items = (sdk/'include/d/d_item_data.h').read_text()
assert '/* 0x92 */ dItemNo_NOENTRY_146_e' in items
assert 'dRes_INDEX_ITEMICON_BTI_NI_ITEM_ICON_MAKIMONO_e=0x3D' in (sdk/'assets/GZ2E01/res/Layout/itemicon.h').read_text()
collect = (sdk/'src/d/d_menu_collect.cpp').read_text()
for name, end in [('wait_proc', 'pointerWait'), ('pointerActivateCurrent', 'save_open_init')]:
    start = collect.index('void dMenu_Collect2D_c::'+name+'(')
    body = collect[start:collect.index('\n}', start)]
    assert 'mCursorX == 3 && mCursorY == 3' in body and 'mSubWindowOpenCheck = 5' in body
assert 'field_0x22d[3][3] = isSkillIconVisible();' in collect
core = (root/'src/survivors.cpp').read_text()
a = core.index('void select_upgrade(')
select = core[a:core.index('\nvoid start_run(', a)]
# Compile the host's descriptor validation, not a permissive UI mock: rejected
# descriptors fail the tab builder and cause Dusklight to deactivate the mod.
host_ui = (sdk/'src/dusk/mods/svc/ui.cpp').read_text()
a = host_ui.index('constexpr size_t kUiControlStringSetModeSize =')
validator = host_ui[a:host_ui.index('constexpr size_t kUiControlOptionsSize =', a)]
a = host_ui.index('bool valid_color_preset(')
validator += host_ui[a:host_ui.index('bool copy_list_items(', a)]
fixture = r'''
#include <any>
#include <cassert>
#include <climits>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include "survivors_items.hpp"
#include "mods/svc/ui.h"
// Icon buttons are not used here; the real validator still references lookup.
namespace ui {const char* material_icon(const char*){assert(false);return "";}}
// SDK_VALIDATOR
using namespace dawnlight::survivors;
using u8=uint8_t;
constexpr u8 dItemNo_NOENTRY_146_e=0x92;
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {
template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}
template<class T>T& arg_ref(void* args,int n){return *std::any_cast<T>(&static_cast<std::any*>(args)[n]);}
}
struct dMenu_Collect2D_c{unsigned mSubWindowOpenCheck=0;};
ModResult add_choice_actions(UiElementHandle,bool){return MOD_OK;}
ModResult add_choice_header(UiElementHandle){return MOD_OK;}
void banish_choice(ModContext*,void*){}
Run s_run;
UiWindowHandle s_powerupsWindow=0;
bool s_powerupsRequested=false,arena=true,visible=false,failPush=false;
dMenu_Collect2D_c* s_powerupsMenu=nullptr;
bool in_arena(){return arena;}bool ui_visible(){return visible;}
std::array<unsigned,3>s_choices{0,1,2};
unsigned money=100,lifeMax=15,closed=0,pushed=0;
unsigned dComIfGs_getRupee(){return money;}unsigned dComIfGs_getRupeeMax(){return 1000;}
void dComIfGs_setRupee(unsigned r){money=r;}unsigned dComIfGs_getMaxLife(){return lifeMax;}
void dComIfGs_setMaxLife(unsigned n){lifeMax=n;}
void close_window(){++closed;}
struct Element{UiElementHandle parent=0;std::string kind,text,css,help;UiPressedFn pressed=nullptr;void* data=nullptr;bool visible=true;};
std::map<UiElementHandle,Element> elements;UiElementHandle next=3;
ModResult add(UiElementHandle parent,const char* kind,const char* text,UiElementHandle* out){
 assert(elements.count(parent));const auto id=next++;auto& e=elements[id];e.parent=parent;e.kind=kind;e.text=text;
 if(out){*out=id;}return MOD_OK;
}
ModResult row(ModContext*,UiElementHandle parent,const UiRowDesc* desc,UiElementHandle* out){
 assert(!desc->wrap);return add(parent,"row","",out);
}
ModResult rml(ModContext*,UiElementHandle parent,const char* text,UiElementHandle* out){return add(parent,"rml",text,out);}
ModResult text(ModContext*,UiElementHandle parent,const char* text,UiElementHandle* out){return add(parent,"text",text,out);}
ModResult control(ModContext*,UiElementHandle parent,const UiControlDesc* desc,UiElementHandle* out){
 if(out)*out=0;
 if(!parent||!desc||!valid_control_desc(*desc))return MOD_INVALID_ARGUMENT;
 UiElementHandle id=0;auto result=add(parent,"button",desc->label,&id);
 if(out){*out=id;}auto& e=elements.at(id);e.pressed=desc->on_pressed;e.data=desc->user_data;e.help=desc->help_rml?desc->help_rml:"";
 return result;
}
ModResult css(ModContext*,UiElementHandle id,const char* name,bool active){assert(active);elements.at(id).css=name;return MOD_OK;}
ModResult setVisible(ModContext*,UiElementHandle id,bool value){elements.at(id).visible=value;return MOD_OK;}
UiWindowClosedFn onClosed=nullptr;
ModResult push(ModContext*,const UiWindowDesc* desc,UiWindowHandle* out){
 ++pushed;if(failPush)return MOD_UNAVAILABLE;
 assert(desc->tab_count==1&&std::string(desc->tabs[0].title)=="Power-ups");
 *out=42;onClosed=desc->on_closed;visible=true;
 const auto result=desc->tabs[0].build(nullptr,42,1,2,nullptr,nullptr);
 assert(result==MOD_OK);return result;
}
ModResult close(ModContext*,UiWindowHandle h){assert(h==42);++closed;visible=false;onClosed(nullptr,h,nullptr);return MOD_OK;}
struct Log{void warn(ModContext*,const char*){}}logService;auto* svc_log=&logService;
UiService service{};
extern "C" {ModContext* mod_ctx=nullptr;const UiService* svc_ui=&service;}
'''
main = r'''
void fresh(){
 s_run.start(123);s_powerupsWindow=0;s_powerupsRequested=false;s_powerupsMenu=nullptr;
 arena=true;visible=failPush=false;money=100;lifeMax=15;closed=pushed=0;
 elements.clear();elements[1].kind=elements[2].kind="pane";next=3;
}
int main(){
 (void)kTreasureStyle;
 service.pane_add_row=row;service.pane_add_rml=rml;service.pane_add_text=text;
 service.pane_add_control=control;service.elem_set_class=css;service.elem_set_visible=setVisible;
 service.window_push=push;service.window_close=close;
 // This was the invalid descriptor that returned error 5 in the crash log.
 fresh();UiControlDesc invalid=UI_CONTROL_DESC_INIT;
 invalid.label="Power - Level 2";invalid.help_rml="+2 damage";
 UiElementHandle rejected=99;
 assert(control(nullptr,1,&invalid,&rejected)==MOD_INVALID_ARGUMENT);
 assert(rejected==0&&elements.size()==2);
 fresh();s_run.phase=Phase::Upgrade;s_run.offers={Upgrade::Power,Upgrade::Heart,Upgrade::Wave};
 const auto rng=s_run.random;assert(build_upgrade_choices(1,2)==MOD_OK);
 std::vector<Element*> buttons;unsigned cards=0;
 for(auto& [id,e]:elements){
   if(e.css=="survivors-treasure-card")++cards;
   if((e.kind=="button"&&e.css!="survivors-card-banish"))buttons.push_back(&e);
   if(e.kind=="rml")assert(e.text.find(kPowerupScrollIcon)!=std::string::npos&&e.text.find("survivors-treasure-delta")!=std::string::npos);
 }
 assert(cards==3&&buttons.size()==3&&!elements.at(2).visible&&s_run.random==rng);
 assert(s_run.rank(Upgrade::Power)==0&&lifeMax==15&&money==100);
 buttons[1]->pressed(nullptr,buttons[1]->data);
 assert(s_run.rank(Upgrade::Heart)==1&&s_run.rank(Upgrade::Power)==0&&lifeMax==20&&closed==1);
 buttons[1]->pressed(nullptr,buttons[1]->data);assert(lifeMax==20&&closed==1);
 // All upgrade kinds produce a valid preview and don't mutate the live run.
 for(unsigned i=0;i<unsigned(Upgrade::Count);++i){
   fresh();s_run.phase=Phase::Upgrade;s_run.shield=1;s_run.red_potion=true;s_run.master_sword=1;s_run.ranks[8]=s_run.ranks[9]=s_run.ranks[10]=s_run.ranks[11]=1;
   s_run.offers.fill(Upgrade(i));assert(build_upgrade_choices(1,2)==MOD_OK);
   assert(s_run.phase==Phase::Upgrade&&s_run.rank(Upgrade::Heart)==0&&money==100);
 }
 // Potion healing previews show the actual fractional gain and commit only the rank.
 fresh();s_run.red_potion=true;s_run.phase=Phase::Upgrade;
 s_run.offers={Upgrade::PotionHealing,Upgrade::Count,Upgrade::Count};
 assert(build_upgrade_choices(1,2)==MOD_OK);bool healing=false;
 for(const auto& [id,e]:elements)if(e.kind=="rml")healing|=e.text.find("1.00 hearts per red potion -&gt; 1.25 hearts per red potion")!=std::string::npos;
 assert(healing&&s_run.potion_heal_quarters()==4);
 for(const auto& [id,e]:elements)if((e.kind=="button"&&e.css!="survivors-card-banish"))e.pressed(nullptr,e.data);
 assert(s_run.potion_heal_quarters()==5&&s_run.equipment_count()==1);
 // Critical cards show the exact Luck-adjusted chance and damage multiplier.
 for(auto power:{Upgrade::CritChance,Upgrade::CritDamage}){
   fresh();s_run.phase=Phase::Upgrade;s_run.ranks[unsigned(Upgrade::Luck)]=20;
   s_run.offers={power,Upgrade::Count,Upgrade::Count};
   const auto previewRng=s_run.random;assert(build_upgrade_choices(1,2)==MOD_OK);
   bool delta=false;
   for(const auto& [id,e]:elements)if(e.kind=="rml"){
     const auto expected=power==Upgrade::CritChance?
       "0% base; 0.00% with Luck -&gt; 1% base; 3.00% with Luck":
       "x2.00 critical damage -&gt; x2.05 critical damage";
     delta|=e.text.find(expected)!=std::string::npos;
   }
   assert(delta&&s_run.random==previewRng&&s_run.rank(power)==0);
   for(const auto& [id,e]:elements)if((e.kind=="button"&&e.css!="survivors-card-banish"))e.pressed(nullptr,e.data);
   assert(s_run.rank(power)==1&&s_run.equipment_count()==0);
 }
 // Rarity presentation and the actual activation agree, including overflow.
 for(unsigned r=0;r<5;++r){
   fresh();s_run.phase=Phase::Upgrade;s_run.offers={Upgrade::Luck,Upgrade::Power,Upgrade::Rupees};
   s_run.offer_rarities.fill(Rarity(r));s_run.ranks[unsigned(Upgrade::Power)]=4;
   const auto rngBefore=s_run.random;assert(build_upgrade_choices(1,2)==MOD_OK);
   unsigned copies=0;std::vector<Element*> picks;
   for(auto& [id,e]:elements){
     if((e.kind=="button"&&e.css!="survivors-card-banish"))picks.push_back(&e);
     if(e.kind!="rml")continue;
     ++copies;const std::string color=rarity_colors[r];
     assert(e.text.find("border-color: "+color)!=std::string::npos&&e.text.find("color: "+color)!=std::string::npos);
     assert(e.text.find(std::string(rarity_names[r])+" x"+number(r+1))!=std::string::npos);
   }
   assert(copies==3&&s_run.random==rngBefore&&s_run.rank(Upgrade::Luck)==0);
   picks[0]->pressed(nullptr,picks[0]->data);assert(s_run.luck_percent()==100+10*(r+1));
   fresh();s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Power;s_run.ranks[3]=29;s_run.offer_rarities[0]=Rarity(r);
   select_upgrade(nullptr,&s_choices[0]);assert(s_run.ranks[3]==30&&money==100+20*r);
 }
 // Luck previews and grants agree at the new cap, with surplus paid as rupees.
 fresh();s_run.phase=Phase::Upgrade;s_run.offers.fill(Upgrade::Luck);
 s_run.ranks[unsigned(Upgrade::Luck)]=18;s_run.offer_rarities.fill(Rarity::Legendary);
 assert(build_upgrade_choices(1,2)==MOD_OK);
 bool beforeLuck=false,afterLuck=false;
 for(auto& [id,e]:elements){beforeLuck|=e.text.find("+180% Luck")!=std::string::npos;afterLuck|=e.text.find("+200% Luck")!=std::string::npos;}
 assert(beforeLuck&&afterLuck);
 select_upgrade(nullptr,&s_choices[0]);
 assert(s_run.luck_percent()==300&&!s_run.eligible(Upgrade::Luck)&&money==160);
 // Native controller/touch request is consumed before dMw's fade/subscreen transition.
 fresh();dMenu_Collect2D_c menu,other;std::any args[]{&menu};bool shown=false;
 after_powerup_scroll_visible(nullptr,nullptr,&shown,nullptr);assert(shown);
 menu.mSubWindowOpenCheck=5;after_powerup_collection_action(nullptr,args,nullptr,nullptr);
 assert(menu.mSubWindowOpenCheck==0&&s_powerupsRequested&&s_powerupsMenu==&menu&&pushed==0);
 assert(before_powerup_collection_wait(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 open_powerup_window();assert(s_powerupsWindow==42&&pushed==1);
 open_powerup_window();assert(pushed==1);
 bool empty=false;for(const auto& [id,e]:elements)empty|=e.text.find("No power-ups collected")!=std::string::npos;
 assert(empty);
 close(nullptr,42);assert(!s_powerupsWindow&&!s_powerupsRequested&&!s_powerupsMenu);
 assert(before_powerup_collection_wait(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 // Overview is a read-only list of owned ranks with effective descriptions.
 fresh();s_run.ranks[3]=2;s_run.ranks[8]=5;s_run.ranks[12]=3;
 s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::CritDamage)]=30;
 menu.mSubWindowOpenCheck=5;after_powerup_collection_action(nullptr,args,nullptr,nullptr);
 open_powerup_window();assert(s_powerupsWindow==42&&pushed==1);
 const auto ranks=s_run.ranks;const auto overviewRng=s_run.random;unsigned count=0;
 for(const auto& [id,e]:elements)if((e.kind=="button"&&e.css!="survivors-card-banish")){
   ++count;assert(e.pressed&&!e.help.empty());
   assert(e.help.find("</span><br/>")!=std::string::npos);
   e.pressed(nullptr,e.data);e.pressed(nullptr,e.data);
   if(e.text.find("Hero's Bow")!=std::string::npos)assert(e.help.find("2 arrows/shot")!=std::string::npos&&e.help.find("Fire Lv.3")!=std::string::npos);
   if(e.text.find("Power")!=std::string::npos)assert(e.help.find("+2 damage")!=std::string::npos);
   if(e.text.find("Crit Chance")!=std::string::npos)assert(e.help.find("30.00% with Luck")!=std::string::npos);
   if(e.text.find("Crit Damage")!=std::string::npos)assert(e.help.find("x3.50 critical damage")!=std::string::npos);
 }
 assert(count==5&&s_run.ranks==ranks&&s_run.random==overviewRng);
 assert(money==100&&lifeMax==15&&closed==0&&s_run.phase==Phase::Running);
 close(nullptr,42);assert(!s_powerupsWindow&&!s_powerupsRequested&&!s_powerupsMenu);
 menu.mSubWindowOpenCheck=5;after_powerup_collection_action(nullptr,args,nullptr,nullptr);
 open_powerup_window();assert(s_powerupsWindow==42&&pushed==2);
 close_powerup_window();assert(!visible&&!s_powerupsRequested&&!s_powerupsMenu);
 // Story-mode flags and other collection requests are unaffected.
 fresh();arena=false;shown=false;after_powerup_scroll_visible(nullptr,nullptr,&shown,nullptr);assert(!shown);
 menu.mSubWindowOpenCheck=5;after_powerup_collection_action(nullptr,args,nullptr,nullptr);assert(menu.mSubWindowOpenCheck==5&&!s_powerupsRequested);
 arena=true;for(unsigned request:{1u,2u,3u,4u,6u}){menu.mSubWindowOpenCheck=request;after_powerup_collection_action(nullptr,args,nullptr,nullptr);assert(menu.mSubWindowOpenCheck==request&&!s_powerupsRequested);}
 // Owner destruction/reset closes only our window. Open failures leave the menu usable.
 menu.mSubWindowOpenCheck=5;after_powerup_collection_action(nullptr,args,nullptr,nullptr);open_powerup_window();
 std::any otherArgs[]{&other};before_powerup_collection_delete(nullptr,otherArgs,nullptr,nullptr);assert(s_powerupsWindow);
 before_powerup_collection_delete(nullptr,args,nullptr,nullptr);assert(!s_powerupsWindow&&!s_powerupsRequested&&!s_powerupsMenu);
 menu.mSubWindowOpenCheck=5;after_powerup_collection_action(nullptr,args,nullptr,nullptr);failPush=true;open_powerup_window();assert(!s_powerupsRequested&&!s_powerupsMenu);
 // The native texture hook targets only our unused UI ID during these windows.
 std::array<std::any,11> iconArgs;iconArgs[1]=u8(0x92);iconArgs[10]=-1;
 fresh();before_powerup_icon(nullptr,iconArgs.data(),nullptr,nullptr);assert(std::any_cast<int>(iconArgs[10])==-1);
 s_run.phase=Phase::Upgrade;before_powerup_icon(nullptr,iconArgs.data(),nullptr,nullptr);assert(std::any_cast<int>(iconArgs[10])==0x3d);
 iconArgs[1]=u8(0x43);iconArgs[10]=-1;before_powerup_icon(nullptr,iconArgs.data(),nullptr,nullptr);assert(std::any_cast<int>(iconArgs[10])==-1);
 iconArgs[1]=u8(0x92);arena=false;before_powerup_icon(nullptr,iconArgs.data(),nullptr,nullptr);assert(std::any_cast<int>(iconArgs[10])==-1);
 arena=true;s_run.phase=Phase::Running;s_powerupsRequested=true;before_powerup_icon(nullptr,iconArgs.data(),nullptr,nullptr);assert(std::any_cast<int>(iconArgs[10])==0x3d);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp)/'test.cpp'
    binary = Path(tmp)/'test'
    cpp.write_text(fixture.replace('// SDK_VALIDATOR', validator) + select + common + source + main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes',
                    '-I',str(root/'src'),'-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors power-ups: level-up cards, scroll mapping, native requests, overview, isolation and cleanup passed')

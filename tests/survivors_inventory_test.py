"""Run inventory guards and reward text through the real SDK message builder."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'dusklight'
source = (root / 'src/survivors_inventory.inc').read_text()
# Preserve the real enum type: u8 stand-ins hide Clang's narrowing diagnostics.
item_header = (sdk / 'include/d/d_item_data.h').read_text()
enum_start = item_header.rfind('enum {', 0, item_header.index('dItemNo_HEART_e'))
item_enum = item_header[enum_start:item_header.index('};', enum_start) + 2]
fixture = r'''
#include <cassert>
#include <any>
#include <map>
#include <memory>
#include <string>
#include "survivors_items.hpp"
#include "mods/svc/flow.hpp"
using namespace dawnlight::survivors;
using u8=uint8_t;
using u32=uint32_t;
constexpr int SLOT_0=0,SLOT_4=4,SLOT_6=6,SLOT_11=11,SLOT_14=14,SLOT_15=15,SLOT_20=20,SLOT_21=21,SLOT_22=22,MAX_ITEM_SLOTS=24;
// ITEM_ENUM
std::array<u8,24> inventory;
u8 dComIfGs_getItem(int slot,bool){return slot<24?inventory[slot]:u8(dItemNo_NONE_e);}
void dComIfGs_setItem(int slot,u8 item){inventory[slot]=item;}
bool arena=true;bool in_arena(){return arena;}
Run s_run;
u8 shield_item(const Run& r){return r.shield_tier()==2?dItemNo_HYLIA_SHIELD_e:r.shield_tier()==1?dItemNo_SHIELD_e:dItemNo_WOOD_SHIELD_e;}
struct dSv_player_status_a_c{};
struct Player{dSv_player_status_a_c status;auto& getPlayerStatusA(){return status;}};
struct Info{Player player;auto& getPlayer(){return player;}};
struct{Info info;}g_dComIfG_gameInfo;
struct dMenu_Ring_c{bool mPlayerIsWolf=false;unsigned mCurrentSlot=0;std::array<u8,24>mItemSlots{};};
struct dMenu_ItemExplain_c{u32 field_0xcc=0,field_0xc8=0,field_0xc4=0,field_0xd0=0;u8 field_0xde=1,field_0xdf=10;unsigned numberUpdates=0;void setNumber(){++numberUpdates;}};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
bool treasureOwned=true;bool owns_treasure_event(){return treasureOwned;}
// TREASURE_ITEM
struct Message{unsigned id;uint8_t language;MessageOverrideFn fn;void* data;};
std::map<MessageOverrideHandle,Message> messages;MessageOverrideHandle nextId=1;bool failMessage=false;
ModResult override_message_fn(ModContext*,uint16_t group,uint16_t id,uint8_t language,MessageOverrideFn fn,void* data,MessageOverrideHandle* handle){
 assert(group==0&&id<5000);if(failMessage)return MOD_UNAVAILABLE;
 *handle=nextId++;messages[*handle]={id,language,fn,data};return MOD_OK;
}
ModResult remove_override(ModContext*,MessageOverrideHandle id){messages.erase(id);return MOD_OK;}
MessageService messageService{.override_message_fn=override_message_fn,.remove_override=remove_override};
extern "C"{ModContext* mod_ctx=nullptr;const MessageService* svc_message=&messageService;const FlowService* svc_flow=nullptr;}
'''
main = r'''
std::vector<uint8_t> bytes_for(MessageId id,MessageLanguage language=MESSAGE_LANGUAGE_ENGLISH){
 for(auto it=messages.rbegin();it!=messages.rend();++it){auto& m=it->second;if(m.id!=id||m.language!=language)continue;
  MessageOverrideContext context{0,id,uint8_t(language),nullptr,0};MessageTextData text{};
  if(m.fn(nullptr,&context,&text,m.data)){assert(text.text_size>1&&text.text[text.text_size-1]==0);return {text.text,text.text+text.text_size};}
 }return {0};
}
std::string plain(MessageId id){std::string text;const auto& bytes=bytes_for(id);for(size_t i=0;i+1<bytes.size();){if(bytes[i]==0x1a)i+=bytes[i+1];else text+=char(bytes[i++]);}return text;}
std::vector<std::string> red_segments(MessageId id){
 std::vector<std::string> out;bool red=false;const auto& bytes=bytes_for(id);
 for(size_t i=0;i+1<bytes.size();){if(bytes[i]==0x1a){if(bytes[i+1]==6&&bytes[i+2]==255&&bytes[i+3]==0&&bytes[i+4]==0){red=bytes[i+5]==MESSAGE_COLOR_RED;if(red)out.emplace_back();}i+=bytes[i+1];}else{if(red)out.back()+=char(bytes[i]);++i;}}
 return out;
}
int main(){
 inventory.fill(dItemNo_NONE_e);s_run.start(1);
 Run before=s_run,after=before;before.shield=1;after.shield=2;
 assert(prepare_treasure_message(ChestReward::Shield,before,after));
 assert(plain(treasure_message_id())=="Wooden Shield\nBlock chance: 5% -> 10%");
 assert(red_segments(treasure_message_id())==std::vector<std::string>{"5% -> 10%"});
 const auto previous=treasure_message_id();failMessage=true;
 assert(!prepare_treasure_message(ChestReward::Bow,before,after)&&treasure_message_id()==previous);failMessage=false;
 // Every reward level reports only values changed by that reward, including
 // damage/rate with already collected global upgrades and Fire Arrows.
 for(auto reward:{ChestReward::Bow,ChestReward::Boomerang,ChestReward::Bombs,ChestReward::Ball})for(unsigned level=0;level<5;++level){
  before=s_run;before.ranks[3]=2;before.ranks[4]=3;before.ranks[5]=4;before.ranks[12]=2;
  auto weapon=chest_weapon(reward);before.ranks[unsigned(weapon)]=level;after=before;++after.ranks[unsigned(weapon)];
  assert(prepare_treasure_message(reward,before,after));const auto changes=reward_changes(reward,before,after);
  auto red=red_segments(treasure_message_id());assert(red.size()==changes.size());
  for(unsigned i=0;i<changes.size();++i)assert(red[i]==changes[i].before+" -> "+changes[i].after&&changes[i].before!=changes[i].after);
  if(reward==ChestReward::Bow&&level==2){auto text=plain(treasure_message_id());assert(text.find("Arrows: 1 -> 2")!=std::string::npos);assert(text.find("Damage: 10 -> 11")!=std::string::npos);}
 }
 before=s_run;before.master_sword=4;before.ranks[0]=3;after=before;after.master_sword=5;
 assert(prepare_treasure_message(ChestReward::MasterSword,before,after));
 assert(plain(treasure_message_id()).find("Light Sword\nSword bonus: +6 -> +7") == 0);
 assert(prepare_treasure_message(ChestReward::RedPotion,s_run,s_run));
 assert(plain(treasure_message_id()).find("Bottles: 0 -> 1")!=std::string::npos);
 inventory[11]=inventory[12]=inventory[13]=dItemNo_RED_BOTTLE_e;inventory[14]=dItemNo_EMPTY_BOTTLE_e;
 assert(prepare_treasure_message(ChestReward::RedPotion,s_run,s_run));
 assert(plain(treasure_message_id())=="Red Potion\nRed potions: 3 -> 4");
 auto bottles=inventory;sync_survivors_inventory();assert(inventory==bottles);
 s_run.ranks[8]=s_run.ranks[9]=s_run.ranks[10]=s_run.ranks[11]=1;s_run.master_sword=5;s_run.shield=7;s_run.dual_wield=true;
 sync_survivors_inventory();
 assert(inventory[4]==dItemNo_BOW_e&&inventory[0]==dItemNo_BOOMERANG_e&&inventory[15]==dItemNo_NORMAL_BOMB_e&&inventory[6]==dItemNo_IRONBALL_e);
 assert(inventory[20]==dItemNo_LIGHT_SWORD_e&&inventory[21]==dItemNo_SWORD_e&&inventory[22]==dItemNo_HYLIA_SHIELD_e);
 for(int i=11;i<=14;++i)assert(inventory[i]==bottles[i]);
 auto* status=&g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA();
 for(int button=0;button<4;++button)for(unsigned slot=0;slot<256;++slot){
  std::any args[]={status,button,u8(slot)};const bool allowed=slot==255||(slot>=11&&slot<=14);
  assert((before_item_binding(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE)==allowed);
  assert((before_item_mix(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE)==(slot==255));
 }
 dSv_player_status_a_c foreign;std::any foreignArgs[]={&foreign,0,u8(4)};
 assert(before_item_binding(nullptr,foreignArgs,nullptr,nullptr)==HOOK_CONTINUE);
 dMenu_Ring_c ring;std::any ringArgs[]={&ring};ring.mItemSlots[0]=4;
 assert(before_ring_assignment(nullptr,ringArgs,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 ring.mItemSlots[0]=11;assert(before_ring_assignment(nullptr,ringArgs,nullptr,nullptr)==HOOK_CONTINUE);
 inventory[11]=dItemNo_NONE_e;assert(!survivors_slot_assignable(11));
 arena=false;ring.mItemSlots[0]=4;assert(before_ring_assignment(nullptr,ringArgs,nullptr,nullptr)==HOOK_CONTINUE);
 std::any bindArgs[]={status,0,u8(4)};assert(before_item_binding(nullptr,bindArgs,nullptr,nullptr)==HOOK_CONTINUE);arena=true;
 dMenu_ItemExplain_c explain;explain.field_0xcc=dItemNo_BOW_e+0x165;explain.field_0xc8=explain.field_0xc4=dItemNo_BOW_e+0x265;u8 opened=1;std::any explainArgs[]={&explain,u8(4)};
 after_item_explain(nullptr,explainArgs,&opened,nullptr);assert(explain.numberUpdates==1&&explain.field_0xdf==0);
 assert(plain(explain.field_0xcc)=="Hero's Bow"&&plain(explain.field_0xc8).find("Fire Lv.0")!=std::string::npos);
 const auto oldDescription=explain.field_0xc8;s_run.ranks[12]=5;s_run.ranks[8]=5;
 after_item_explain(nullptr,explainArgs,&opened,nullptr);
 assert(explain.field_0xc8==oldDescription&&explain.field_0xd0==~0u);
 assert(plain(explain.field_0xc8).find("3 arrows/shot")!=std::string::npos&&plain(explain.field_0xc8).find("Fire Lv.5 (+10 dmg)")!=std::string::npos);
 opened=0;const auto id=explain.field_0xc8;after_item_explain(nullptr,explainArgs,&opened,nullptr);assert(explain.field_0xc8==id);
 for(unsigned rank=0;rank<=5;++rank){
  Run run;run.ranks.fill(rank);run.master_sword=rank;run.shield=rank*2;
  for(unsigned reward=0;reward<unsigned(ChestReward::Count);++reward){
   const auto text=item_description(ChestReward(reward),run);assert(text.size()<480&&std::count(text.begin(),text.end(),'\n')==3);
   size_t start=0,end;do{end=text.find('\n',start);assert((end==std::string::npos?text.size():end)-start<=54);start=end+1;}while(end!=std::string::npos);
  }
 }
 assert(messages.size()==18); // Six languages per reward/name/description, never accumulated.
 // Native IDs work with the original global-archive selection and item-icon code.
 for(unsigned shield:{1u,4u,7u})for(unsigned master:{1u,5u})for(unsigned reward=0;reward<unsigned(ChestReward::Count);++reward){
  after=s_run;after.shield=shield;after.master_sword=master;
  assert(prepare_treasure_message(ChestReward(reward),s_run,after));
  assert(native_icon(treasure_message_id())==treasure_item(ChestReward(reward),after));
  for(auto language:{MESSAGE_LANGUAGE_ENGLISH,MESSAGE_LANGUAGE_GERMAN,MESSAGE_LANGUAGE_JAPANESE})assert(bytes_for(treasure_message_id(),language).size()>1);
 }
 assert(native_icon(0x8123)==dItemNo_BOW_e); // Reproduces the previous wrong-icon fallback.
 treasureOwned=false;assert(plain(treasure_message_id()).empty());treasureOwned=true;
 arena=false;assert(plain(explain.field_0xc8).empty());assert(plain(treasure_message_id()).empty());arena=true;
 s_treasureMessage.reset();s_itemNameMessage.reset();s_itemDescriptionMessage.reset();assert(messages.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, binary = Path(tmp)/'test.cpp', Path(tmp)/'test'
    treasure = (root/'src/survivors_treasure.inc').read_text()
    reward_item = treasure[treasure.index('u8 treasure_item('):treasure.index('// Native DEFAULT_GETITEM')]
    native = (sdk/'src/d/d_msg_scrn_item.cpp').read_text()
    icon = native[native.index('    int iVar6 ='):native.index('    for (int i = 0; i < 3; i++)')]
    icon = icon.replace('dMsgObject_getMessageID()', 'id').replace('isOugiID()', 'false').replace("*dComIfGp_getNextStageName() != 'T'", 'false')
    icon_function = '#define JUT_WARN(...)\nint native_icon(int id){int mItemIndex=0;\n'+icon+'return mItemIndex;}\n'
    assert 'if (param_1 > 5000)' in (sdk/'src/d/d_msg_string_base.cpp').read_text()
    cpp.write_text(fixture.replace('// ITEM_ENUM', item_enum).replace('// TREASURE_ITEM', reward_item) + source + icon_function + main)
    subprocess.run([os.environ.get('CXX', 'c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-missing-field-initializers',
                    '-I',str(root/'src'),'-I',str(sdk/'sdk/include'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors inventory: reward deltas/red tags, message lifetime, inventory, binding guards and live descriptions passed')

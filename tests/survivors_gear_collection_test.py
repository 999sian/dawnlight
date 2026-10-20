"""Native Collection cell ownership, input interception, text and texture lifetime."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
native=(root/'dusklight/src/d/d_menu_collect.cpp').read_text()
assert 'field_0x22d[1][4] = dComIfGs_getPohSpiritNum();' in native
assert 'mCursorX == 3 && mCursorY == 4' in native and 'mSubWindowOpenCheck = 3;' in native
source=(root/'src/survivors_gear_collection.inc').read_text()
# MULTI_CHAR is a host multi-character tag helper, equivalent here.
source=re.sub(r"MULTI_CHAR\('([^']+)'\)",lambda m:'tag("'+m[1]+'")',source)
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u8=uint8_t;using u64=uint64_t;using Mtx=int;
u64 tag(const char* s){u64 n=0;while(*s)n=n*256+*s++;return n;}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}}
struct J2DPane{bool visible=true;int type=0;void show(){visible=true;}void hide(){visible=false;}bool isVisible(){return visible;}int getTypeID(){return type;}};
struct J2DTextBox:J2DPane{char text[512]{};J2DTextBox(){type=19;}struct Buffer{char* buffer;size_t size;};Buffer getStringPtr(){return{text,sizeof(text)};}};
struct Screen{std::map<u64,J2DPane*> panes;J2DPane* search(u64 key){return panes.contains(key)?panes[key]:nullptr;}};
struct Manager{struct Vec{float x,y;};Vec getGlobalVtx(J2DPane*,Mtx*,int i,bool,int){return i?Vec{48,48}:Vec{0,0};}u8 getAlpha(){return 255;}}manager;
struct dMenu_Collect2D_c{Screen* mpScreen=nullptr;Manager* mpLinkPm=&manager;u8 field_0x22d[7][6]{};unsigned mCursorX=1,mCursorY=4,mItemNameString=123,mSubWindowOpenCheck=0;void setAButtonString(unsigned n){assert(!n);}};
bool arena=true;bool in_arena(){return arena;}GearInventory s_gear;
const char* gear_name(GearItem g){return g.kind==GearKind::Sword?"Sword":"Tunic";}
std::string gear_description(GearItem g){return "Stats "+std::to_string(g.first);}
struct Graf{void setup2D(){}}graf;Graf* dComIfGp_getCurrentGrafPort(){return &graf;}
struct ResTIMG{}tex;bool textureAvailable=true;unsigned lookups=0,draws=0;
struct Archive{void* getIdxResource(unsigned index){++lookups;assert(index==0x70||index==0x6b||index==0x69||index==0x62||index==0x0c||index==0x20);return textureAvailable?&tex:nullptr;}}archive;
Archive* dComIfGp_getItemIconArchive(){return &archive;}
struct J2DPicture{explicit J2DPicture(const ResTIMG* t){assert(t==&tex);}void setAlpha(u8){}void draw(float,float,float w,float h,bool,bool,bool){assert(w==48&&h==48);++draws;}};
'''
main=r'''
int main(){
 Screen screen;J2DPane poe,letter;poe.visible=false;J2DTextBox title,description;
 screen.panes[tag("item_2_n")]=&poe;screen.panes[tag("lett_4_n")]=&letter;
 screen.panes[tag("item_n04")]=&title;screen.panes[tag("f_text0")]=&description;
 dMenu_Collect2D_c menu;menu.mpScreen=&screen;menu.field_0x22d[1][4]=17;menu.field_0x22d[3][4]=1;
 s_gear.add({GearKind::Sword,5,4,5});s_gear.add({GearKind::Tunic,3,2,0});s_gear.equip(0);s_gear.equip(1);
 std::any args[]{&menu};after_gear_collection_screen(nullptr,args,nullptr,nullptr);
 assert(menu.field_0x22d[1][4]==1&&menu.field_0x22d[3][4]==1);
 before_gear_collection_draw(nullptr,args,nullptr,nullptr);assert(!poe.visible&&!letter.visible&&menu.mItemNameString==0);
 assert(std::string(title.text)=="Legendary Sword"&&std::string(description.text)=="Stats 5");
 after_gear_collection_draw(nullptr,args,nullptr,nullptr);assert(draws==2&&poe.visible&&letter.visible);
 menu.mCursorX=3;menu.mSubWindowOpenCheck=3;after_gear_collection_action(nullptr,args,nullptr,nullptr);
 assert(menu.mSubWindowOpenCheck==0&&std::string(description.text)=="Stats 3");
 menu.mCursorX=0;menu.mCursorY=5;menu.mSubWindowOpenCheck=1;after_gear_collection_action(nullptr,args,nullptr,nullptr);assert(menu.mSubWindowOpenCheck==1);
 textureAvailable=false;before_gear_collection_draw(nullptr,args,nullptr,nullptr);after_gear_collection_draw(nullptr,args,nullptr,nullptr);assert(draws==2&&poe.visible);
 textureAvailable=true;before_gear_collection_draw(nullptr,args,nullptr,nullptr);after_gear_collection_draw(nullptr,args,nullptr,nullptr);assert(draws==4&&lookups==6);
 for(auto kind:{GearKind::HerosBow,GearKind::StormBoomerang,GearKind::BombBag,GearKind::IronBall}) {
  s_gear.items[0]={kind,1,1,1};assert(s_gear.equipped(GearSlot::Weapon).kind==kind);
  const auto count=draws;before_gear_collection_draw(nullptr,args,nullptr,nullptr);after_gear_collection_draw(nullptr,args,nullptr,nullptr);assert(draws==count+2);
 }
 before_gear_collection_delete(nullptr,args,nullptr,nullptr);assert(!s_gearCollection&&menu.field_0x22d[1][4]==17&&menu.field_0x22d[3][4]==1&&!poe.visible);
 arena=false;after_gear_collection_screen(nullptr,args,nullptr,nullptr);assert(!s_gearCollection&&menu.field_0x22d[1][4]==17);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(fixture+source+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-multichar','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors equipment Collection: native cells, stats, letter interception, resource recovery and restoration passed')

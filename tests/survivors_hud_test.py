"""Validate Survivors vertex packets against native J2D font/rectangle setup."""
from pathlib import Path
import re
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')
source = (root / 'src/survivors.cpp').read_text()
graf = (sdk / 'libs/JSystem/src/J2DGraph/J2DGrafContext.cpp').read_text()
font = (sdk / 'libs/JSystem/src/JUtility/JUTResFont.cpp').read_text()


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 0
    for end in range(brace, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError(signature)


native = '\n'.join([
    function(graf, 'void J2DGrafContext::setup2D()'),
    function(graf, 'void J2DGrafContext::fillBox('),
    function(font, 'void JUTResFont::setGX()'),
])
# Only vertex descriptors and byte emission are emulated. Execute the actual
# native setup/fill functions, so the test does not invent a safe fillBox API.
constants = sorted(set(re.findall(r'\bGX_\w+', native)) - {'GX_MAX_TEVSTAGE'})
tracked = {'GXClearVtxDesc', 'GXSetVtxDesc', 'GXBegin', 'GXEnd',
           'GXPosition3f32', 'GXColor1u32', 'GXSetNumTexGens'}
noops = sorted(set(re.findall(r'\b(GX\w+)\(', native)) - tracked)
prefix = r'''
bool s_victory=false,s_deathPending=false;unsigned s_victoryTicks=0;

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include "survivors_roster.hpp"
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using f32=float; using GXTevStageID=int; using GXBlendMode=int; using GXBlendFactor=int;
using Mtx=float[3][4];
void MTXIdentity(Mtx){}
#define ZoneScoped
'''
prefix += 'enum { GX_MAX_TEVSTAGE=16, ' + ', '.join(constants) + ' };\n'
prefix += '\n'.join('template<class... T> void '+name+'(T...){}' for name in noops)
stubs = r'''
unsigned packetBytes=0, expectedBytes=0, boxes=0, labels=0, texGens=0;
bool textureCoordinates=false;
void GXClearVtxDesc(){textureCoordinates=false;}
void GXSetVtxDesc(int attr,int type){if(attr==GX_VA_TEX0)textureCoordinates=type==GX_DIRECT;}
void GXSetNumTexGens(unsigned n){texGens=n;}
void GXBegin(int,int,unsigned vertices){
    packetBytes=0;
    // fillBox uses F32 XYZ + RGBA8. Font TEX0 is two direct U16 coordinates.
    expectedBytes=vertices*(12+4+(textureCoordinates?4:0));
}
void GXPosition3f32(float,float,float){packetBytes+=12;}
void GXColor1u32(unsigned){packetBytes+=4;}
void GXEnd(){assert(packetBytes==expectedBytes && "GX vertex packet length mismatch");++boxes;}
namespace JGeometry {
template<class T> struct TBox2 {
    struct Point {T x,y;} i,f;
    TBox2(T x,T y,T right,T bottom):i{x,y},f{right,bottom}{}
};
}
namespace JUtility {
struct TColor {
    unsigned value;
    TColor(unsigned v=0):value(v){}
    TColor(unsigned r,unsigned g,unsigned b,unsigned a):value((r<<24)|(g<<16)|(b<<8)|a){}
    operator unsigned() const {return value;}
};
}
struct J2DGrafContext {
    Mtx mPosMtx{}; int mLineWidth=6;
    struct {int mType=0,mSrcFactor=0,mDstFactor=0;} mBoxPart;
    JUtility::TColor mColorTL{1},mColorTR{2},mColorBR{3},mColorBL{4};
    void setup2D(); void fillBox(const JGeometry::TBox2<f32>&);
    void setColor(JUtility::TColor c){setColor(c,c,c,c);}
    void setColor(JUtility::TColor a,JUtility::TColor b,JUtility::TColor c,JUtility::TColor d){
        mColorTL=a;mColorTR=b;mColorBR=c;mColorBL=d;
    }
} context;
unsigned skulls=0,skullLookups=0;bool hasMapArchive=true,hasSkullTexture=true;
struct ResTIMG{} skullTexture;
struct MapArchive{} mapArchive;
MapArchive* dComIfGp_getDmapResArchive(){return hasMapArchive?&mapArchive:nullptr;}
void* JKRGetNameResource(const char* name,MapArchive* archive){
 assert(archive==&mapArchive&&std::string(name)=="tt_map_icon_boss_ci8_32_00.bti");
 ++skullLookups;return hasSkullTexture?&skullTexture:nullptr;
}
struct J2DPicture {
 explicit J2DPicture(ResTIMG* image){assert(image==&skullTexture);}
 void draw(float x,float y,float w,float h,bool a,bool b,bool c){
  assert(x==392&&y==40&&w==20&&h==20&&!a&&!b&&!c);++skulls;textureCoordinates=true;
 }
};
struct JUTResFont {void setGX();} font,ruby;
unsigned rubyLabels=0;std::vector<std::string> printed;unsigned textColor=0;
bool hasFont=true, arena=true, transition=false, window=false, hasGraf=true;
JUTResFont* mDoExt_getMesgFont(){return hasFont?&font:nullptr;}
JUTResFont* mDoExt_getRubyFont(){return hasFont?&ruby:nullptr;}
enum {HBIND_CENTER,VBIND_TOP};
struct J2DPrint {
    JUTResFont* face;
    J2DPrint(JUTResFont* f,JUtility::TColor c,JUtility::TColor):face(f){textColor=c;f->setGX();}
    void setFontSize(float,float){}
    void printReturn(const char* t,float,float,int,int,float,float,int){printed.emplace_back(t);++labels;if(face==&ruby)++rubyLabels;}
};

using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz {float x=0,y=0,z=0;float abs2(cXyz p)const{return (x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z);}};
struct Actor {int health=100,field_0x560=100;struct{cXyz pos;}current;};
std::array<Actor,8> actors;
Actor player;bool hasPlayer=true;
Actor* daAlink_getAlinkActorClass(){return hasPlayer?&player:nullptr;}
Actor* actor_by_id(ActorId id){return id<actors.size()?&actors[id]:nullptr;}
struct Enemy {ActorId id=kNone;bool elite=false,initialized=true,dormant=false,retiring=false,credited=false;
 int eliteSlot=-1;unsigned species=0;};
std::array<Enemy,8> s_enemies;
struct dMeter2Draw_c {} meter;
namespace mods {template<class T>T arg(void*,unsigned){return &meter;}}
unsigned bossBars=0;float bossPercentage=0;bool barAvailable=true;
bool draw_boss_health_meter(dMeter2Draw_c*,float percentage,float x,float y,float w,float h){
 assert(x==144&&y==120&&w==320&&h==24);
 if(!barAvailable)return false;
 ++bossBars;bossPercentage=percentage;textureCoordinates=true;return true;
}
struct ModContext {};
int potion_slot(){return -1;}
Run s_run;bool s_chestsExhausted=false; unsigned s_spawnFailures=0;
bool saving=false;bool save_modal_active(){return saving;}
bool in_arena(){return arena;} bool transitioning(){return transition;} bool ui_visible(){return window;}
J2DGrafContext* dComIfGp_getCurrentGrafPort(){return hasGraf?&context:nullptr;}
struct mDoGph_gInf_c {static float getMinXF(){return 0;} static float getWidthF(){return 608;}};
'''
main = r'''
int main(){
    // First visible HUD after the entrance, ordinary runs, and both messages.
    for(auto phase:{Phase::Running,Phase::Upgrade,Phase::Treasure,Phase::Victory}){
        for(bool availableFont:{false,true})for(unsigned ticks:{0u,duration})for(unsigned failures:{0u,10u})for(unsigned horde:{0u,450u}){
            s_run.phase=phase;s_run.ticks=ticks;s_run.horde_remaining=horde;s_run.xp=4;s_spawnFailures=failures;hasFont=availableFont;
            boxes=labels=0;textureCoordinates=true;
            draw_hud(nullptr,nullptr,nullptr,nullptr);
            assert(boxes==2u && !textureCoordinates && texGens==0);
            assert(context.mColorTL==1 && context.mColorTR==2 && context.mColorBR==3 && context.mColorBL==4);
            assert((labels!=0)==availableFont);
        }
    }

    // Baseline displays only the clock; the first difficulty step adds skull/rank.
    s_run.start(42);hasFont=true;printed.clear();rubyLabels=boxes=0;
    draw_run_rank_hud(304);assert(rubyLabels==1&&boxes==0&&skulls==0&&skullLookups==0);
    assert(printed[0].find("00:00")!=std::string::npos);
    s_run.options.enemy_hp=2;printed.clear();rubyLabels=boxes=0;
    draw_run_rank_hud(304);assert(rubyLabels==2&&boxes==0&&skulls==1&&printed[1]=="1");
    hasMapArchive=false;draw_run_rank_hud(304);assert(skulls==1&&skullLookups==1);hasMapArchive=true;
    hasSkullTexture=false;draw_run_rank_hud(304);assert(skulls==1&&skullLookups==2);hasSkullTexture=true;
    draw_run_rank_hud(304);assert(skulls==2&&skullLookups==3);
    s_run.start(42);s_run.options.enemy_hp=10;s_run.options.enemy_damage=10;
    s_run.options.enemy_speed=3;s_run.options.horde=5;s_run.phase=Phase::Victory;s_run.continue_endless();
    printed.clear();rubyLabels=0;hasFont=true;draw_hud(nullptr,nullptr,nullptr,nullptr);
    assert(rubyLabels==2&&printed[0].find("Endless")!=std::string::npos&&printed[1]=="24");
    s_run.start(42);
    // Victory uses the boss font, a red ceiling-rounded 10-second timer,
    // and restores GX vertex descriptors for the next native HUD element.
    s_run.phase=Phase::Running;s_victory=true;hasFont=true;
    for(unsigned tick:{0u,1u,29u,30u,270u,299u,300u}) {
        printed.clear();rubyLabels=boxes=0;s_victoryTicks=tick;
        draw_hud(nullptr,nullptr,nullptr,nullptr);
        assert(rubyLabels==3&&printed[0]=="Victory"&&printed[1]=="Victory");
        assert(printed.back()==std::to_string(tick>=300?0:(300-tick+29)/30));
        assert(textColor==0xff4137ffu&&!boxes&&!textureCoordinates&&texGens==0);
    }
    s_victory=false;s_deathPending=true;s_run.phase=Phase::Defeat;boxes=labels=0;
    draw_hud(nullptr,nullptr,nullptr,nullptr);assert(!boxes&&!labels);s_deathPending=false;
    s_run.phase=Phase::Running;saving=true;boxes=labels=rubyLabels=0;
    draw_hud(nullptr,nullptr,nullptr,nullptr);assert(!boxes&&!labels&&!rubyLabels);saving=false;
    // Only live, initialized elites appear. Prefer the nearest initially and
    // keep it stable as another elite walks closer; timed bosses take priority.
    assert(!boss_hud_target());
    s_enemies[0].id=0;s_enemies[0].elite=true;s_enemies[0].dormant=true;
    assert(!boss_hud_target());s_enemies[0].dormant=false;
    assert(boss_hud_target()==&s_enemies[0]);
    s_enemies[1]=s_enemies[0];s_enemies[1].id=1;
    actors[0].current.pos.x=100;assert(boss_hud_target()==&s_enemies[0]);
    s_enemies[1].eliteSlot=0;s_enemies[1].species=unsigned(Species::Darknut);
    actors[1].health=25;assert(boss_hud_target()==&s_enemies[1]);
    hasFont=true;s_run.phase=Phase::Running;bossBars=rubyLabels=0;
    draw_hud(nullptr,nullptr,nullptr,nullptr);
    assert(bossBars==1&&bossPercentage==25&&rubyLabels==3&&!textureCoordinates);
    assert(!std::strcmp(boss_hud_name(s_enemies[1].species),"Darknut"));
    for(const auto& roster:map_rosters)for(auto species:roster.elites)
        assert(std::strcmp(boss_hud_name(unsigned(species)),"Elite"));
    // Missing meter/font is safe; a missing font does not suppress the HP bar.
    hasFont=false;assert(draw_boss_hud(&meter,304));
    barAvailable=false;assert(!draw_boss_hud(&meter,304));barAvailable=true;
    for(int reason=0;reason<7;++reason){
        s_enemies[1].initialized=reason!=0;s_enemies[1].retiring=reason==1;
        s_enemies[1].credited=reason==2;s_enemies[1].id=reason==3?99:1;
        actors[1].health=reason==4?0:100;actors[1].field_0x560=reason==5?0:100;
        s_enemies[1].elite=reason!=6;
        assert(boss_hud_target()==&s_enemies[0]);
    }
    s_enemies[1]={};actors[0].health=0;assert(!boss_hud_target()&&s_bossHudTarget==kNone);
    actors[0].health=100;hasPlayer=false;assert(!boss_hud_target());hasPlayer=true;
    for(int reason=0;reason<6;++reason){
        arena=reason!=0;transition=reason==1;window=reason==2;hasGraf=reason!=3;
        s_run.phase=reason==4?Phase::Loading:reason==5?Phase::Ready:Phase::Running;boxes=labels=0;
        draw_hud(nullptr,nullptr,nullptr,nullptr);assert(boxes==0 && labels==0);
    }
    std::cout<<"Survivors HUD: native font/box vertex packets, messages and render gates passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(prefix + stubs + '\nbool survivors_hub_active(){return false;} void draw_hub_labels(){} void draw_chest_prices(){} void draw_combat_feedback(){}\n' + native + '\n' + function(source, 'void label(') + '\n' +
                   (root/'src/survivors_boss_hud.inc').read_text() + '\n' + function(source, 'void draw_hud(') + '\n' + main)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

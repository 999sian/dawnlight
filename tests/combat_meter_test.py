"""Exercise production shared-meter geometry and restoration around boss draws."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/combat_meter.cpp').read_text()


def function(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[start:end + 1]
    raise AssertionError(signature)


fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include "combat_meter.hpp"
using namespace dawnlight;
using Mtx=float[3][4];
struct Vec {float x,y,z;};
namespace JUtility {struct TColor {
    unsigned value;
    TColor(unsigned v=0):value(v){}
    TColor(unsigned r,unsigned g,unsigned b,unsigned a):value(r<<24|g<<16|b<<8|a){}
    bool operator==(const TColor&)const=default;
};}
struct J2DPicture {
    bool visible=false;
    JUtility::TColor black{13},white{27};
    void show(){visible=true;}void hide(){visible=false;}bool isVisible(){return visible;}
    auto getBlack(){return black;}auto getWhite(){return white;}
    void setBlackWhite(JUtility::TColor b,JUtility::TColor w){black=b;white=w;}
};
struct CPaneMgr {
    J2DPicture pane;
    CPaneMgr* parent=nullptr;
    float posX=0,posY=0,x=0,y=0,width=200,height=20,scaleX=1,scaleY=1,alpha=.4f;
    float initX=0,initY=0,initWidth=200,initHeight=20;
    J2DPicture* getPanePtr(){return &pane;}
    float getPosX(){return posX;}float getPosY(){return posY;}
    float getTranslateX(){return x;}float getTranslateY(){return y;}
    float getSizeX(){return width;}float getSizeY(){return height;}
    float getScaleX(){return scaleX;}float getScaleY(){return scaleY;}
    float getAlphaRate(){return alpha;}
    float getInitPosX(){return initX;}float getInitPosY(){return initY;}
    float getInitSizeX(){return initWidth;}float getInitSizeY(){return initHeight;}
    void move(float a,float b){posX=a;posY=b;}void translate(float a,float b){x=a;y=b;}
    void resize(float a,float b){width=a;height=b;}void scale(float a,float b){scaleX=a;scaleY=b;}
    void setAlphaRate(float a){alpha=a;}
    void setBlackWhite(JUtility::TColor b,JUtility::TColor w){pane.setBlackWhite(b,w);}
    Vec point(float a,float b){
        a=posX+x+a*scaleX;b=posY+y+b*scaleY;
        return parent?parent->point(a,b):Vec{a,b,0};
    }
    Vec getGlobalVtx(Mtx*,int corner,bool,int){return point(corner&1?width:0,corner&2?height:0);}
};
struct J2DGrafContext {unsigned resets=0;void setup2D(){++resets;}} graf;
bool hasGraf=true;
auto* dComIfGp_getCurrentGrafPort(){return hasGraf?&graf:nullptr;}
struct J2DScreen {void draw(float,float,J2DGrafContext*);};
class dMeter2Draw_c {public:
    J2DScreen* mpKanteraScreen=nullptr;
    CPaneMgr *mpMagicParent=nullptr,*mpMagicBase=nullptr,*mpMagicFrameL=nullptr,
        *mpMagicMeter=nullptr,*mpMagicFrameR=nullptr,*mpLifeParent=nullptr;
};
struct DawnlightFierceDeityHudFrame {
    unsigned size=0;int state=0;float left=0,top=0,width=0,height=0,scaleX=0,scaleY=0,alpha=0;
    J2DGrafContext* graf=nullptr;
};
int fierce_deity_hud_state(){return 0;}
'''
geometry = source[source.index('struct PaneState {'):source.index('ScreenBounds full_heart_bounds(')]
fixture += geometry + '\n' + function('ScreenBounds combat_meter_bounds(')
fixture += r'''
unsigned heartPlacements=0,fadeDraws=0,screenDraws=0;
int combat_meter_transform(CombatMeterStyle){return 0;}
ScreenBounds position_combat_meter(dMeter2Draw_c* m,int,int){++heartPlacements;return combat_meter_bounds(m);}
namespace dawnlight {float combat_meter_screen_alpha(dMeter2Draw_c*){return 1;}}
void draw_combat_meter_screen(J2DScreen*,J2DGrafContext*,bool,float,const DawnlightFierceDeityHudFrame*){++fadeDraws;}
dMeter2Draw_c* current=nullptr;
float expectedRatio=1;
ScreenBounds drawn;
void J2DScreen::draw(float,float,J2DGrafContext* g){
    ++screenDraws;assert(g==&graf);
    drawn=combat_meter_bounds(current);
    for(auto* p:{current->mpMagicParent,current->mpMagicBase,current->mpMagicFrameL,
        current->mpMagicMeter,current->mpMagicFrameR})assert(p->pane.visible&&p->alpha==1);
    assert(current->mpMagicMeter->pane.black==JUtility::TColor(255,100,100,255));
    assert(current->mpMagicMeter->pane.white==JUtility::TColor(210,0,0,255));
    assert(std::abs(current->mpMagicMeter->width-current->mpMagicMeter->initWidth*expectedRatio)<.001f);
}
'''
fixture += '\nnamespace dawnlight {\n' + '\n'.join(function(s) for s in [
    'static bool draw_meter(', 'void draw_combat_meter(', 'bool draw_boss_health_meter('
]) + '\n}\n'
main = r'''
int main(){
    std::array<CPaneMgr,5> panes;
    J2DScreen screen;
    dMeter2Draw_c meter;current=&meter;
    meter.mpKanteraScreen=&screen;meter.mpMagicParent=&panes[0];meter.mpMagicBase=&panes[1];
    meter.mpMagicFrameL=&panes[2];meter.mpMagicMeter=&panes[3];meter.mpMagicFrameR=&panes[4];
    panes[0].move(23,16);panes[0].translate(31,12);panes[0].scale(.7f,1.3f);
    for(unsigned i=1;i<panes.size();++i){panes[i].parent=&panes[0];panes[i].pane.visible=i%2;}
    panes[2].initWidth=panes[2].width=12;
    panes[4].initWidth=panes[4].width=12;panes[4].initX=200;panes[4].posX=80;
    panes[3].width=23;panes[3].translate(2,0);
    std::array<PaneState,5> before;
    for(unsigned i=0;i<panes.size();++i)before[i]=save_pane(&panes[i]);
    const auto verifyRestored=[&]{
        for(unsigned i=0;i<panes.size();++i){const auto a=save_pane(&panes[i]);const auto& b=before[i];
            assert(a.posX==b.posX&&a.posY==b.posY&&a.x==b.x&&a.y==b.y);
            assert(a.width==b.width&&a.height==b.height&&a.scaleX==b.scaleX&&a.scaleY==b.scaleY);
            assert(a.alpha==b.alpha&&a.visible==b.visible);
            assert(panes[i].pane.black==JUtility::TColor(13)&&panes[i].pane.white==JUtility::TColor(27));
        }
    };
    for(float health:{-10.f,0.f,1.f,25.f,100.f,150.f}){
        expectedRatio=std::clamp(health,0.f,100.f)/100;
        const unsigned resets=graf.resets;
        assert(draw_boss_health_meter(&meter,health,144,120,320,24));
        assert(std::abs(drawn.left-144)<.001f&&std::abs(drawn.top-120)<.001f);
        assert(std::abs(drawn.right-464)<.001f&&std::abs(drawn.bottom-144)<.001f);
        assert(graf.resets==resets+2&&!fadeDraws&&!heartPlacements);verifyRestored();
    }
    assert(screenDraws==6);
    assert(!draw_boss_health_meter(nullptr,50,0,0,320,24));
    assert(!draw_boss_health_meter(&meter,50,0,0,0,24));
    hasGraf=false;assert(!draw_boss_health_meter(&meter,50,0,0,320,24));verifyRestored();hasGraf=true;
    meter.mpMagicFrameR=nullptr;assert(!draw_boss_health_meter(&meter,50,0,0,320,24));
    meter.mpMagicFrameR=&panes[4];meter.mpLifeParent=&panes[0];
    for(auto style:{CombatMeterStyle::Stamina,CombatMeterStyle::StaminaExhausted,CombatMeterStyle::FierceDeity}){
        draw_combat_meter(&meter,50,style,1);verifyRestored();
    }
    assert(fadeDraws==3&&heartPlacements==3&&screenDraws==6);
    std::cout<<"Combat meters: boss placement, red fill, clamps, GX reset, pane restoration and existing paths passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp = Path(temp) / 'test.cpp'
    binary = Path(temp) / 'test'
    cpp.write_text(fixture + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

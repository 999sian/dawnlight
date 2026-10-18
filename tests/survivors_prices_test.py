"""Floating prices use current shared cost, paid chests and visible camera space."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;using s16=short;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
Run s_run;constexpr unsigned kSmallChestCount=2;
struct Chest{bool active=false,opening=false;cXyz position;bool free=false,fixed=false;};std::array<Chest,3>s_chests;
bool event=false,view=true;bool dComIfGp_event_runCheck(){return event;}bool dComIfGd_getView(){return view;}
float matrix[4][4]={{},{},{},{0,0,1,0}};auto* dComIfGd_getProjViewMtx(){return &matrix;}
float cM_ssin(s16 a){return std::sin(float(a)*3.14159265f/32768);}
void mDoLib_project(cXyz* world,cXyz* screen){*screen=*world;}
namespace mDoGph_gInf_c{float getMinXF(){return 0;}float getMaxXF(){return 640;}float getHeightF(){return 480;}}
std::vector<std::string>labels;void label(const char* text,float,float,float){labels.push_back(text);}
'''
main=r'''
int main(){
 s_run.start(1);for(auto& c:s_chests)c={true,false,{320,0,1000}};
 draw_chest_prices();assert((labels==std::vector<std::string>{"20","20"}));
 labels.clear();s_run.chests_opened=2;draw_chest_prices();assert((labels==std::vector<std::string>{"32","32"}));
 labels.clear();s_chests[1].fixed=true;draw_chest_prices();assert((labels==std::vector<std::string>{"32","40"}));
 labels.clear();s_chests[0].opening=true;draw_chest_prices();assert(labels.size()==1);
 labels.clear();s_chests[1].position.z=-1000;draw_chest_prices();assert(labels.empty());
 s_chests[1].position={900,0,1000};draw_chest_prices();assert(labels.empty());
 s_chests[1].position={320,0,1000};event=true;draw_chest_prices();assert(labels.empty());event=false;
 view=false;draw_chest_prices();assert(labels.empty());view=true;
 s_run.phase=Phase::TreasureChoice;draw_chest_prices();assert(labels.empty());
 std::cout<<"Survivors prices: shared cost, paid-only cards, camera bounds and pause gates passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_prices.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)

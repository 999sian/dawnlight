"""Production dry-floor and reachable-spawn checks, with native collision boundaries."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}float absXZ()const{return std::hypot(x,z);}};
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_WtrChk{cXyz bottom;float top=0,height=0;void Set(cXyz p,float y){bottom=p;top=y;}float GetHeight(){return height;}};
struct dBgS_LinChk{void Set(cXyz*,cXyz*,void*){}};
struct Arena{int room=6;}data;const Arena& arena(){return data;}
int terrain=0,code=0,room=6;bool wet=false,wall=false,barrier=false;float waterY=50;
struct Bg{
 float GroundCross(dBgS_ObjGndChk* q){
  if(terrain==1&&q->pos.x>400&&q->pos.x<600)return -1.e9f;
  if(terrain==2&&q->pos.x>400)return 100; // Reachable by sight, not by walking.
  if(terrain==3)return q->pos.x*0.2f; // Continuous gentle slope.
  if(terrain==4&&q->pos.z>20)return -100; // Unsafe footprint edge.
  return 0;
 }
 int GetRoomId(dBgS_ObjGndChk&){return room;}int GetGroundCode(dBgS_ObjGndChk&){return code;}
 bool WaterChk(dBgS_WtrChk* q){q->height=waterY;return wet&&waterY>=q->bottom.y&&waterY<=q->top;}
 bool LineCross(dBgS_LinChk*){return wall;}
}bg;Bg& dComIfG_Bgsp(){return bg;}
bool arena_path_clear(cXyz,cXyz,float){return !barrier;}
'''
main=r'''
int main(){
 cXyz from(0,0,0),to(1000,0,0);
 assert(walkable_spawn_path(from,to,40)&&to.y==0);
 wet=true;assert(!walkable_spawn_path(from,to,40)); // Actual water volume over ordinary ground.
 waterY=-100;assert(walkable_spawn_path(from,to,40)); // Bridge above water.
 wet=false;
 for(int bad:{1,2,4}){terrain=bad;to={1000,0,0};assert(!walkable_spawn_path(from,to,40));}
 terrain=3;to={1000,0,0};assert(walkable_spawn_path(from,to,40)&&std::abs(to.y-200)<1);
 terrain=0;room=7;assert(!walkable_spawn_path(from,to,40));room=6;
 for(int bad:{4,5,10}){code=bad;assert(!walkable_spawn_path(from,to,40));}code=0;
 wall=true;assert(!walkable_spawn_path(from,to,40));wall=false;
 barrier=true;assert(!walkable_spawn_path(from,to,40));
 std::cout<<"Survivors ground: water, bridges, ledges, gaps, slopes, footprints and barriers passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_ground.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)

"""Bounded production knockback: strength, rapid fire, pause ownership and terrain."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
struct cXyz {float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz p)const{return{x+p.x,y+p.y,z+p.z};}
 cXyz operator-(cXyz p)const{return{x-p.x,y-p.y,z-p.z};}
 cXyz operator*(float s)const{return{x*s,y*s,z*s};}
 cXyz& operator+=(cXyz p){x+=p.x;y+=p.y;z+=p.z;return *this;}
 float absXZ()const{return std::sqrt(x*x+z*z);}};
struct fopAc_ac_c {int health=100;float speedF=7;cXyz speed{1,12,3};struct{cXyz pos;}current,old;cXyz eyePos;};
fopAc_ac_c player,actor;
auto* daAlink_getAlinkActorClass(){return &player;}
bool native_boss_profile(int p){return p==99;}
struct Enemy{bool elite=false;int profile=0;}enemy;
struct Effect{cXyz knockStep;unsigned knockFrames=0,knockCooldown=0;}effect;
Effect& effect_for(Enemy&){return effect;}
Run s_run;
bool blocked=false,ledge=false,noFloor=false;float floorY=0,destinationY=0;
struct dBgS_LinChk {void Set(cXyz*,cXyz*,fopAc_ac_c*){}};
struct Background {bool LineCross(dBgS_LinChk*){return blocked;}}bg;
auto& dComIfG_Bgsp(){return bg;}
bool dry_arena_floor(cXyz& p,float,float){p.y=floorY;return !noFloor;}
bool walkable_spawn_path(cXyz,cXyz& p,float){p.y=destinationY;return !ledge;}
void reset(){s_run.start(1);effect={};actor={};actor.current.pos=actor.old.pos=actor.eyePos={0,0,100};
 enemy={};floorY=destinationY=0;blocked=ledge=noFloor=false;}
'''
main = r'''
int main(){
 for(unsigned rank=0;rank<=10;++rank)for(bool heavy:{false,true})for(bool elite:{false,true}){
  reset();enemy.elite=elite;s_run.ranks[unsigned(Upgrade::Knockback)]=rank;
  // Weapon level, Reach, Power, crit, run time and simultaneous pellets never scale the push.
  s_run.ranks[unsigned(Upgrade::Ball)]=s_run.ranks[unsigned(Upgrade::ShieldAttack)]=30;
  s_run.ranks[unsigned(Upgrade::Power)]=s_run.ranks[unsigned(Upgrade::Reach)]=30;s_run.ticks=duration;
  for(unsigned n=0;n<100;++n)queue_knockback(enemy,&actor,heavy);
  const float distance=((heavy?240.f:0.f)+40.f*rank)*(elite?0.f:1.f);
  for(unsigned frame=0;frame<6;++frame){update_knockback(enemy,&actor);queue_knockback(enemy,&actor,heavy);}
  assert(std::abs(actor.current.pos.z-100-distance)<.001f&&actor.current.pos.y==0);
  assert(actor.current.pos.z==actor.old.pos.z&&actor.current.pos.z==actor.eyePos.z);
  assert(!effect.knockFrames);
  for(unsigned frame=0;frame<5;++frame){queue_knockback(enemy,&actor,heavy);update_knockback(enemy,&actor);}
  assert(std::abs(actor.current.pos.z-100-distance)<.001f);
 }
 reset();s_run.ranks[unsigned(Upgrade::Knockback)]=10;
 queue_knockback(enemy,&actor,false);queue_knockback(enemy,&actor,true);
 for(unsigned frame=0;frame<6;++frame)update_knockback(enemy,&actor);
 assert(std::abs(actor.current.pos.z-740)<.001f); // strongest simultaneous hit, not 1040 units
 for(unsigned reason=0;reason<3;++reason){
  reset();queue_knockback(enemy,&actor,true);blocked=reason==0;ledge=reason==1;noFloor=reason==2;
  update_knockback(enemy,&actor);assert(actor.current.pos.z==100&&!effect.knockFrames);
 }
 reset();actor.current.pos.y=actor.old.pos.y=actor.eyePos.y=600;
 queue_knockback(enemy,&actor,true);for(unsigned n=0;n<6;++n)update_knockback(enemy,&actor);
 assert(actor.current.pos.y==600&&std::abs(actor.current.pos.z-340)<.001f);
 // Low airborne attacks keep their altitude instead of being snapped to the floor.
 for(float height:{1.f,20.f,59.f,60.f,61.f}) {
  reset();actor.current.pos.y=actor.old.pos.y=actor.eyePos.y=height;
  queue_knockback(enemy,&actor,true);
  for(unsigned frame=0;frame<6;++frame)update_knockback(enemy,&actor);
  assert(actor.current.pos.y==height&&actor.old.pos.y==height&&actor.eyePos.y==height);
  assert(actor.speedF==7&&actor.speed.x==1&&actor.speed.y==12&&actor.speed.z==3);
 }
 // A safe slope translates the floor offset, preserving airborne height and velocity.
 reset();actor.current.pos.y=actor.old.pos.y=actor.eyePos.y=20;destinationY=10;
 queue_knockback(enemy,&actor,true);update_knockback(enemy,&actor);
 assert(actor.current.pos.y==30&&actor.old.pos.y==30&&actor.eyePos.y==30&&actor.speed.y==12);
 reset();enemy.profile=99;queue_knockback(enemy,&actor,true);assert(!effect.knockFrames);
 effect.knockFrames=6;effect.knockCooldown=12;update_knockback(enemy,&actor);assert(!effect.knockFrames&&!effect.knockCooldown&&actor.current.pos.z==100);
 reset();actor.health=0;queue_knockback(enemy,&actor,true);assert(!effect.knockFrames);
 std::cout<<"Survivors knockback: fixed heavy push, capped upgrades, elites, simultaneous/rapid hits, walls/ledges and flight passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture + (root / 'src/survivors_knockback.inc').read_text() + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'src'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

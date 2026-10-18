"""Production close-range facing before attack and after native movement."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;using s16=short;
struct ModContext{};
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}
 float absXZ()const{return std::hypot(x,z);}};
struct fopAc_ac_c{unsigned id=1;int health=20;cXyz scale{1,1,1};struct{cXyz pos;struct{s16 y=0;}angle;}current;struct{s16 y=0;}shape_angle;};
struct daAlink_c:fopAc_ac_c{}player;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}unsigned s_player=9;
constexpr int fpcNm_E_OC_e=1,fpcNm_E_MS_e=2,fpcNm_E_BA_e=3,fpcNm_E_RD_e=4;
struct daE_OC_c:fopAc_ac_c{int mActionMode=4;};
struct e_ms_class:fopAc_ac_c{int mAction=1;s16 field_0x5c8=0,mPlayerAngleY=0;};
struct e_ba_class{fopAc_ac_c enemy;enum{ACT_FIGHT_FLY,ACT_FIGHT,ACT_ATTACK,ACT_CHANCE};
 bool mIsDying=false;float mKnockbackSpeed=0;int mAction=ACT_ATTACK;s16 mPlayerAngleY=0;cXyz mTargetPos;};
struct e_rd_class{fopAc_ac_c enemy;int actor_set=0,ride_mode=0,action=4;float field_0x9f0=0;s16 angleY=0;};
struct Enemy{bool initialized=true,retiring=false,credited=false,dormant=false;int profile=1;}enemy;
bool owned=true,playing=true;Enemy* tracked(unsigned id){return owned&&id==1?&enemy:nullptr;}
bool playable(){return playing;}Run s_run;
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
struct dBgS_Acch{fopAc_ac_c* actor;fopAc_ac_c* getMyAc(){return actor;}};
namespace mods{template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
'''
main=r'''
int main(){
 s_run.start(1);player.id=9;daE_OC_c bok;e_ms_class rat;e_ba_class bat;e_rd_class bulblin;
 fopAc_ac_c* actors[]={&bok,&rat,&bat.enemy,&bulblin.enemy};
 for(unsigned i=0;i<4;++i){auto* actor=actors[i];enemy.profile=i+1;
  for(cXyz pos:{cXyz(100,0,0),cXyz(-100,0,0),cXyz(0,0,-100)}){
   player.current.pos=pos;track_enemy_facing(actor);const auto expected=cM_atan2s(pos.x,pos.z);
   assert(actor->shape_angle.y==expected&&actor->current.angle.y==expected);
   actor->shape_angle.y=actor->current.angle.y=0;dBgS_Acch acch{actor};std::any args[]={&acch};
   after_enemy_facing(nullptr,args,nullptr,nullptr);assert(actor->shape_angle.y==expected);
  }
  assert((i!=1||rat.field_0x5c8==actor->shape_angle.y)&&(i!=3||bulblin.angleY==actor->shape_angle.y));
  for(unsigned reason=0;reason<7;++reason){actor->shape_angle.y=actor->current.angle.y=123;
   enemy.dormant=reason==0;enemy.retiring=reason==1;enemy.credited=reason==2;owned=reason!=3;
   playing=reason!=4;actor->health=reason==5?0:20;player.current.pos={reason==6?2000.f:100.f,0,0};
   track_enemy_facing(actor);assert(actor->shape_angle.y==123&&actor->current.angle.y==123);
  }
  enemy={};owned=playing=true;actor->health=20;
 }
 player.current.pos={100,0,0};
 bok.mActionMode=6;rat.mAction=4;bat.mAction=e_ba_class::ACT_CHANCE;bulblin.action=20;
 for(unsigned i=0;i<4;++i){enemy.profile=i+1;actors[i]->shape_angle.y=123;track_enemy_facing(actors[i]);assert(actors[i]->shape_angle.y==123);}
 enemy.profile=1;bok.mActionMode=4;bok.scale={2,2,2};player.current.pos={700,0,0};track_enemy_facing(&bok);assert(bok.shape_angle.y==0x4000);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_enemy_facing.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors enemy facing: live melee tracking, native movement boundary, elite range and reaction/isolation gates passed')

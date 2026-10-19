"""Independent ranged release: same-frame weapons, sword overlap and cosmetic isolation."""
from pathlib import Path
import subprocess
import sys
import tempfile
root=Path(__file__).resolve().parents[1]
sdk=Path(sys.argv[1]) if len(sys.argv)>1 else root/'dusklight'
native=(sdk/'src/d/actor/d_a_alink_ironball.inc').read_text()
native_next=native[native.index('int daAlink_c::checkNextActionIronBall()'):native.index('int daAlink_c::procIronBallSubjectInit()')]
core=(root/'src/survivors.cpp').read_text()
for method,alias in [('checkUpperItemActionIronBall','SurvivorsBallAction'),('checkNextActionIronBall','SurvivorsBallNext')]:
    assert f'DEFINE_HOOK(&daAlink_c::{method}, {alias});' in core
    assert f'PRE({alias}, before_ranged_native);' in core
source=(root/'src/survivors_ranged.inc').read_text().split('void draw_ranged()')[0]
fixture=r'''
#include <cassert>
#include <cmath>
#include <vector>
#include "survivors_rules.hpp"
#include "survivors_projectiles.hpp"
using namespace dawnlight::survivors;
using u16=unsigned short;using s16=short;
constexpr int FALSE=0,dItemNo_NONE_e=255;
constexpr u16 dRes_ID_ALANM_BCK_ARELORD_e=1,dRes_ID_ALANM_BCK_ASHOOT_e=2,dRes_ID_ALANM_BCK_BOOMTHROW_e=3,dRes_ID_ALANM_BCK_BOMBTHROW_e=4,dRes_ID_ALANM_BCK_IBTHROW_e=5,dRes_ID_ALANM_BCK_IBPULL_e=6,dRes_ID_ALANM_BCK_IBCATCH_e=7;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}float absXZ()const{return std::sqrt(x*x+z*z);}};
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.14159265);}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* p,int){return static_cast<T>(p);}}
struct Frame{float frame=0,rate=1;int end=12;float getFrame(){return frame;}int getEnd(){return end;}void setRate(float n){rate=n;}void setFrame(float n){frame=n;}};
struct daAlink_c{
 enum{UPPER_2=2};int id=1,mEquipItem=0x103,proc=0;u16 animation=0;
 bool equipping=false;bool checkEquipAnime(){return equipping;}
 int mFastShotTime=0;short field_0x2fe4=0;bool locked=true;
 bool checkIronBallAnime(){return animation==dRes_ID_ALANM_BCK_IBTHROW_e;}
 void setIronBallReadyAnime(){animation=dRes_ID_ALANM_BCK_IBTHROW_e;}
 void setFastShotTimer(){mFastShotTime=10;}
 bool checkAttentionLock(){return locked;}
 int procIronBallSubjectInit(){proc=2;return 1;}int procIronBallMoveInit(){proc=2;return 1;}
 int checkNextActionIronBall();
 Frame mUpperFrameCtrl[3],mUnderFrameCtrl[1];float field_0x347c=5,field_0x3484=8;
 struct{cXyz pos;struct{s16 y=123;}angle;}current;
 struct{s16 x=0,y=0;}shape_angle,mBodyAngle;
 bool checkUpperAnime(u16 a){return animation==a;}
 void resetUpperAnime(int,float){animation=0;}
 void setUpperAnimeBaseSpeed(u16 a,float,float){animation=a;mUpperFrameCtrl[2]={0,1,a==1?12:18};}
 void deleteEquipItem(int,int){mEquipItem=dItemNo_NONE_e;}
 void procWaitInit(){proc=0;}
 bool checkAnmEnd(Frame* f){return f->frame>=f->end;}
}player;
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
int fopAcM_GetID(daAlink_c* p){return p->id;}
bool sword_aim_target(daAlink_c*,cXyz& target,cXyz& forward){target={0,140,900};forward={0,0,1};return true;}
bool swordHeld=false;bool manual_attack(daAlink_c*){return swordHeld;}
bool playing=true,targetExists=true;float targetDistance=300;
bool playable(){return playing;}bool in_arena(){return playing;}
bool normal_sword_proc(daAlink_c* p){return p->proc==1;}
bool sword_action_available(daAlink_c* p){return p->proc==0;}
struct{bool ready=true;}s_loot;
Run s_run;std::array<unsigned,12>s_cooldowns{};int s_player=1;
enum class SwordAction{None,Normal};SwordAction s_swordAction=SwordAction::Normal;
bool s_wavePending=false;unsigned s_comboStep=0,s_comboTimeout=0;
using ActorId=unsigned;constexpr ActorId kNone=~0u;
unsigned targetCount=1;
bool nearest_item_target(daAlink_c*,cXyz& target,float range,std::span<const ActorId> excluded={},ActorId* chosen=nullptr){
 for(unsigned i=0;i<targetCount;++i)if(std::find(excluded.begin(),excluded.end(),i)==excluded.end()){
  target={targetDistance+float(i*10),100,0};if(chosen)*chosen=i;return targetExists&&target.x<range;}
 return false;}
std::vector<ActorId> shotTargets;
constexpr unsigned kItemBegin=8,kSlotsPerItem=4;
struct Attack{bool active=false,returning=false;unsigned weapon=0,age=0,bowVolleySlot=20;uint64_t bowVolley=0;};std::deque<Attack>s_attacks(initial_projectile_slots);
std::vector<cXyz>destinations;std::vector<Upgrade>shots;std::vector<float>spread;
bool launch_item(daAlink_c*,Upgrade weapon,cXyz targetPosition,float angle=0,unsigned group=20,uint64_t volley=0,ActorId target=kNone,bool manual=false){
 assert(!manual||(s_run.options.manual_enabled(weapon)&&swordHeld));
 destinations.push_back(targetPosition);shotTargets.push_back(target);
 unsigned type=unsigned(weapon)-unsigned(Upgrade::Bow);
 auto& a=acquire_projectile(s_attacks,unsigned(weapon),item_projectile_begin(type),item_initial_slots(type));
 a.active=true;a.weapon=unsigned(weapon);a.bowVolleySlot=group;a.bowVolley=volley;shots.push_back(weapon);spread.push_back(angle);return true;}
'''
manual_source=(root/'src/survivors_manual_attack.inc').read_text()
start=manual_source.index('bool manual_skill(')
fixture+=manual_source[start:manual_source.index('\n}',start)+2]
main=r'''
void fresh(){targetCount=1;shotTargets.clear();targetDistance=300;player={};s_run.start(1);s_cooldowns={};s_ranged={};s_rangedRest=0;
 s_attacks=std::deque<Attack>(initial_projectile_slots);shots.clear();spread.clear();playing=targetExists=true;s_loot.ready=true;
 for(unsigned i=8;i<=11;++i)s_run.ranks[i]=1;}
int main(){
 // Each item independently opts into reticle aim or its original automatic gates.
 for(bool enabled:{false,true})for(bool held:{false,true})for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball}){
  fresh();swordHeld=held;s_run.options.manual_aim[unsigned(manual_ability(weapon))]=enabled;targetExists=false;
  if(held&&enabled){destinations.clear();assert(queue_ranged(&player,weapon));assert(destinations[0].z==900&&destinations[0].y==140&&spread[0]==0);assert(!queue_ranged(&player,weapon));continue;}
  assert(!queue_ranged(&player,weapon)&&!s_cooldowns[unsigned(weapon)]);
  targetExists=true;destinations.clear();assert(queue_ranged(&player,weapon));
  assert(destinations[0].x==targetDistance&&destinations[0].y==100&&destinations[0].z==0);
  assert(!queue_ranged(&player,weapon));
 }
 for(auto weapon:{Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball}) {
  fresh();swordHeld=true;targetExists=false;s_run.ranks[unsigned(Upgrade::Multishot)]=3;destinations.clear();
  assert(queue_ranged(&player,weapon)&&destinations.size()==4);
  for(const auto& target:destinations)assert(target.x==0&&target.y==140&&target.z==900);
 }
 swordHeld=false;

 for(auto weapon:{Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball})for(unsigned targets:{1u,2u,4u}){
  fresh();targetCount=targets;s_run.ranks[unsigned(Upgrade::Multishot)]=3;
  assert(queue_ranged(&player,weapon)&&shots.size()==4);
  for(unsigned i=0;i<targets;++i)assert(shotTargets[i]==i);
  for(unsigned i=targets;i<4;++i)assert(shotTargets[i]==0);
 }
 for(unsigned rank=0;rank<=3;++rank)for(unsigned bow:{1u,10u,20u})for(bool held:{false,true}){
  swordHeld=held;
  fresh();s_run.ranks[unsigned(Upgrade::Multishot)]=rank;s_run.ranks[unsigned(Upgrade::Bow)]=bow;
  assert(queue_ranged(&player,Upgrade::Bow)&&shots.size()==s_run.bow_shots());
  for(auto angle:spread)assert(std::abs(angle)<=s_run.bow_fan_degrees()*3.14159265f/360+.0001f);
  if(held)assert(spread[0]==0);
 }
 swordHeld=false;fresh();player.proc=1; // an active native sword swing does not block any item
 for(unsigned i=8;i<=11;++i)assert(queue_ranged(&player,Upgrade(i)));
 assert(shots.size()==4 && player.proc==1 && player.mEquipItem==0x103 && !s_ranged.active);
 for(unsigned i=8;i<=11;++i)assert(!queue_ranged(&player,Upgrade(i)));
 fresh();s_run.ranks[8]=10;
 assert(queue_ranged(&player,Upgrade::Bow));assert(shots.size()==3);
 assert(queue_ranged(&player,Upgrade::Boomerang));assert(queue_ranged(&player,Upgrade::Bombs));
 assert(queue_ranged(&player,Upgrade::Ball));assert(shots.size()==6);
 assert(player.mEquipItem==0x103 && s_ranged.active);
 player.proc=1;update_ranged(&player);assert(!s_ranged.active && player.proc==1);
 assert(shots.size()==6); // cosmetic cancellation never duplicates/cancels releases
 fresh();targetDistance=2300;s_run.ranks[unsigned(Upgrade::Reach)]=10;
 assert(queue_ranged(&player,Upgrade::Bow));assert(queue_ranged(&player,Upgrade::Boomerang));
 assert(!queue_ranged(&player,Upgrade::Bombs));assert(!queue_ranged(&player,Upgrade::Ball));
 // Full five-arrow fans persist beyond the old 20-arrow and 20-volley limits.
 fresh();s_run.ranks[8]=30;
 for(unsigned volley=0;volley<30;++volley) {
  s_cooldowns[8]=0;assert(queue_ranged(&player,Upgrade::Bow));
  assert(shots.size()==5*(volley+1));
  auto lo=*std::min_element(spread.end()-5,spread.end());
  auto hi=*std::max_element(spread.end()-5,spread.end());
  assert(std::abs(hi-lo-3.14159265f/4)<1.e-6f);
 }
 unsigned arrows=0;for(auto&a:s_attacks)if(a.active&&a.weapon==8)++arrows;
 assert(arrows==150&&s_attacks.back().bowVolleySlot==29);
 auto serial=s_attacks[8].bowVolley;
 for(auto&a:s_attacks)if(a.bowVolleySlot==0)a.active=false;
 s_cooldowns[8]=0;assert(queue_ranged(&player,Upgrade::Bow));
 assert(s_attacks[8].bowVolleySlot==0&&s_attacks[8].bowVolley!=serial);
 fresh();targetExists=false;assert(!queue_ranged(&player,Upgrade::Bow)&&!s_cooldowns[8]);
 targetExists=true;playing=false;assert(!queue_ranged(&player,Upgrade::Bow));
 playing=true;s_loot.ready=false;assert(!queue_ranged(&player,Upgrade::Bow));
 fresh();player.proc=2; // Jump Strike body pose remains untouched.
 assert(queue_ranged(&player,Upgrade::Bow)&&player.proc==2&&shots.size()==1);
 fresh();assert(queue_ranged(&player,Upgrade::Ball));int result=1;
 assert(before_ranged_native(nullptr,&player,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0);
 player.animation=99;update_ranged(&player);assert(!s_ranged.active&&player.animation==99);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+native_next+source+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors ranged: simultaneous releases, sword/jump overlap, range, cooldowns and cosmetic isolation passed')

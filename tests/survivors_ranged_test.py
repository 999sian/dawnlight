"""Production ranged animation scheduler: release frames, aim, interrupts and fairness."""
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
bool playing=true,targetExists=true;float targetDistance=300;
bool playable(){return playing;}bool in_arena(){return playing;}
bool normal_sword_proc(daAlink_c* p){return p->proc==1;}
bool sword_action_available(daAlink_c* p){return p->proc==0;}
struct{bool ready=true;}s_loot;
Run s_run;std::array<unsigned,12>s_cooldowns{};int s_player=1;
enum class SwordAction{None,Normal};SwordAction s_swordAction=SwordAction::Normal;
bool s_wavePending=false;unsigned s_comboStep=0,s_comboTimeout=0;
bool nearest_item_target(daAlink_c*,cXyz& target,float range){target={targetDistance,100,0};return targetExists&&targetDistance<range;}
constexpr unsigned kItemBegin=8,kSlotsPerItem=4;
struct Attack{bool active=false,returning=false;unsigned age=0;};std::array<Attack,projectile_capacity>s_attacks;
std::vector<Upgrade>shots;std::vector<float>spread;
bool launch_item(daAlink_c*,Upgrade weapon,cXyz,float angle=0){shots.push_back(weapon);spread.push_back(angle);if(weapon==Upgrade::Ball)s_attacks[item_projectile_begin(3)].active=true;return true;}
'''
main=r'''
void fresh(){targetDistance=300;player={};s_run.start(1);s_cooldowns={};s_ranged={};s_rangedRest=0;s_lastRanged=Upgrade::Boomerang;
 s_attacks={};shots.clear();spread.clear();playing=targetExists=true;s_loot.ready=true;s_run.ranks[8]=s_run.ranks[9]=1;}
void tick(){auto& f=player.mUpperFrameCtrl[2];f.frame=std::min(float(f.end),f.frame+f.rate);update_ranged(&player);}
int main(){
 // Range is checked at scheduling, aiming and release, without a short-range
 // weapon blocking the boomerang in round-robin order.
 fresh();targetDistance=2000;s_lastRanged=Upgrade::Ball;
 assert(!queue_ranged(&player,Upgrade::Boomerang));
 s_run.ranks[unsigned(Upgrade::Reach)]=10;
 assert(!queue_ranged(&player,Upgrade::Bow));assert(queue_ranged(&player,Upgrade::Boomerang));
 assert(s_ranged.target.x==2000);
 targetDistance=2500;before_ranged_aim(nullptr,&player,nullptr,nullptr);assert(s_ranged.target.x==2500);
 release_ranged(&player);assert(shots.size()==1&&shots[0]==Upgrade::Boomerang);
 targetDistance=4000;release_ranged(&player);assert(shots.size()==1);
 fresh();assert(queue_ranged(&player,Upgrade::Bow));
 assert(player.animation==dRes_ID_ALANM_BCK_ARELORD_e&&shots.empty()&&player.mEquipItem==dItemNo_NONE_e);
 assert(!queue_ranged(&player,Upgrade::Boomerang));
 before_ranged_aim(nullptr,&player,nullptr,nullptr);
 assert(player.shape_angle.y==0x4000&&player.mBodyAngle.x<0&&player.current.angle.y==123);
 s_ranged.weapon=Upgrade::Ball;before_ranged_aim(nullptr,&player,nullptr,nullptr);
 assert(player.mBodyAngle.x==0&&player.current.angle.y==123);s_ranged.weapon=Upgrade::Bow;
 int result=1;assert(before_ranged_native(nullptr,&player,&result,nullptr)==HOOK_SKIP_ORIGINAL&&result==0);
 for(unsigned i=0;i<11;++i){tick();assert(shots.empty());}
 tick();assert(shots.size()==1&&s_ranged.released&&player.animation==dRes_ID_ALANM_BCK_ASHOOT_e);
 for(unsigned i=0;i<30;++i){tick();}assert(shots.size()==1&&!s_ranged.active&&player.animation==0);
 fresh();s_run.ranks[8]=7;s_run.ranks[4]=5;
 assert(queue_ranged(&player,Upgrade::Bow));const auto accelerated=player.mUpperFrameCtrl[2].rate;
 assert(accelerated>1);for(unsigned i=0;i<20;++i)tick();
 assert(shots.size()==3&&spread[1]>0&&spread[2]<0);
 fresh();s_run.ranks[8]=30;
 assert(queue_ranged(&player,Upgrade::Bow));for(unsigned i=0;i<30;++i)tick();
 assert(shots.size()==10&&spread.size()==10);
 fresh();s_lastRanged=Upgrade::Bow;assert(queue_ranged(&player,Upgrade::Boomerang));
 player.mUpperFrameCtrl[2].frame=5;update_ranged(&player);assert(shots.empty());
 player.mUpperFrameCtrl[2].frame=8;update_ranged(&player);assert(shots.size()==1&&s_ranged.released);
 update_ranged(&player);assert(shots.size()==1); // Fast frames never double-release.
 fresh();assert(queue_ranged(&player,Upgrade::Bow));player.proc=2;update_ranged(&player);
 assert(!s_ranged.active&&shots.empty());
 fresh();assert(queue_ranged(&player,Upgrade::Bow));player.animation=99;update_ranged(&player);
 assert(player.animation==99&&!s_ranged.active); // Preserve another system's animation.
 fresh();assert(queue_ranged(&player,Upgrade::Bow));cancel_ranged();
 for(unsigned i=0;i<30;++i){tick();}assert(shots.empty()); // Opening a chest cannot fire a delayed shot.
 fresh();assert(queue_ranged(&player,Upgrade::Bow));targetExists=false;
 for(unsigned i=0;i<30;++i){tick();}assert(shots.empty());
 fresh();player.equipping=true;assert(!queue_ranged(&player,Upgrade::Bow));
 fresh();s_comboStep=2;s_comboTimeout=60;assert(queue_ranged(&player,Upgrade::Bow));
 assert(s_comboStep==2&&s_comboTimeout==60); // Automatic item poses preserve sword combo progress.
 fresh();player.proc=1;player.mUnderFrameCtrl[0].frame=4;assert(!queue_ranged(&player,Upgrade::Bow));
 player.mUnderFrameCtrl[0].frame=10;assert(queue_ranged(&player,Upgrade::Bow)&&player.proc==0);
 cancel_ranged();s_rangedRest=0;s_cooldowns={};
 assert(!queue_ranged(&player,Upgrade::Bow));assert(queue_ranged(&player,Upgrade::Boomerang));
 // Equal due times cannot starve any of the four weapons.
 fresh();s_run.ranks[8]=s_run.ranks[9]=0;s_run.ranks[10]=1;
 assert(queue_ranged(&player,Upgrade::Bombs)&&player.animation==dRes_ID_ALANM_BCK_BOMBTHROW_e);
 player.mUpperFrameCtrl[2].frame=3;update_ranged(&player);assert(shots.empty());
 player.mUpperFrameCtrl[2].frame=4;update_ranged(&player);assert(shots.size()==1&&shots[0]==Upgrade::Bombs);
 fresh();s_run.ranks[8]=s_run.ranks[9]=0;s_run.ranks[11]=1;
 assert(queue_ranged(&player,Upgrade::Ball)&&player.animation==dRes_ID_ALANM_BCK_IBTHROW_e);
 // Execute the real native next-action selector: it steals our upper animation
 // for a manually equipped iron ball and immediately cancels the run scheduler.
 player.checkNextActionIronBall();update_ranged(&player);
 assert(!s_ranged.active&&shots.empty());
 for(bool locked:{false,true}){
  fresh();s_run.ranks[8]=s_run.ranks[9]=0;s_run.ranks[11]=1;player.locked=locked;
  assert(queue_ranged(&player,Upgrade::Ball));
  for(int frame=0;frame<6;++frame){
   int nativeResult=1;
   if(before_ranged_native(nullptr,&player,&nativeResult,nullptr)==HOOK_CONTINUE)player.checkNextActionIronBall();
   assert(nativeResult==0&&player.proc==0);tick();assert(s_ranged.active);
  }
  assert(shots.size()==1&&shots[0]==Upgrade::Ball);
 }
 fresh();s_run.ranks[8]=s_run.ranks[9]=0;s_run.ranks[11]=1;
 assert(queue_ranged(&player,Upgrade::Ball));
 player.mUpperFrameCtrl[2].frame=5;update_ranged(&player);assert(shots.empty());
 player.mUpperFrameCtrl[2].frame=6;update_ranged(&player);assert(shots.size()==1);
 s_attacks[item_projectile_begin(3)].returning=true;update_ranged(&player);assert(player.animation==dRes_ID_ALANM_BCK_IBPULL_e);
 s_attacks[item_projectile_begin(3)].active=false;update_ranged(&player);assert(player.animation==dRes_ID_ALANM_BCK_IBCATCH_e);
 for(unsigned i=0;i<30;++i){tick();}assert(!s_ranged.active&&shots.size()==1);
 fresh();for(unsigned i=8;i<12;++i)s_run.ranks[i]=1;s_lastRanged=Upgrade::Ball;
 for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball}){
  assert(queue_ranged(&player,weapon));cancel_ranged();s_rangedRest=0;s_cooldowns={};
 }

}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+native_next+source+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Survivors ranged: native animation phases, release timing, haste, aim, cancellation and fairness passed')

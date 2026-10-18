"""Run combat ledge steering through the pinned native cliff/terrain decision."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
native=(root/'dusklight/src/d/actor/d_a_alink.cpp').read_text()
a=native.index('BOOL daAlink_c::checkAutoJumpAction()')
check=native[a:native.index('\nBOOL daAlink_c::checkCutJumpInFly()',a)]
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;using f32=float;using BOOL=int;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
struct cXyz {float x=0,y=0,z=0;void set(float a,float b,float c){x=a;y=b;z=c;}s16 atan2sX_Z(){return 0;}};
struct cM3dGPla{cXyz mNormal{0,0,1};};
float JMAFastSqrt(float x){return std::sqrt(x);}
#define SQUARE(x) ((x)*(x))
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793);}
int cLib_distanceAngleS(s16 a,s16 b){return std::abs(int(s16(a-b)));}
float cM_ssin(s16 a){return std::sin(a*3.141592653589793/32768);}
float cM_scos(s16 a){return std::cos(a*3.141592653589793/32768);}
bool cBgW_CheckBWall(float){return true;}
struct Gnd{bool hit=false;float height=-200;bool ChkGroundHit(){return hit;}void ClrGroundHit(){hit=false;}float GetGroundH(){return height;}void SetPos(cXyz*){}};
struct Line{cXyz cross; cXyz& GetCross(){return cross;}cXyz* GetCrossP(){return &cross;}};
struct Bg{float GroundCross(Gnd*){return 0;}template<class T>bool GetTriPla(T&,cM3dGPla*){return false;}int GetWallCode(Line&){return 2;}}bg;
Bg& dComIfG_Bgsp(){return bg;}
unsigned life=12;unsigned dComIfGs_getLife(){return life;}bool dComIfGs_checkBottle(int){return false;}
constexpr int dItemNo_FAIRY_e=1;constexpr float l_autoDownHeight=-30;
struct daPy_demo_c{enum{DEMO_TYPE_SPECIAL_e=3};};
struct daAlink_c {
 enum{PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,PROC_WAIT_TURN,PROC_MOVE_TURN,
 PROC_CUT_NORMAL,PROC_CUT_TURN,PROC_GUARD_SLIP,PROC_AUTO_JUMP,PROC_FALL,PROC_DAMAGE,PROC_DOOR_OPEN,PROC_WARP,
 PROC_WOLF_LOCK_ATTACK,PROC_WOLF_LOCK_ATTACK_TURN,PROC_WOLF_GIANT_PUZZLE,PROC_FRONT_ROLL,PROC_CROUCH};
 enum{UPPER_2,FLG0_WATER_IN_MOVE=1,FLG0_UNK_2000=2,ERFLG0_NOT_AUTO_JUMP=1,ERFLG0_FORCE_AUTO_JUMP=2,ERFLG0_NOT_HANG=4,ERFLG0_UNK_100=8};
 static constexpr unsigned MODE_PLAYER_FLY=0x70C52;
 int mProcID=PROC_MOVE;unsigned id=7,mGndPolyAtt1=0,mMode=0,field_0x3178=0,flags=0,modeFlags=0;
 bool wolf=false,heavy=false,noJumpStage=false,ability=false;
 struct{cXyz pos;struct{s16 y=0;}angle;}current;
 struct{s16 y=0;}shape_angle; s16 mMoveAngle=0;
 float mMoveValue=1,mStickValue=1,mNormalSpeed=20,speedF=20,mWaterY=-1000,field_0x3428=0;
 cXyz speed{0,0,20},field_0x35c4,mWindSpeed,field_0x3594,field_0x3540,field_0x3798;
 Gnd mLinkAcch,mLinkGndChk;Line mLinkLinChk;
 struct{int field_0x3012=0;}mProcVar5;
 struct{int getDemoType(){return 0;}}mDemo;
 struct{void SetPolyInfo(Line&){}}mPolyInfo1;
 struct Params{float mJumpLimitSpeed=5,mWeakJumpLimitSpeed=5,mJumpSpeedLimit=5,mFallInterpolation=5,mNormalFallInterp=5,hang_foot_pos_height=30;};
 struct Hio{struct{Params m;}mAutoJump,mWallHang;struct{struct{Params m;}mWlAutoJump;}mWolf;}hio;
 Hio* mpHIO=&hio;
 bool checkModeFlg(unsigned n){return modeFlags&n;}bool checkWolf(){return wolf;}
 bool checkNoResetFlg0(unsigned){return false;}bool checkEndResetFlg0(unsigned n){return flags&n;}
 bool checkMagneBootsFly(){return false;}bool checkFlyAtnWait(){return false;}bool getSumouMode(){return false;}
 bool checkMagneBootsOn(){return false;}bool checkFmChainGrabAnime(){return false;}bool checkWolfDashMode(){return false;}
 bool checkIronBallWaitAnime(){return false;}bool checkGoronSideMove(){return false;}bool checkNotAutoJumpStage(){return noJumpStage;}
 bool checkBootsOrArmorHeavy(){return heavy;}bool checkGrabAnime(){return false;}bool checkGrabGlide(){return false;}
 bool commonLineCheck(cXyz*,cXyz*){return false;}bool checkClimbCode(Line&){return false;}bool cancelMagneBootsOn(){return false;}
 bool getWallEdgePos(cXyz&,cM3dGPla*,cM3dGPla*,cXyz*,int){return false;}
 void resetUpperAnime(int,float){assert(false);}
 int procAutoJumpInit(int){mProcID=PROC_AUTO_JUMP;current.angle.y=shape_angle.y;mNormalSpeed=15;return 1;}
 int procFallInit(int,float){mProcID=PROC_FALL;return 1;}
 int procSwimWaitInit(int){return 1;}int procWolfAutoJumpInit(int){assert(false);return 1;}
 int procWolfFallInit(int,float){assert(false);return 1;}int procDiveJumpInit(){return procAutoJumpInit(0);}
 int procWolfHangFallStartInit(cM3dGPla*){return 0;}int procHangFallStartInit(cM3dGPla*){return 0;}
 int procClimbDownStartInit(s16){return 0;}int procRollJumpInit(){return 0;}
 BOOL checkAutoJumpAction();
};
unsigned fopAcM_GetID(daAlink_c* p){return p->id;}
bool playing=true;bool playable(){return playing;}unsigned s_player=7;Run s_run;
bool jump_abilities_busy_for(daAlink_c* p){return p->ability;}
bool auto_movement_action(daAlink_c* p){return p->mProcID==daAlink_c::PROC_CUT_NORMAL||p->mProcID==daAlink_c::PROC_CUT_TURN||p->mProcID==daAlink_c::PROC_GUARD_SLIP;}
'''
main=r'''
int main(){
 s_run.start(1);
 for(int proc:{daAlink_c::PROC_MOVE,daAlink_c::PROC_ATN_ACTOR_MOVE,daAlink_c::PROC_CUT_NORMAL,daAlink_c::PROC_CUT_TURN,daAlink_c::PROC_GUARD_SLIP})
 for(s16 facing:{s16(0x4000),s16(-0x4000),s16(0x7000)}){
  daAlink_c link;link.mProcID=proc;link.shape_angle.y=facing;
  assert(link.checkAutoJumpAction()&&link.mProcID==daAlink_c::PROC_FALL); // Reproduce the native failure.
  link.mProcID=proc;link.mNormalSpeed=0;link.mStickValue=.2f;std::any args[]={&link};
  before_survivors_ledge_jump(nullptr,args,nullptr,nullptr);link.checkAutoJumpAction();after_survivors_ledge_jump(nullptr,nullptr,nullptr,nullptr);
  assert(link.mProcID==daAlink_c::PROC_AUTO_JUMP&&link.current.angle.y==0&&link.shape_angle.y==0);
  assert(link.mNormalSpeed==15&&link.mStickValue==.2f&&s_ledgeJumpScopes.empty());
 }
 // Restore facing/recoil when still over shallow ground; keep all native vetoes.
 for(int reason=0;reason<9;++reason){
  daAlink_c link;link.shape_angle.y=0x4000;link.mNormalSpeed=9;link.mStickValue=.3f;
  if(reason==0)link.mLinkAcch.height=-10;
  if(reason==1)link.flags=daAlink_c::ERFLG0_NOT_AUTO_JUMP;
  if(reason==2)link.heavy=true;
  if(reason==3)link.noJumpStage=true;
  if(reason==4)link.speedF=2;
  if(reason==5)link.mMoveValue=.2f;
  if(reason==6)playing=false;
  if(reason==7)link.ability=true;
  if(reason==8)link.mProcID=daAlink_c::PROC_DAMAGE;
  std::any args[]={&link};before_survivors_ledge_jump(nullptr,args,nullptr,nullptr);
  link.checkAutoJumpAction();after_survivors_ledge_jump(nullptr,nullptr,nullptr,nullptr);
  assert(link.mProcID!=daAlink_c::PROC_AUTO_JUMP&&link.mStickValue==.3f);
  if(reason==0)assert(link.shape_angle.y==0x4000&&link.mNormalSpeed==9);
  playing=true;
 }
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+check+(root/'src/survivors_ledge_jump.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-variable','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors ledge jumps: native backward/sideways checks, launch direction, scoped restoration and vetoes passed')

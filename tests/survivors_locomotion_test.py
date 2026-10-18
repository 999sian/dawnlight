"""Directional lower-body presentation without changing native attack tracks."""
from pathlib import Path
import os, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <any>
#include <cassert>
#include <optional>
#include <vector>
#include "survivors_locomotion.hpp"
#include "dual_wield_math.hpp"
using namespace dawnlight;using namespace dawnlight::survivors;
using u16=unsigned short;using s16=short;using ActorId=unsigned;
constexpr unsigned kNone=~0u;
struct cXyz {float x=0,y=0,z=0;cXyz operator-(cXyz p)const{return{x-p.x,y-p.y,z-p.z};}
 float absXZ()const{return std::hypot(x,z);}};
struct J3DTransformInfo{cXyz mScale{1,1,1};struct{s16 x=0,y=0,z=0;}mRotation;cXyz mTranslate;};
struct J3DAnmTransform{unsigned field_0x1e=35;float frame=0;int index=0;
 virtual ~J3DAnmTransform()=default;virtual void getTransform(u16 j,J3DTransformInfo* out)const{
  *out={};out->mTranslate={float(index),float(j),frame};out->mRotation.y=s16(index*100);}
 float getFrameMax()const{return 24;}void setFrame(float f){frame=f;}};
struct J3DAnmTransformKey:J3DAnmTransform{};
struct J3DMtxCalc{virtual ~J3DMtxCalc()=default;};
struct J3DJoint{J3DMtxCalc* calc=nullptr;J3DMtxCalc* getMtxCalc(){return calc;}void setMtxCalc(J3DMtxCalc* c){calc=c;}};
struct J3DModelData{unsigned count=35;std::array<J3DJoint,35> joints;
 unsigned getJointNum(){return count;}J3DJoint* getJointNodePointer(unsigned j){return &joints[j];}};
struct J3DModel{J3DModelData data;J3DModelData* getModelData(){return &data;}};
struct OldFrame{bool valid=true;dual::Quat root;bool getOldFrameFlg(){return valid;}
 dual::Quat* getOldFrameQuaternion(unsigned){return &root;}};
struct mDoExt_AnmRatioPack{J3DAnmTransform* clip=nullptr;float weight=0;
 void setAnmTransform(J3DAnmTransform* c){clip=c;}void setRatio(float w){weight=w;}};
struct mDoExt_MtxCalcAnmBlendTblOld:J3DMtxCalc{OldFrame* old; mDoExt_AnmRatioPack* pack;
 mDoExt_MtxCalcAnmBlendTblOld(OldFrame* o,int,mDoExt_AnmRatioPack* p):old(o),pack(p){}
 J3DAnmTransform* getAnm(int i){return i?nullptr:pack->clip;}float getRatio(int){return 0;}};
struct JKRSolidHeap{};JKRSolidHeap* heap=nullptr;unsigned allocations=0,destroys=0,loads=0;
bool failHeap=false,failClip=false;
JKRSolidHeap* mDoExt_createSolidHeapFromGame(int,int){if(failHeap)return nullptr;++allocations;return new JKRSolidHeap;}
JKRSolidHeap* mDoExt_setCurrentHeap(JKRSolidHeap* h){auto* previous=heap;heap=h;return previous;}
std::vector<J3DAnmTransform*> loaded;
void mDoExt_destroySolidHeap(JKRSolidHeap* h){++destroys;for(auto* clip:loaded)delete clip;loaded.clear();delete h;}
void mDoExt_adjustSolidHeap(JKRSolidHeap*){}
bool archive=true;void* dComIfGp_getAnmArchive(){return archive?reinterpret_cast<void*>(1):nullptr;}
struct daAlink_c{
 enum daAlink_ANM{ANM_ATN_RETURN_FROM_WALK,ANM_ATN_WALK_LEFT,ANM_ATN_BACK_WALK,ANM_ATN_WALK_RIGHT,
 ANM_ATN_RETURN_FROM_RUN,ANM_ATN_RUN_LEFT,ANM_ATN_BACK_RUN,ANM_ATN_RUN_RIGHT};
 struct Bck{u16 m_underID;}bck;
 Bck* getMainBckData(daAlink_ANM a){bck.m_underID=u16(a);return &bck;}
 void* loadAram(u16 id,unsigned){++loads;if(failClip)return nullptr;auto* p=new J3DAnmTransform;p->index=id+1;loaded.push_back(p);return p;}
 bool wolf=false,horse=false,allowed=true,locomotion=true,cut=false,shield=false;
 unsigned id=7;struct{cXyz pos;}current,old;struct{s16 y=0;}shape_angle;
 float mMoveValue=1,mMaxSpeed=20;
 struct{struct{struct{float mMaxSpeed=20,mWalkChangeRate=.3f,mRunChangeRate=.8f,mWalkAnmSpeed=1,mRunAnmSpeed=1;}m;}mMove;}hio;
 decltype(hio)* mpHIO=&hio;OldFrame cache;OldFrame* field_0x2060=&cache;
 J3DAnmTransform native; mDoExt_AnmRatioPack nativePack;
 mDoExt_MtxCalcAnmBlendTblOld nativeCalc{&cache,1,&nativePack};
 mDoExt_MtxCalcAnmBlendTblOld* field_0x1f20=&nativeCalc;
 J3DModel model;J3DModel* mpLinkModel=&model;
 daAlink_c(){nativePack.clip=&native;for(auto& j:model.data.joints)j.calc=&nativeCalc;}
 bool checkWolf(){return wolf;}bool checkHorseRide(){return horse;}
};
struct ModContext{};enum HookAction{HOOK_CONTINUE};
namespace mods{template<class T>T arg(void* a,unsigned i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}}
unsigned s_player=7;struct{unsigned ticks=0;}s_run;
enum class SwordAction{None,Normal,Spin,Jump};SwordAction s_swordAction=SwordAction::None;
bool auto_movement(daAlink_c* l){return l&&l->id==s_player&&l->allowed;}
bool sword_action_available(daAlink_c* l){return l->locomotion;}
bool normal_sword_proc(daAlink_c* l){return l->cut;}
bool shield_pose(daAlink_c* l){return l->shield;}
unsigned fopAcM_GetID(daAlink_c* l){return l->id;}
s16 cM_atan2s(float x,float z){return s16(std::atan2(x,z)*32768/3.141592653589793f);}
'''
helpers = (root/'src/dual_wield_animation.hpp').read_text()
helpers = helpers[helpers.index('inline dual::Quat joint_rotation'):helpers.index('inline void pose_jump_charge_arm')]
main = r'''
int main(){
 for(unsigned angle=0;angle<65536;++angle){auto w=locomotion_weights(s16(angle));float sum=0;
  for(auto v:w){assert(v>=0&&v<=1);sum+=v;}assert(std::abs(sum-1)<1e-6);}
 for(unsigned i=0;i<4;++i){auto w=locomotion_weights(s16(i*16384));assert(w[i]==1);
  w=locomotion_weights(s16(i*16384+8192));assert(w[i]==.5f&&w[(i+1)%4]==.5f);}
 assert(locomotion_run_blend(3,20,.3f,.8f)==0&&locomotion_run_blend(20,20,.3f,.8f)==1);
 daAlink_c link;std::any args[]={&link,&link.model};
 auto sample=[&](float x,float z){link.current.pos={x,0,z};after_locomotion_position(nullptr,args,nullptr,nullptr);};
 auto begin=[&](){before_locomotion_model(nullptr,args,nullptr,nullptr);};
 auto end=[&](){after_locomotion_model(nullptr,args,nullptr,nullptr);};
 // Cold-load once; preserve root/upper calculators and restore the pelvis.
 sample(0,20);begin();assert(loads==8&&allocations==1&&heap==nullptr);
 assert(link.model.data.joints[16].calc!=&link.nativeCalc);
 assert(link.model.data.joints[0].calc==&link.nativeCalc&&link.model.data.joints[1].calc==&link.nativeCalc);
 auto phase=s_locomotion.phase;auto* borrowed=link.model.data.joints[16].calc;
 std::any nested[]={&link,static_cast<J3DModel*>(nullptr)};
 before_locomotion_model(nullptr,nested,nullptr,nullptr);after_locomotion_model(nullptr,nested,nullptr,nullptr);
 assert(link.model.data.joints[16].calc==borrowed);end();assert(link.model.data.joints[16].calc==&link.nativeCalc);
 begin();assert(s_locomotion.phase==phase&&loads==8);end(); // repeated calc is not an extra step
 for(unsigned angle=0;angle<65536;angle+=8192){++s_run.ticks;link.shape_angle.y=s16(-int(angle));sample(0,20);begin();
  auto w=locomotion_weights(s16(angle));float expected=0;for(unsigned d=0;d<4;++d)expected+=w[d]*(d+5);
  assert(std::abs(s_locomotion.pose.joints[17].mTranslate.x-expected)<.001f);end();}
 // Sword root twists are cancelled at the pelvis; native clocks/packs remain.
 link.locomotion=false;link.cut=true;s_swordAction=SwordAction::Normal;
 ++s_run.ticks;sample(0,20);begin();s_locomotion.pose.weight=1;
 link.cache.root=dual::from_euler({.3f,.7f,-.2f});J3DTransformInfo pelvis;
 s_locomotion.pose.getTransform(16,&pelvis);
 auto actual=dual::multiply(link.cache.root,joint_rotation(pelvis));
 auto expected=dual::multiply(joint_rotation(s_locomotion.pose.joints[0]),joint_rotation(s_locomotion.pose.joints[16]));
 auto a=dual::rotate(actual,{0,0,1}),b=dual::rotate(expected,{0,0,1});assert(dual::length(a-b)<.001f);
 assert(link.native.frame==0&&link.nativePack.clip==&link.native);end();
 // Standstill, teleports, disabled auto movement (flight/damage/events/pause),
 // spin ownership, wolf, riding and other rigs keep their native poses.
 for(unsigned reason=0;reason<9;++reason){
  link.allowed=reason!=0;link.wolf=reason==1;link.horse=reason==2;
  link.model.data.count=reason==3?40:35;link.cut=reason!=4;
  s_swordAction=reason==4?SwordAction::Spin:SwordAction::Normal;
  link.mMoveValue=reason==5?.01f:1;link.cache.valid=reason!=8;
  ++s_run.ticks;sample(0,reason==6?0:reason==7?1000:20);begin();assert(!s_locomotion.joint);end();
 }
 link.allowed=true;link.wolf=link.horse=false;link.cut=false;link.locomotion=true;link.cache.valid=true;
 ++s_run.ticks;sample(0,20);begin();clear_locomotion();assert(link.model.data.joints[16].calc==&link.nativeCalc);
 assert(allocations==destroys&&loaded.empty());clear_locomotion();assert(allocations==destroys);
 failHeap=true;sample(0,20);begin();assert(!s_locomotion.joint);end();clear_locomotion();failHeap=false;
 failClip=true;sample(0,20);begin();assert(!s_locomotion.joint&&allocations==destroys);end();clear_locomotion();
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture+helpers+(root/'src/survivors_locomotion.inc').read_text()+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Survivors locomotion: direction blends, independent legs, root compensation, ownership, restoration and cleanup passed')

"""Production pad swap, Palace camera selection and touch image ownership."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u32=unsigned;
constexpr unsigned PAD_1=0,PAD_BUTTON_B=0x200,PAD_TRIGGER_L=0x40,PAD_BUTTON_A=0x100;
struct Pad{u32 mButtonFlags=0,mPressedButtonFlags=0,mHoldLockL=0,mTrigLockL=0;float mTriggerLeft=0;}pad;
struct mDoCPd_c{static Pad& getCpadInfo(unsigned port){assert(port==0);return pad;}};
struct ModContext{};enum HookAction{HOOK_CONTINUE};
namespace mods{template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}}
Run s_run;bool playing=true,arenaActive=true,event=false;
bool playable(){return playing;}bool in_arena(){return arenaActive;}bool dComIfGp_event_runCheck(){return event;}
struct Arena{const char* stage="D_MN08";}map;
const Arena& arena(){return map;}
struct stage_camera2_data_class{int field_0x16=13;};
struct dCamera_c{unsigned mPadID=0;int mCamTypeNum=20,normalType=4,mMapToolType=13;
 struct Tool{stage_camera2_data_class mCamData;int mCameraIndex=1;void* mpActor=nullptr;}mStageCamTool,mRoomMapTool,mDefRoomCamTool,mTagCamTool;
 int GetCameraTypeFromCameraName(const char* name){assert(std::string_view(name)=="DungeonS");return normalType;}
};
namespace dusk::ui{enum class Control{B,L};}
namespace Rml{
using String=std::string;
struct Element{Element* parent=nullptr;bool hidden=false,visible=false,hasItem=false;std::vector<std::unique_ptr<Element>> children;};
using ElementPtr=std::unique_ptr<Element>;
}
using Rml::Element;
struct NativeTouch{bool mWasSuppressed=false;struct Elements{Element* root=nullptr;Element* icon=nullptr;};std::array<Elements,2>mControlElements;};
Element* append(Element* parent,Rml::ElementPtr child,bool){auto* raw=child.get();raw->parent=parent;parent->children.push_back(std::move(child));return raw;}
Rml::ElementPtr remove_child(Element* parent,Element* child){
 for(auto it=parent->children.begin();it!=parent->children.end();++it)if(it->get()==child){auto p=std::move(*it);parent->children.erase(it);p->parent=nullptr;return p;}
 return {};}
struct TouchApi{
 Element* (*parent)(const Element*)=[](const Element* e){return e->parent;};
 decltype(&::append) append=::append;decltype(&::remove_child) remove=::remove_child;
 Element* (*child)(const Element*,int)=[](const Element* e,int i)->Element*{return size_t(i)<e->children.size()?e->children[i].get():nullptr;};
 void (*setClass)(Element*,const std::string*,bool)=[](Element* e,const std::string* n,bool b){assert(*n=="has-item");e->hasItem=b;};
 bool (*isClassSet)(const Element*,const std::string*)=[](const Element* e,const std::string* n){return *n=="visible"?e->visible:e->hasItem;};
 void (*setPseudoClass)(Element*,const std::string*,bool)=[](Element* e,const std::string* n,bool b){assert(*n=="hidden");e->hidden=b;};
 bool (*isPseudoClassSet)(const Element*,const std::string*)=[](const Element* e,const std::string* n){assert(*n=="hidden");return e->hidden;};
}s_touchApi;
bool s_extraHooksReady=true;
'''
core=(root/'src/survivors.cpp').read_text()
start=core.index('bool survivors_switch_attack_lock()')
gate=core[start:core.index('\n}',start)+2]
main=r'''
int main(){
 s_run.start(1);assert(!survivors_switch_attack_lock());s_run.options.switch_attack_lock=true;
 // Physical L, including analog-only input, becomes B with one trigger edge.
 for(int i=0;i<10;++i){
  before_survivors_pad(nullptr,nullptr,nullptr,nullptr);
  bool old=pad.mHoldLockL;pad.mButtonFlags=PAD_BUTTON_A;pad.mPressedButtonFlags=0;
  pad.mTriggerLeft=1;pad.mHoldLockL=1;pad.mTrigLockL=!old;
  after_survivors_pad(nullptr,nullptr,nullptr,nullptr);
  assert(pad.mButtonFlags==(PAD_BUTTON_A|PAD_BUTTON_B)&&!pad.mHoldLockL&&pad.mTriggerLeft==0);
  assert(bool(pad.mPressedButtonFlags&PAD_BUTTON_B)==(i==0));
 }
 before_survivors_pad(nullptr,nullptr,nullptr,nullptr);pad={PAD_BUTTON_B,PAD_BUTTON_B,0,0,0};
 after_survivors_pad(nullptr,nullptr,nullptr,nullptr);
 assert(pad.mButtonFlags==PAD_TRIGGER_L&&pad.mPressedButtonFlags==PAD_TRIGGER_L&&pad.mHoldLockL&&pad.mTrigLockL&&pad.mTriggerLeft==1);
 before_survivors_pad(nullptr,nullptr,nullptr,nullptr);assert(!pad.mHoldLockL);
 // Both held, release, disabled and modal/menu states.
 pad={PAD_BUTTON_B|PAD_TRIGGER_L,PAD_BUTTON_B|PAD_TRIGGER_L,1,1,1};after_survivors_pad(nullptr,nullptr,nullptr,nullptr);
 assert(pad.mButtonFlags==(PAD_BUTTON_B|PAD_TRIGGER_L)&&pad.mHoldLockL);
 before_survivors_pad(nullptr,nullptr,nullptr,nullptr);pad={};after_survivors_pad(nullptr,nullptr,nullptr,nullptr);assert(!pad.mButtonFlags&&!pad.mHoldLockL);
 for(int gate=0;gate<3;++gate){before_survivors_pad(nullptr,nullptr,nullptr,nullptr);playing=gate!=0;s_run.phase=gate==1?Phase::Upgrade:Phase::Running;s_run.options.switch_attack_lock=gate!=2;
  pad={PAD_BUTTON_B,PAD_BUTTON_B,0,0,0};after_survivors_pad(nullptr,nullptr,nullptr,nullptr);assert(pad.mButtonFlags==PAD_BUTTON_B&&!pad.mHoldLockL);}
 playing=true;s_run.phase=Phase::Running;s_run.options.switch_attack_lock=true;
 dCamera_c camera;
 std::any cameraArgs[]={&camera};
 for(int gate=0;gate<6;++gate){
  camera={};event=gate==0;arenaActive=gate!=1;map.stage=gate==2?"D_DLBR0":"D_MN08";
  camera.mPadID=gate==3?1:0;camera.normalType=gate==4?255:4;
  after_survivors_camera_tools(nullptr,cameraArgs,nullptr,nullptr);
  const int expected=gate==5?4:13;
  assert(camera.mStageCamTool.mCamData.field_0x16==expected && camera.mMapToolType==expected);
  assert(camera.mRoomMapTool.mCamData.field_0x16==expected && camera.mDefRoomCamTool.mCamData.field_0x16==expected);
  assert(camera.mTagCamTool.mCamData.field_0x16==13);
 }
 camera={};camera.mRoomMapTool.mpActor=&camera;camera.mDefRoomCamTool.mCameraIndex=0xff;
 after_survivors_camera_tools(nullptr,cameraArgs,nullptr,nullptr);
 assert(camera.mRoomMapTool.mCamData.field_0x16==13 && camera.mDefRoomCamTool.mCamData.field_0x16==13);
 camera.mRoomMapTool.mpActor=nullptr;camera.mRoomMapTool.mCameraIndex=0x1ff;
 after_survivors_camera_tools(nullptr,cameraArgs,nullptr,nullptr);assert(camera.mRoomMapTool.mCamData.field_0x16==13);
 Element b,l;auto* icon=append(&b,std::make_unique<Element>(),true);auto* label=append(&l,std::make_unique<Element>(),true);
 icon->visible=true;b.hasItem=true;NativeTouch touch;touch.mControlElements={NativeTouch::Elements{&b,icon},NativeTouch::Elements{&l,nullptr}};
 for(int frame=0;frame<100;++frame){
  move_survivors_touch_sword(&touch);assert(icon->parent==&l&&!label->parent&&!b.hasItem);
  restore_survivors_touch_sword();assert(icon->parent==&b&&!label->hidden&&b.hasItem&&b.children.size()==1&&l.children.size()==1);
 }
 move_survivors_touch_sword(&touch);s_run.options.switch_attack_lock=false;move_survivors_touch_sword(&touch);assert(icon->parent==&b&&!label->hidden);
 s_run.options.switch_attack_lock=true;touch.mWasSuppressed=true;move_survivors_touch_sword(&touch);assert(icon->parent==&b);
 touch.mWasSuppressed=false;label->hidden=true;move_survivors_touch_sword(&touch);restore_survivors_touch_sword();assert(label->hidden);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+gate+(root/'src/survivors_controls.inc').read_text()+(root/'src/survivors_touch_sword.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I'+str(root/'src'),'-I'+str(root/'include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors controls: swap edges/holds/gates, Palace base camera, touch sword movement/restoration passed')

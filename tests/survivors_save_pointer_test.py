"""Exercise native save cursor/pointer disagreement with the production adapter."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <any>
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
using ActorId=int;using ModResult=int;
constexpr int kNone=-1,MOD_OK=0,MOD_UNAVAILABLE=1;
struct ModContext{};struct ModError{std::string message;};
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{
template<class T>T arg(void* p,unsigned n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}
int set_error(ModError* e,int r,const char* text){if(e)e->message=text;return r;}
}
namespace dusk::menu_pointer{
struct State{float x=0,y=0;bool valid=false,down=false,pressed=false,released=false,clicked=false,touch=false;};
}
dusk::menu_pointer::State pointer;
const dusk::menu_pointer::State& pointer_state()noexcept{return pointer;}
int resolveMode=0;
struct HookService{int resolve(void*,const char* name,void** out,void*){
 assert(std::string(name)=="dusk::menu_pointer::state");
 *out=resolveMode==2?nullptr:reinterpret_cast<void*>(&pointer_state);return resolveMode==1?MOD_UNAVAILABLE:MOD_OK;
}}hookService;
auto* svc_hook=&hookService;void* mod_ctx=nullptr;
struct STControl{enum{TRIG_UP=4,TRIG_DOWN=8};unsigned mDirectionTrig=0;};
struct dMsgScrn3Select_c{unsigned char mDPDPoint=0xff;int requestedPane=1;};
struct dMsgScrnExplain_c{
 enum{STATUS_MOVE_SELECT_e=4};unsigned char status=4,mSelCursor=0;
 STControl* mpStick=nullptr;dMsgScrn3Select_c* mpSelect_c=nullptr;
 unsigned getStatus(){return status;}
};
struct dMenu_save_c{unsigned status=2;dMsgScrnExplain_c* mpScrnExplain=nullptr;unsigned getSaveStatus(){return status;}};
bool arena=true;bool in_arena(){return arena;}
ActorId s_savePrompt=42;std::vector<dMenu_save_c*> s_nativeSaveScreens;
'''
main = r'''
int main(){
 ModError error;
 for(resolveMode=1;resolveMode<=2;++resolveMode){
  assert(initialize_save_pointer(&error)==MOD_UNAVAILABLE&&!s_savePointerState);
  assert(error.message=="failed to resolve native save dialog pointer state");
 }
 resolveMode=0;assert(initialize_save_pointer(&error)==MOD_OK&&s_savePointerState);
 STControl stick;dMsgScrn3Select_c selection,foreign;
 dMsgScrnExplain_c explain;explain.mpStick=&stick;explain.mpSelect_c=&selection;
 dMenu_save_c save;save.mpScrnExplain=&explain;s_nativeSaveScreens={&save};
 unsigned hovered=0,nativeMoves=0;
 auto frame=[&](unsigned direction){
  stick.mDirectionTrig=direction;
  // Native Explain processes D-pad/stick before requesting the animated pane.
  if(direction==STControl::TRIG_DOWN)explain.mSelCursor=1;
  if(direction==STControl::TRIG_UP)explain.mSelCursor=0;
  selection.requestedPane=explain.mSelCursor+1;
  std::any args[]={&selection};bool result=false;
  if(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE){
   // Native pointerMove overrides the requested pane, independently of Explain.
   ++nativeMoves;result=hovered<2;selection.mDPDPoint=hovered;
   if(result)selection.requestedPane=int(hovered)+1;
  }
  after_save_pointer(nullptr,args,&result,nullptr);
 };
 pointer={100,100,true};frame(0);assert(explain.mSelCursor==0&&selection.requestedPane==1);
 // Stationary hover over Yes used to undo the visible Down-to-No transition.
 frame(STControl::TRIG_DOWN);assert(explain.mSelCursor==1&&selection.requestedPane==2);
 const auto before=nativeMoves;
 for(unsigned i=0;i<60;++i){frame(0);assert(explain.mSelCursor==1&&selection.requestedPane==2);}
 assert(nativeMoves==before);
 // A/confirm would now follow the native No branch, without writing a save.
 bool wrote=explain.mSelCursor==0;assert(!wrote);
 frame(STControl::TRIG_UP);frame(0);assert(explain.mSelCursor==0&&selection.requestedPane==1);
 // No hover left behind by a released touch can steal controller selection.
 pointer.valid=false;frame(STControl::TRIG_DOWN);frame(0);assert(explain.mSelCursor==1);
 pointer.valid=true;frame(0);assert(explain.mSelCursor==1&&selection.requestedPane==2);
 // Actual new mouse movement takes over again, then real touch selects No.
 pointer.x=120;frame(0);assert(explain.mSelCursor==0&&selection.requestedPane==1);
 hovered=1;pointer.y=200;pointer.pressed=pointer.touch=true;frame(0);
 assert(explain.mSelCursor==1&&selection.requestedPane==2);
 pointer.pressed=false;pointer.clicked=pointer.released=true;frame(0);
 assert(explain.mSelCursor==1); // the next native confirmation chooses No
 // Controller wins when a touch D-pad also produces a pointer event.
 hovered=0;frame(STControl::TRIG_DOWN);assert(explain.mSelCursor==1&&selection.requestedPane==2);
 pointer.clicked=pointer.released=false;frame(0);assert(explain.mSelCursor==1);
 // New post-run prompt, even with reused heap addresses, gets fresh ownership.
 ++s_savePrompt;frame(0);assert(explain.mSelCursor==0&&selection.requestedPane==1);
 std::any args[]={&foreign};bool result=false;
 assert(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
 // Every other game dialog, native save and loading/closing state is untouched.
 args[0]=&selection;
 arena=false;assert(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);arena=true;
 s_savePrompt=kNone;assert(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);s_savePrompt=44;
 for(unsigned status:{0u,1u,3u}){save.status=status;assert(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);}
 save.status=2;explain.status=5;assert(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
 explain.status=4;s_nativeSaveScreens.clear();assert(before_save_pointer(nullptr,args,&result,nullptr)==HOOK_CONTINUE);
 std::cout<<"Post-run save: stable Yes/No pad selection, pointer synchronization, touch/mouse takeover, reused windows and scope passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp, exe = Path(temp) / 'test.cpp', Path(temp) / 'test'
    cpp.write_text(fixture + (root / 'src/survivors_save_pointer.inc').read_text() + main)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

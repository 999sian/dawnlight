"""Exercise the production native-save lifetime and gameplay input gate."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <vector>
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
struct dMenu_save_c {int status=0;int getSaveStatus(){return status;}};
namespace mods {template<class T>T arg(void* p,int){return static_cast<T>(p);}}
constexpr int kNone=-1;
int s_savePrompt=kNone;
bool arena=true,s_progressSaving=false;
bool in_arena(){return arena;}
// PRODUCTION
int main(){
 dMenu_save_c first,second;
 auto active=[] {return before_save_gameplay_menu(nullptr,nullptr,nullptr,nullptr)==HOOK_SKIP_ORIGINAL;};
 assert(!active());
 assert(before_save_screen_open(nullptr,&first,nullptr,nullptr)==HOOK_CONTINUE);
 before_save_screen_open(nullptr,&first,nullptr,nullptr);
 assert(s_nativeSaveScreens.size()==1&&!active());
 // Loading/open/error/closing states all own the controls, independently of
 // native pause flags. The callbacks never consume the save dialog's input.
 for(int state:{1,2,3,4}){first.status=state;assert(active());}
 first.status=0;assert(!active());
 before_save_screen_open(nullptr,&second,nullptr,nullptr);
 second.status=2;assert(active());
 before_save_screen_delete(nullptr,&first,nullptr,nullptr);assert(active());
 arena=false;assert(!active());
 before_save_screen_delete(nullptr,&second,nullptr,nullptr);
 arena=true;assert(!active()&&s_nativeSaveScreens.empty());
 // Cover the frame before the Game Over save object has opened, and hub writes.
 s_savePrompt=42;assert(active());s_savePrompt=kNone;assert(!active());
 s_progressSaving=true;assert(active());s_progressSaving=false;assert(!active());
 before_save_screen_open(nullptr,nullptr,nullptr,nullptr);assert(s_nativeSaveScreens.empty());
 before_save_screen_delete(nullptr,&first,nullptr,nullptr);assert(!active());
}
'''.replace('// PRODUCTION', (root/'src/survivors_save_modal.inc').read_text())
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Native save modal: loading, open, error, close, deletion, nested windows and Survivors scope passed')

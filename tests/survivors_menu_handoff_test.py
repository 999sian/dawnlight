"""Exercise the menu handoff against native command mapping and stack semantics."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
def extract(text,name):
 m=re.search(r'^(?:void|bool|NavCommand) '+re.escape(name)+r'\([^;{}]*\)\s*(?:noexcept\s*)?\{',text,re.M)
 assert m,name
 start=text.index('{',m.start());depth=0
 for i in range(start,len(text)):
  depth+=(text[i]=='{')-(text[i]=='}')
  if not depth:return text[m.start():i+1]
fixture=r'''
#include <any>
#include <cassert>
#include <memory>
#include <string>
#include <vector>
#include "survivors_rules.hpp"
#include "mods/svc/ui.h"
#include "dusk/ui/nav_types.hpp"
#define svc_ui test_ui
#define svc_hook test_hook
#define svc_log test_log
using namespace dawnlight::survivors;
using dusk::ui::NavCommand;
ModContext* mod_ctx=nullptr;
enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{
 template<class T>T arg(void* p,int i){return std::any_cast<T>(static_cast<std::any*>(p)[i]);}
}
namespace Rml {
 namespace Input{enum KeyIdentifier{KI_UNKNOWN,KI_UP,KI_DOWN,KI_LEFT,KI_RIGHT,KI_ESCAPE,KI_RETURN,KI_NUMPADENTER,KI_F1,KI_NEXT,KI_PRIOR};}
 struct Event{Input::KeyIdentifier key=Input::KI_F1;int shift=0;
  template<class T>T GetParameter(const char* n,T)const{return T(std::string(n)=="shift_key"?shift:int(key));}};
}
struct Document{
 bool shown=false,pending=false,wasShown=false;UiWindowHandle handle=0;UiWindowClosedFn closed=nullptr;
 bool visible(){return shown;}void toggle(){shown=!shown;}
 bool handle_nav_command(Rml::Event&,NavCommand);
};
bool game_obscured_below(Document&){return false;}
constexpr int kSoundMenuClose=1,kSoundMenuOpen=2;void mDoAud_seStartMenu(int){}
'''
native_map=extract((root/'dusklight/src/dusk/ui/ui.cpp').read_text(),'map_nav_event')
native_command=extract((root/'dusklight/src/dusk/ui/document.cpp').read_text(),'Document::handle_nav_command')
boundaries=r'''
std::vector<std::unique_ptr<Document>> documents;
Document* top(){for(auto i=documents.rbegin();i!=documents.rend();++i)if(!(*i)->pending)return i->get();return nullptr;}
void* host_top(){return top();}
NavCommand host_command(const void* e){return map_nav_event(*static_cast<const Rml::Event*>(e));}
unsigned navigations=0;
bool host_navigate(void* d,void* e,NavCommand c){++navigations;return static_cast<Document*>(d)->handle_nav_command(*static_cast<Rml::Event*>(e),c);}
bool ui_visible(){for(auto& d:documents)if(d->shown&&!d->pending)return true;return false;}
unsigned nextHandle=1;
Document* push(){if(auto* old=top()){old->wasShown=old->shown;old->shown=false;}
 documents.push_back(std::make_unique<Document>());auto* d=documents.back().get();d->shown=true;d->handle=nextHandle++;return d;}
void pop(unsigned handle){for(auto& d:documents)if(d->handle==handle){d->shown=false;d->pending=true;break;}
 if(auto* d=top())d->shown=d->wasShown;}
struct UI{
 ModResult window_push(ModContext*,const UiWindowDesc* desc,UiWindowHandle* out){auto* d=push();d->closed=desc->on_closed;*out=d->handle;return MOD_OK;}
 ModResult window_close(ModContext*,UiWindowHandle h){pop(h);return MOD_OK;}
 ModResult dialog_close(ModContext*,UiDialogHandle h){pop(h);return MOD_OK;}
}ui;auto* test_ui=&ui;
struct Hook{ModResult resolve(ModContext*,const char* name,void** out,void*){
 std::string n=name;*out=n=="dusk::ui::top_document"?reinterpret_cast<void*>(&host_top):n=="dusk::ui::map_nav_event"?reinterpret_cast<void*>(&host_command):reinterpret_cast<void*>(&host_navigate);return MOD_OK;}}hook;
auto* test_hook=&hook;
struct Log{void warn(ModContext*,const char*){assert(false);}}logService;auto* test_log=&logService;
Run s_run;RunOptions s_options;unsigned s_map=3;Records s_records;
bool arena=true,transition=false;bool in_arena(){return arena;}bool transitioning(){return transition;}
void* s_menuDocument=nullptr;void* s_talentDocument=nullptr;
UiWindowHandle s_window=0,s_refreshChoices=0;UiDialogHandle s_talentDialog=0;
ChestReward s_chosenTreasure=ChestReward::Count;
void dismiss_treasure_choice(){assert(false);}
ModResult build_window(ModContext*,UiWindowHandle,UiElementHandle,UiElementHandle,void*,ModError*){return MOD_OK;}
auto build_difficulty=build_window,build_talents=build_window,build_run_settings=build_window;
ModResult update_talent_tab(ModContext*,void*,ModError*){return MOD_OK;}
constexpr char kTreasureStyle[]="",kArenaStyle[]="";
'''
main=r'''
bool request(Document* d,Rml::Event& event){bool handled=false;std::any args[]={static_cast<void*>(d),static_cast<void*>(&event)};
 const auto action=before_survivors_menu_event(nullptr,args,&handled,nullptr);return action==HOOK_SKIP_ORIGINAL&&handled;}
int main(){
 assert(initialize_menu_handoff(nullptr)==MOD_OK);
 auto* host=push();host->shown=false;
 s_run.phase=Phase::Ready;s_options.enemy_hp=7;s_options.camera_distance=350;s_records.talents.ranks[0]=3;
 Rml::Event event;open_window();assert(s_window&&s_menuDocument==top());
 // Menu is the native F1 command: controller bindings and three-finger taps
 // synthesize the same key. Shift-F1 and ordinary navigation remain untouched.
 auto* main=top();event.shift=1;assert(!request(main,event)&&main->shown);event.shift=0;
 event.key=Rml::Input::KI_DOWN;assert(!request(main,event));event.key=Rml::Input::KI_F1;
 for(auto phase:{Phase::Running,Phase::Upgrade,Phase::TreasureChoice,Phase::Victory}){
  s_run.phase=phase;assert(!request(main,event)&&s_window);
 }s_run.phase=Phase::Ready;
 arena=false;assert(!request(main,event));arena=true;transition=true;assert(!request(main,event));transition=false;
 auto* foreign=push();assert(!request(foreign,event)&&s_window);assert(!request(main,event));pop(foreign->handle);
 const auto oldHandle=s_window;auto oldClosed=main->closed;
 assert(request(main,event)&&!s_window&&host->shown&&navigations==1&&main->pending);
 open_window();assert(!s_window); // Host menu has priority, not just one update.
 auto* settings=push();open_window();assert(!s_window);
 pop(settings->handle);open_window();assert(!s_window&&host->shown);
 host->toggle();open_window();assert(s_window&&s_window!=oldHandle);
 oldClosed(nullptr,oldHandle,nullptr);assert(s_window); // Delayed fade/destruction cannot clear the new window.
 assert(s_map==3&&s_options.enemy_hp==7&&s_options.camera_distance==350&&s_records.talents.ranks[0]==3);
 // A description popup must yield in the same press, closing both our windows.
 auto* dialog=push();s_talentDialog=dialog->handle;remember_menu_document(true);
 assert(request(dialog,event)&&!s_talentDialog&&!s_window&&host->shown);
 assert(!s_menuDocument&&!s_talentDocument);
 host->toggle();open_window();assert(s_window);
 // If pop already uncovered a visible host, never toggle it shut again.
 host->wasShown=true;const auto calls=navigations;assert(request(top(),event));
 assert(host->shown&&navigations==calls&&!s_window);
 for(unsigned i=0;i<20;++i){host->toggle();open_window();assert(s_window);assert(request(top(),event)&&host->shown&&!s_window);}
 // Mode exit does not reopen anything; ordinary host navigation is untouched.
 arena=false;assert(!request(host,event));close_window();assert(!s_menuDocument&&!s_talentDocument);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+native_map+native_command+boundaries+extract(core,'close_window')+(root/'src/survivors_menu_handoff.inc').read_text()+extract(core,'open_window')+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-Wno-attributes','-Wno-unused-parameter','-I',str(root/'src'),'-I',str(root/'dusklight/sdk/include'),'-I',str(root/'dusklight/src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors menu: native Menu/Shift-F1 mapping, host subwindows, talent popup, restoration, state and delayed callbacks passed')

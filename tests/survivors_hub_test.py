"""Native door ordering, hub save/close and scene routing without starting combat early."""
from pathlib import Path
import re,subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
hub=(root/'src/survivors_hub.inc').read_text()
def extract(source,name):
 m=re.search(r'^(?:void|bool|HookAction) '+name+r'\([^;{}]*\)\s*\{',source,re.M);assert m,name
 start=source.index('{',m.start());depth=0
 for i in range(start,len(source)):
  depth+=(source[i]=='{')-(source[i]=='}')
  if not depth:return source[m.start():i+1]
fixture=r'''
#include <any>
#include <cassert>
#include <string>
#include "survivors_rules.hpp"
#include "survivors_maps.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using u16=unsigned short;using s16=short;using s8=signed char;using s32=int;
constexpr unsigned kNone=~0u;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* a,int i){return std::any_cast<T>(static_cast<std::any*>(a)[i]);}
 template<class T>T& arg_ref(void* a,int i){return *std::any_cast<T>(&static_cast<std::any*>(a)[i]);}}
struct fopAc_ac_c{unsigned id;};struct daAlink_c:fopAc_ac_c{};
daAlink_c link{{1}};fopAc_ac_c door{2};
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
bool doorAlive=true;fopAc_ac_c* actor_by_id(unsigned id){return doorAlive&&id==2?&door:nullptr;}
Records s_records;Run s_run;RunOptions s_options;
bool s_active=true,s_gameStarted=true,s_saveReady=true,s_restart=false,s_progressSaving=false,s_arenaMeshReady=true;
bool s_hub=true,s_hubDeparture=false,s_hubDoorStarted=false,s_arrivingRun=false,s_allowHubDoor=false,s_hubSaveRequested=false;
enum class HubMenu{None,Talents,Run};HubMenu s_hubMenu=HubMenu::None;
unsigned s_hubClosingWindow=0,s_hubDoor=kNone,s_hubDoorTicks=0,s_player=1,s_map=0,s_requestedMap=0;
u16 s_hubDoorPriority=0,s_hubDoorFlags=0;
bool visible=false,event=false,arenaPresent=true,transition=false,resetting=false;
bool in_arena(){return s_active&&s_gameStarted&&arenaPresent;}
bool ui_visible(){return visible;}bool transitioning(){return transition;}
bool dComIfGp_event_runCheck(){return event;}bool dComIfGp_isPauseFlag(){return false;}
bool midna_dialog_busy(){return false;}
bool playable(){return in_arena()&&!visible&&!event&&!transition;}
namespace mDoRst{bool isReset(){return resetting;}bool isReturnToMenu(){return false;}}
void dComIfGs_setRestartRoomParam(int){}
const Arena& arena(){return arenas[s_map];}
unsigned cleared=0,opened=0,orders=0,pedestalRemoves=0;
void clear_arena_geometry(){++cleared;}
void update_survivors_pedestal(bool enabled){if(!enabled)++pedestalRemoves;}
void open_window(){visible=true;++opened;}
void hub_menu_closed();void close_window(){visible=false;hub_menu_closed();}
struct Log{void warn(ModContext*,const char*){}}logger;auto* svc_log=&logger;ModContext* mod_ctx=nullptr;
HookAction before_hub_door(ModContext*,void*,void*,void*);
bool acceptOrder=true;
int fopAcM_orderDoorEvent(fopAc_ac_c* a,fopAc_ac_c* b,u16 priority,u16 flags){
 std::any args[]={a,b,priority,flags};int result=0;
 if(before_hub_door(nullptr,args,&result,nullptr)==HOOK_SKIP_ORIGINAL)return result;
 assert(s_allowHubDoor&&!visible&&!s_progressSaving&&!s_hubSaveRequested);
 ++orders;return acceptOrder;
}
'''
main=r'''
int main(){
 s_run.phase=Phase::Ready;
 // Loading the save never opens a window and Ready leaves Link executable.
 assert(!visible&&s_hubMenu==HubMenu::None&&!pause_run_actor(false));
 assert(pause_run_actor(true));visible=true;assert(pause_run_actor(false));visible=false;
 std::any request[]={static_cast<fopAc_ac_c*>(&link),&door,u16(7),u16(9)};int result=4;
 assert(before_hub_door(nullptr,request,&result,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(!result&&visible&&opened==1&&s_hubMenu==HubMenu::Run&&s_hubDoor==2&&orders==0);
 // A locked destination cannot bypass the disabled UI by calling Start directly.
 s_requestedMap=1;start_run(nullptr,nullptr);assert(!s_hubDeparture&&visible);
 s_options.debug.unlock_all_maps=true;assert(map_available(1)==debug_enabled);
 s_options.debug.unlock_all_maps=false;s_records.cleared_maps=1;assert(map_available(1));
 s_records.cleared_maps=0;s_requestedMap=0;
 // Back/cancel saves automatically but does not open the door or start a run.
 close_window();assert(s_hubSaveRequested&&s_hubMenu==HubMenu::None);
 update_hub(&link);assert(!orders&&!s_hubDeparture&&s_run.phase==Phase::Ready);
 s_hubSaveRequested=false;
 before_hub_door(nullptr,request,&result,nullptr);
 start_run(nullptr,nullptr);assert(!visible&&s_hubDeparture&&s_hubSaveRequested&&s_run.phase==Phase::Ready);
 update_hub(&link);assert(!orders); // save queued
 s_hubSaveRequested=false;s_progressSaving=true;update_hub(&link);assert(!orders);
 s_progressSaving=false;s_hubClosingWindow=17;update_hub(&link);assert(!orders);s_hubClosingWindow=0;
 assert(pause_run_actor(false)); // Waiting at the door cannot move Link away.
 visible=true;update_hub(&link);assert(!orders);visible=false;
 event=true;update_hub(&link);assert(!orders);event=false;
 update_hub(&link);assert(orders==1&&s_hubDoorStarted&&!s_allowHubDoor&&s_run.phase==Phase::Ready);
 assert(!pause_run_actor(false)); // Native door animation retains player execution.
 update_hub(&link);assert(orders==1); // Never replay an accepted event each tick.
 for(unsigned map=0;map<arenas.size();++map){
  s_hub=s_hubDoorStarted=s_hubDeparture=true;s_map=0;s_requestedMap=map;
  std::any stage[]={static_cast<const char*>("D_MN06"),s16(8),s8(9),s8(2)};
  assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_CONTINUE);
  assert(!s_hub&&s_arrivingRun&&!s_hubDoorStarted&&!s_hubDeparture&&s_map==map);
  assert(std::string(std::any_cast<const char*>(stage[0]))==arenas[map].stage);
  assert(std::any_cast<s16>(stage[1])==arenas[map].point&&std::any_cast<s8>(stage[2])==arenas[map].room);
  // No run snapshot, timer, starting item or mobs until the destination is ready.
  assert(s_run.phase==Phase::Ready&&s_run.ticks==0&&s_run.ranks[0]==0);
  assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 }
 // Unconfirmed hub exits and all combat doors stay blocked.
 s_hub=true;s_hubDoorStarted=s_hubDeparture=false;s_hubMenu=HubMenu::None;visible=false;
 std::any stage[]={static_cast<const char*>("F_SP108"),s16(9),s8(9),s8(9)};
 assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 s_hub=false;s_run.phase=Phase::Running;
 assert(before_hub_door(nullptr,request,&result,nullptr)==HOOK_SKIP_ORIGINAL&&!visible);
 // A failed native order remains retryable without teleporting.
 s_hub=true;s_run.phase=Phase::Ready;s_hubDoor=2;s_hubDeparture=true;acceptOrder=false;s_hubDoorTicks=0;
 for(int i=0;i<61;++i)update_hub(&link);
 assert(!s_hubDeparture&&!s_hubDoorStarted);
 s_hubDeparture=s_hubDoorStarted=true;s_hubDoorTicks=600;event=false;update_hub(&link);
 assert(!s_hubDeparture&&!s_hubDoorStarted);
 // Missing door and settings/talent close cannot accidentally authorize departure.
 doorAlive=false;s_hubDeparture=true;update_hub(&link);assert(!s_hubDeparture);
 s_hubMenu=HubMenu::Talents;hub_menu_closed();assert(s_hubSaveRequested&&s_hubMenu==HubMenu::None);
 // Other saves' stage routing is untouched.
 s_active=false;assert(before_stage(nullptr,stage,nullptr,nullptr)==HOOK_CONTINUE);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+'\n'+'\n'.join(extract(core,n) for n in ['map_available','hub_menu_closed','start_run','before_stage','pause_run_actor'])+'\n'+'\n'.join(extract(hub,n) for n in ['before_hub_door','update_hub'])+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),'-I',str(root/'include'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors hub: free movement, native door gating, save/close, cancellation, routing and recovery passed')

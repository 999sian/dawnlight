"""Results -> deferred native save creation must not pause its own initializer."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
native=(root/'dusklight/src/d/d_gameover.cpp').read_text()
def block(source,marker):
 start=source.index(marker);brace=source.index('{',start);depth=0
 for end in range(brace,len(source)):
  depth+=(source[end]=='{')-(source[end]=='}')
  if not depth:return source[start:end+1]
request=block(core,'    if (s_saveRequested) {')
callback=block(core,'void save_results(')
# Compile the actual native _create gate: a global pause stalls creation before
# the save window can open or reach its close handler.
create_gate=block(native,'    if (dComIfGp_isPauseFlag() ||')
fixture=r'''
#include <array>
#include <cassert>
#include <cstddef>
struct ModContext{};constexpr int MOD_OK=0,kNone=-1;
void* mod_ctx=nullptr;constexpr char kBlob[]="records";
bool s_saveRequested=false,s_storeFailed=false,paused=false,visible=false,event=false,blobFailure=false,createFailure=false;
int s_savePrompt=kNone,gameOverType=0,heapLock=0,messageStatus=0,creates=0,writes=0,closes=0;
bool ui_visible(){return visible;}bool dComIfGp_event_runCheck(){return event;}
int dMeter2Info_getGameOverType(){return gameOverType;}
bool dComIfGp_isPauseFlag(){return paused;}void dComIfGp_onPauseFlag(){paused=true;}
int dComIfGp_isHeapLockFlag(){return heapLock;}int dComIfGp_getMesgStatus(){return messageStatus;}
void close_window(){++closes;}
struct Records{std::array<unsigned,2> encode(){return {10,20};}}s_records;
struct Service{int set_blob(void*,const char*,const void*,std::size_t){++writes;return blobFailure?1:MOD_OK;}}service;
auto* svc_save=&service;
struct Save{int placements=0;Save& getPlayer(){return *this;}Save& getPlayerReturnPlace(){return *this;}
 void set(const char*,int,int){++placements;}}save;
Save* dComIfGs_getSaveData(){return &save;}
struct Arena{const char* stage="arena";int room=51,point=0;}map;
Arena& arena(){return map;}
int d_GameOver_Create(int type){++creates;gameOverType=type;return createFailure?kNone:42;}
'''
main=r'''
int main(){
 // Both victory and defeat share this exact production request branch.
 for(int ending=0;ending<2;++ending){
  s_savePrompt=kNone;gameOverType=0;creates=writes=closes=0;paused=false;
  save_results(nullptr,nullptr);assert(s_saveRequested&&closes==1);
  visible=true;tick_save();assert(!creates&&s_saveRequested);visible=false;
  event=true;tick_save();assert(!creates&&s_saveRequested);event=false;
  gameOverType=1;tick_save();assert(!creates&&s_saveRequested);gameOverType=0;
  tick_save();assert(s_savePrompt==42&&!s_saveRequested&&creates==1&&writes==1);
  assert(native_create()==4); // Original regression: global pause made this return 0 forever.
  tick_save();assert(creates==1&&writes==1);
  paused=true;assert(native_create()==0);paused=false; // Validate the real host gate.
  heapLock=1;assert(native_create()==0);heapLock=6;assert(native_create()==4);heapLock=0;
  messageStatus=1;assert(native_create()==0);messageStatus=0;
 }
 gameOverType=0;s_savePrompt=kNone;blobFailure=true;save_results(nullptr,nullptr);
 const auto before=creates;tick_save();assert(s_storeFailed&&!s_saveRequested&&creates==before&&!paused);
 blobFailure=false;createFailure=true;save_results(nullptr,nullptr);tick_save();
 assert(s_saveRequested&&s_savePrompt==kNone&&!paused);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+callback+'\nvoid tick_save(){\n'+request+'\n}\nint native_create(){\n'+create_gate+'\nreturn 4;\n}\n'+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Post-run save: deferred native creation, pause gate, UI/event handoff, one request and failure paths passed')

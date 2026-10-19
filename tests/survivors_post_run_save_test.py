"""Exercise the real post-run/native-save handoff, including failed creation/retry."""
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
callback=block(core,'void save_results(')
update=block(core,'void update_survivors()')
assert update.index('update_post_run_save()')<update.index('auto* link =')
# Match the actual native gate, including its deferred-pause restriction.
create_gate=block(native,'    if (dComIfGp_isPauseFlag() ||')
fixture=r'''
#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>
struct ModContext{};constexpr int MOD_OK=0,kNone=-1;
void* mod_ctx=nullptr;constexpr char kBlob[]="records";
bool s_saveRequested=false,s_storeFailed=false,paused=false,visible=false,event=false,blobFailure=false,createFailure=false;
bool creating=false,present=false,finished=false;
unsigned s_saveStartTicks=0;
int s_savePrompt=kNone,gameOverType=0,gameOverStatus=0,heapLock=0,messageStatus=0,creates=0,writes=0,closes=0,opens=0,deletes=0;
bool ui_visible(){return visible;}bool dComIfGp_event_runCheck(){return event;}
int dMeter2Info_getGameOverType(){return gameOverType;}
void dMeter2Info_setGameOverType(int type){gameOverType=type;}
bool dComIfGp_isPauseFlag(){return paused;}
int dComIfGp_isHeapLockFlag(){return heapLock;}int dComIfGp_getMesgStatus(){return messageStatus;}
void close_window(){++closes;}void open_window(){++opens;}
bool fpcM_IsCreating(int){return creating;}
void* fopMsgM_SearchByID(int){return present?&present:nullptr;}
bool d_GameOver_CheckDelete(int){return finished;}
int dComIfGp_getGameoverStatus(){return gameOverStatus;}
void dComIfGp_setGameoverStatus(int status){gameOverStatus=status;}
void d_GameOver_Delete(int& id){++deletes;id=kNone;gameOverType=0;present=false;}
struct Log{
 std::vector<std::string> warnings;
 void info(void*,const char*){}void warn(void*,const char* message){warnings.emplace_back(message);}
}logService;auto* svc_log=&logService;
struct Records{std::array<unsigned,2> encode(){return {10,20};}}s_records;
struct Service{int set_blob(void*,const char*,const void*,std::size_t){++writes;return blobFailure?1:MOD_OK;}}service;
auto* svc_save=&service;
struct Save{int placements=0;Save& getPlayer(){return *this;}Save& getPlayerReturnPlace(){return *this;}
 void set(const char*,int,int){++placements;}}save;
Save* dComIfGs_getSaveData(){return &save;}
struct Arena{const char* stage="arena";int room=51,point=0;}map;
Arena& arena(){return map;}
int d_GameOver_Create(int type){++creates;gameOverType=type;present=!createFailure;return createFailure?kNone:42;}
'''
main=r'''
void reset(){
 s_saveRequested=s_storeFailed=paused=visible=event=blobFailure=createFailure=creating=present=finished=false;
 s_saveStartTicks=0;s_savePrompt=kNone;gameOverType=gameOverStatus=heapLock=messageStatus=creates=writes=closes=opens=deletes=0;
 logService.warnings.clear();
}
int main(){
 // Both endings share this production handoff. A lingering victory event must
 // not block creation: it is not part of the native initializer's gate.
 for(int ending=0;ending<2;++ending){
  reset();event=ending==0;
  save_results(nullptr,nullptr);assert(s_saveRequested&&closes==1);
  save_results(nullptr,nullptr);assert(closes==1); // Repeated click cannot duplicate writes.
  visible=true;assert(update_post_run_save());assert(!creates&&s_saveRequested);visible=false;
  assert(update_post_run_save());assert(s_savePrompt==42&&!s_saveRequested&&creates==1&&writes==1);
  assert(native_create()==4&&!paused);
  save_results(nullptr,nullptr);assert(closes==1);
  creating=true;for(int i=0;i<300;++i)assert(update_post_run_save());
  assert(!deletes&&!opens&&!s_storeFailed&&logService.warnings.size()==1);
  creating=false;assert(update_post_run_save());assert(!deletes&&!opens);
  finished=true;gameOverStatus=1;assert(update_post_run_save());
  assert(deletes==1&&s_savePrompt==kNone&&gameOverType==0&&gameOverStatus==0);
  assert(!update_post_run_save()&&creates==1&&writes==1);
 }
 // Every native startup gate gets a bounded wait, a visible error and a retry.
 for(int gate=0;gate<5;++gate){
  reset();save_results(nullptr,nullptr);
  switch(gate){case 0:visible=true;break;case 1:paused=true;break;case 2:heapLock=1;break;
   case 3:messageStatus=1;break;case 4:gameOverType=2;break;}
  if(gate>=1&&gate<=3)assert(native_create()==0);
  for(int i=0;i<299;++i)assert(update_post_run_save());
  assert(s_saveRequested&&!creates&&!opens);
  assert(update_post_run_save());assert(!s_saveRequested&&s_storeFailed&&opens==1&&!creates&&!writes);
  assert(!update_post_run_save());
  if(gate==4)assert(gameOverType==2); // Do not clear another native dialog's state.
  visible=paused=false;heapLock=messageStatus=gameOverType=0;
  save_results(nullptr,nullptr);assert(!s_storeFailed);assert(update_post_run_save());
  assert(s_savePrompt==42&&creates==1&&writes==1);
 }
 reset();blobFailure=true;save_results(nullptr,nullptr);assert(update_post_run_save());
 assert(s_storeFailed&&!s_saveRequested&&s_savePrompt==kNone&&!creates&&opens==1);
 blobFailure=false;save_results(nullptr,nullptr);assert(update_post_run_save());assert(creates==1);
 // Regression: the native allocator sets type=1 even when returning kNone.
 // Multiple subsequent ticks and a retry must never wait on our orphaned type.
 reset();createFailure=true;save_results(nullptr,nullptr);assert(update_post_run_save());
 assert(s_storeFailed&&!s_saveRequested&&s_savePrompt==kNone&&gameOverType==0&&opens==1);
 for(int i=0;i<600;++i)assert(!update_post_run_save());
 createFailure=false;save_results(nullptr,nullptr);assert(update_post_run_save());
 assert(!s_storeFailed&&s_savePrompt==42&&creates==2&&writes==2);
 // Deferred creation can also fail after allocation. Restore results without
 // attempting _delete on an uninitialized/null native save object.
 present=false;assert(update_post_run_save());
 assert(s_savePrompt==kNone&&!s_saveRequested&&s_storeFailed&&gameOverType==0&&opens==2&&!deletes);
 save_results(nullptr,nullptr);assert(update_post_run_save());assert(s_savePrompt==42&&!s_storeFailed);
 // Native heap lock 6 is allowed; don't stall a valid native save allocation.
 reset();heapLock=6;save_results(nullptr,nullptr);assert(update_post_run_save());assert(creates==1);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp,exe=Path(tmp)/'test.cpp',Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_post_run_save.inc').read_text()+callback+
                '\nint native_create(){\n'+create_gate+'\nreturn 4;\n}\n'+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Post-run save: victory event, native gates/timeouts, creation failures, repeated ticks/retry and normal close passed')

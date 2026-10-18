// Use the host's native background save state machine: it runs before ModLoader
// ticks even while a mod window is open, and flushes per-slot blobs on completion.
// Resolve lazily; a missing host symbol reports failure without closing the menu.
void (*s_enterProgressSave)()=nullptr;
u8* s_hostSavePhase=nullptr;
int* s_hostSaveResult=nullptr;
bool resolve_progress_save() {
    if(s_enterProgressSave && s_hostSavePhase && s_hostSaveResult)return true;
    void *enter=nullptr,*phase=nullptr,*result=nullptr;
    if(svc_hook->resolve(mod_ctx,"enterAutoSave",&enter,nullptr)!=MOD_OK || !enter ||
       svc_hook->resolve(mod_ctx,"mAutoSaveProc",&phase,nullptr)!=MOD_OK || !phase ||
       svc_hook->resolve(mod_ctx,"autoSaveWriteState",&result,nullptr)!=MOD_OK || !result)return false;
    s_enterProgressSave=reinterpret_cast<void(*)()>(enter);
    s_hostSavePhase=static_cast<u8*>(phase);s_hostSaveResult=static_cast<int*>(result);
    return true;
}
void save_progress(ModContext*,void*) {
    if(s_run.phase!=Phase::Ready || s_restart || s_progressSaving)return;
    s_progressSaved=false;
    if(!resolve_progress_save() || *s_hostSavePhase!=0 || mDoMemCd_getStatus(0)!=2) {s_storeFailed=true;return;}
    const auto bytes=s_records.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(s_storeFailed)return;
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(arena().stage,arena().room,arena().point);
    *s_hostSaveResult=0;
    s_enterProgressSave();
    s_progressSaving=*s_hostSavePhase!=0;
    s_storeFailed=!s_progressSaving;
}
void update_progress_save() {
    if(!s_progressSaving || *s_hostSavePhase!=0)return;
    s_progressSaving=false;
    s_storeFailed=*s_hostSaveResult!=1;
    s_progressSaved=!s_storeFailed;
}

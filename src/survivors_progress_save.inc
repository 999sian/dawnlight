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
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(arenas[0].stage,arenas[0].room,arenas[0].point);
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

// Closing a hub menu queues exactly one background save. Let an existing host
// save finish first, and keep failed writes in the hub where they can be retried.
bool update_hub_save() {
    if(s_progressSaving) {
        update_progress_save();
        if(!s_progressSaving && s_storeFailed)s_hubDeparture=false;
        return true;
    }
    if(!s_hubSaveRequested || !in_arena() || transitioning() || dComIfGp_event_runCheck())return false;
    if(resolve_progress_save() && *s_hostSavePhase!=0)return true;
    s_hubSaveRequested=false;save_progress(nullptr,nullptr);
    if(s_storeFailed)s_hubDeparture=false;
    return s_progressSaving;
}

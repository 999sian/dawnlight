// Pump before Link/arena readiness checks: a completed run can leave an event
// running while its actors are frozen. That event is not a native save gate.
void post_run_save_failed(const char* reason) {
    s_saveRequested=false; s_saveStartTicks=0; s_storeFailed=true;
    const auto detail=std::string("Survivors post-run save: ")+reason+
        " (pause="+std::to_string(dComIfGp_isPauseFlag())+
        ", heap="+std::to_string(dComIfGp_isHeapLockFlag())+
        ", message="+std::to_string(dComIfGp_getMesgStatus())+
        ", gameover="+std::to_string(dMeter2Info_getGameOverType())+
        ", event="+std::to_string(dComIfGp_event_runCheck())+")";
    svc_log->warn(mod_ctx,detail.c_str());
    open_window();
}
bool update_post_run_save() {
    if(s_savePrompt!=kNone) {
        if(fpcM_IsCreating(s_savePrompt)) {
            if(++s_saveStartTicks==300)
                svc_log->warn(mod_ctx,"Survivors post-run save: native process is still loading");
        } else if(!fopMsgM_SearchByID(s_savePrompt)) {
            // A deferred request can fail after returning a valid process ID.
            // There is no native object to delete in this case.
            s_savePrompt=kNone;
            if(dMeter2Info_getGameOverType()==1)dMeter2Info_setGameOverType(0);
            post_run_save_failed("native process creation failed");
        } else if(d_GameOver_CheckDelete(s_savePrompt) && dComIfGp_getGameoverStatus()==1) {
            d_GameOver_Delete(s_savePrompt); dComIfGp_setGameoverStatus(0);
            s_saveStartTicks=0;
        }
        return true;
    }
    if(!s_saveRequested)return false;
    // Match dGameover_c::_create's actual gates. Do not enqueue an initializer
    // which cannot run, and return control if another UI keeps the heap busy.
    const auto heap=dComIfGp_isHeapLockFlag();
    if(ui_visible() || dComIfGp_isPauseFlag() || (heap!=0 && heap!=6) ||
       dComIfGp_getMesgStatus()!=0 || dMeter2Info_getGameOverType()!=0) {
        if(++s_saveStartTicks>=300)post_run_save_failed("timed out waiting for native save readiness");
        return true;
    }
    const auto bytes=s_records.encode();
    if(svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK) {
        post_run_save_failed("could not stage the records blob");
        return true;
    }
    dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(arena().stage,arena().room,arena().point);
    s_savePrompt=d_GameOver_Create(1);
    s_saveRequested=false; s_saveStartTicks=0;
    if(s_savePrompt==kNone) {
        // d_GameOver_Create sets the type BEFORE allocating the request. Undo
        // our type on failure or the next attempt will wait on it forever.
        dMeter2Info_setGameOverType(0);
        post_run_save_failed("could not allocate the native save process");
    } else {
        svc_log->info(mod_ctx,"Survivors post-run save: native process requested");
    }
    // Never set the global pause flag here: native deferred creation waits on it.
    return true;
}

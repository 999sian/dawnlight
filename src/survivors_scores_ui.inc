// Score browsing is a separate window: the hub's live Run never changes.
UiWindowHandle s_scoreWindow=0;
UiDialogHandle s_scoreResetDialog=0;
UiDialogHandle s_scoreDeleteDialog=0;
HighScoreRun s_scoreView;
unsigned s_scoreMap=0;
bool s_scoreAssisted=false,s_scoreEndless=false;
struct ScoreCategory {bool assisted,endless;};
std::array<ScoreCategory,4> s_scoreKinds{{{false,false},{true,false},{false,true},{true,true}}};
std::array<UiElementHandle,4> s_scoreButtons{};
std::array<std::string,4> s_scoreLabels{};

bool scores_editable() {
    return s_active && s_gameStarted && s_saveReady && s_loaded && s_hub &&
        s_run.phase==Phase::Ready && settings_editable();
}
void close_score_delete_dialog() {
    const auto dialog=s_scoreDeleteDialog;s_scoreDeleteDialog=0;
    if(dialog)svc_ui->dialog_close(mod_ctx,dialog);
}
void close_high_score(ModContext*,void*) {
    close_score_delete_dialog();
    const auto handle=s_scoreWindow;s_scoreWindow=0;
    if(handle)svc_ui->window_close(mod_ctx,handle);
}
void close_score_windows() {
    const auto dialog=s_scoreResetDialog;s_scoreResetDialog=0;
    if(dialog)svc_ui->dialog_close(mod_ctx,dialog);
    close_high_score(nullptr,nullptr);
    s_scoreButtons={};s_scoreLabels={};
}
void open_high_score(ModContext*,void* data) {
    if(!scores_editable() || s_scoreWindow || s_requestedMap>=arenas.size())return;
    const auto kind=*static_cast<ScoreCategory*>(data);
    const auto& best=s_records.high_score(s_requestedMap,kind.assisted,kind.endless);
    if(!best.present())return;
    s_scoreView=best;s_scoreMap=s_requestedMap;s_scoreAssisted=kind.assisted;s_scoreEndless=kind.endless;
    UiTabDesc tab=UI_TAB_DESC_INIT;tab.title=kind.endless?(kind.assisted?"Cheated Endless Score":"Ranked Endless Score"):best.cheated()?"Cheated high score":"Ranked high score";
    tab.build=[](ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
        return build_results(left,right,&s_scoreView,s_scoreMap);
    };
    UiWindowDesc desc=UI_WINDOW_DESC_INIT;desc.tabs=&tab;desc.tab_count=1;desc.rcss=kResultsStyle;
    desc.on_closed=[](ModContext*,UiWindowHandle handle,void*){
        if(s_scoreWindow==handle){s_scoreWindow=0;close_score_delete_dialog();}
    };
    if(svc_ui->window_push(mod_ctx,&desc,&s_scoreWindow)!=MOD_OK)s_scoreWindow=0;
}
std::string score_label(unsigned category) {
    const auto& best=s_records.high_score(s_requestedMap,s_scoreKinds[category].assisted,s_scoreKinds[category].endless);
    return std::string(category>=2?"Endless Score: ":category?"Cheated: ":"Ranked: ")+(best.present()?result_number(best.score):"—");
}
ModResult update_score_controls() {
    for(unsigned i=0;i<s_scoreButtons.size();++i)if(s_scoreButtons[i]) {
        const auto label=score_label(i);
        if(label!=s_scoreLabels[i]) {
            const auto result=svc_ui->control_set_label(mod_ctx,s_scoreButtons[i],label.c_str());
            if(result!=MOD_OK)return result;
            s_scoreLabels[i]=label;
        }
    }
    return MOD_OK;
}
ModResult build_score_controls(UiElementHandle pane) {
    s_scoreButtons={};
    UiRowDesc desc=UI_ROW_DESC_INIT;std::array<UiElementHandle,2> rows{};
    for(auto& row:rows){
        auto result=svc_ui->pane_add_row(mod_ctx,pane,&desc,&row);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,row,"survivors-score-row",true);if(result!=MOD_OK)return result;
    }
    for(unsigned i=0;i<s_scoreButtons.size();++i) {
        s_scoreLabels[i]=score_label(i);
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.label=s_scoreLabels[i].c_str();control.user_data=&s_scoreKinds[i];control.on_pressed=open_high_score;
        control.help_rml=s_scoreKinds[i].assisted?"Best run with Allow Cheats or another active mod. Select to view its damage breakdown.":
            "Best run without Allow Cheats or other mods. Select to view its damage breakdown.";
        control.is_disabled=[](ModContext*,void* data){return !scores_editable() || s_requestedMap>=arenas.size() ||
            !s_records.high_score(s_requestedMap,static_cast<ScoreCategory*>(data)->assisted,static_cast<ScoreCategory*>(data)->endless).present();};
        const auto result=svc_ui->pane_add_control(mod_ctx,rows[i%2],&control,&s_scoreButtons[i]);
        if(result!=MOD_OK)return result;
    }
    return MOD_OK;
}
bool score_deletable() {
    return scores_editable() && s_scoreWindow && s_scoreMap<arenas.size() &&
        s_scoreView.present() && s_records.high_score(s_scoreMap,s_scoreAssisted,s_scoreEndless).encode()==s_scoreView.encode();
}
void confirm_delete_score(ModContext*,UiDialogHandle dialog,void*) {
    if(dialog!=s_scoreDeleteDialog || !score_deletable())return;
    auto next=s_records;next.delete_high_score(s_scoreMap,s_scoreAssisted,s_scoreEndless);
    const auto bytes=next.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(s_storeFailed) {
        svc_ui->dialog_set_body(mod_ctx,dialog,"<p>The score could not be stored. Nothing was deleted. Choose Yes to retry or No to cancel.</p>");
        return;
    }
    s_records=next;s_progressSaved=false;s_hubSaveRequested=true;
    close_high_score(nullptr,nullptr);
    update_score_controls();
}
void request_delete_score(ModContext*,void*) {
    if(!score_deletable() || s_scoreDeleteDialog || s_scoreResetDialog)return;
    UiDialogAction actions[2]={UI_DIALOG_ACTION_INIT,UI_DIALOG_ACTION_INIT};
    actions[0].label="No";
    actions[0].on_pressed=[](ModContext*,UiDialogHandle handle,void*){if(s_scoreDeleteDialog==handle)s_scoreDeleteDialog=0;};
    actions[1].label="Yes";actions[1].on_pressed=confirm_delete_score;actions[1].keep_open=true;
    actions[1].is_disabled=[](ModContext*,void*){return !score_deletable();};
    const auto body="<p>Permanently delete the "+std::string(s_scoreAssisted?"Cheated":"Ranked")+
        (s_scoreEndless?" Endless Score for ":" high score for ")+treasure_rml_escape(arenas[s_scoreMap].name)+
        " and its run summary?</p><p>Other scores, unlocked maps, talents, XP and lifetime statistics will be kept.</p>";
    UiDialogDesc desc=UI_DIALOG_DESC_INIT;desc.title="Delete Score?";desc.variant=UI_DIALOG_WARNING;
    desc.body_rml=body.c_str();desc.actions=actions;desc.action_count=2;desc.on_dismiss=actions[0].on_pressed;
    if(svc_ui->dialog_push(mod_ctx,&desc,&s_scoreDeleteDialog)!=MOD_OK)s_scoreDeleteDialog=0;
}
void confirm_reset_scores(ModContext*,UiDialogHandle dialog,void*) {
    if(dialog!=s_scoreResetDialog || !scores_editable())return;
    auto next=s_records;next.clear_high_scores();
    const auto bytes=next.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(s_storeFailed) {
        svc_ui->dialog_set_body(mod_ctx,dialog,"<p>High scores could not be stored. Nothing was reset. Choose Yes to retry or No to cancel.</p>");
        return;
    }
    s_records=next;s_progressSaved=false;s_hubSaveRequested=true;
    s_scoreResetDialog=0;svc_ui->dialog_close(mod_ctx,dialog);
}
void request_reset_scores(ModContext*,void*) {
    if(!scores_editable() || s_scoreResetDialog || s_scoreDeleteDialog)return;
    // The host focuses the first action; cancellation and the default are No.
    UiDialogAction actions[2]={UI_DIALOG_ACTION_INIT,UI_DIALOG_ACTION_INIT};
    actions[0].label="No";
    actions[0].on_pressed=[](ModContext*,UiDialogHandle handle,void*){if(s_scoreResetDialog==handle)s_scoreResetDialog=0;};
    actions[1].label="Yes";actions[1].on_pressed=confirm_reset_scores;actions[1].keep_open=true;
    actions[1].is_disabled=[](ModContext*,void*){return !scores_editable();};
    UiDialogDesc desc=UI_DIALOG_DESC_INIT;desc.title="Reset High Scores?";desc.variant=UI_DIALOG_WARNING;
    desc.body_rml="<p>Are you sure you want to permanently reset all normal and Endless Ranked and Cheated high scores and their run summaries?</p><p>Unlocked maps, talents, XP and lifetime statistics will be kept.</p>";
    desc.actions=actions;desc.action_count=2;desc.on_dismiss=actions[0].on_pressed;
    if(svc_ui->dialog_push(mod_ctx,&desc,&s_scoreResetDialog)!=MOD_OK)s_scoreResetDialog=0;
}
ModResult build_reset_scores(UiElementHandle pane) {
    UiControlDesc control=UI_CONTROL_DESC_INIT;control.label="Reset High Scores";control.on_pressed=request_reset_scores;
    control.help_rml="Reset normal and Endless score lists for this Survivors save, with confirmation. Keeps unlocked maps, talents and XP. Load a Survivors save and return to its hub to use this.";
    control.is_disabled=[](ModContext*,void*){return !scores_editable();};
    return svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);
}

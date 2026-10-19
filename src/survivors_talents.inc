// One scrollable list. The running game keeps its own immutable talent snapshot.
UiElementHandle s_talentSummary=0;
std::array<UiElementHandle,unsigned(Talent::Count)> s_talentRows{};
std::array<Talent,unsigned(Talent::Count)> s_talentChoices{
    Talent::MaxHealth,Talent::CritChance,Talent::Sprint,Talent::SprintSpeed,Talent::Magnet,
    Talent::ManualJump,Talent::Glider,Talent::Gale,Talent::GaleCharges,Talent::Rerolls,Talent::Banishes,Talent::Dodge,Talent::StartingItem,Talent::AutoRupees,Talent::BonusDamage,Talent::MeleeHaste,Talent::CritDamage,Talent::LegendaryChance,Talent::LifeRegen};
bool talent_editable() { return s_run.phase==Phase::Ready && !s_restart && !s_progressSaving; }
std::string talent_summary() {
    const char* status=s_progressSaving ? "Saving..." : s_storeFailed ? "Save failed. Please retry Save progress." :
        s_progressSaved ? "Progress saved." : "Each rank costs 1 Unlock Point. Progress and settings save automatically when you close this menu.";
    return "Level "+number(s_records.level())+" | XP "+std::to_string(s_records.level_xp())+" / "+
        number(s_records.next_level_xp())+" | Unlock Points: "+number(s_records.available_points())+"\n"+status;
}
std::string talent_detail(Talent t) {
    return number(s_records.talents.rank(t))+" / "+number(talent_caps[unsigned(t)]);
}
void refresh_talents() {
    svc_ui->elem_set_text(mod_ctx,s_talentSummary,talent_summary().c_str());
    for(unsigned i=0;i<s_talentRows.size();++i)
        svc_ui->elem_set_text(mod_ctx,s_talentRows[i],talent_detail(Talent(i)).c_str());
}
void store_talents(const Records& next) {
    const auto bytes=next.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(!s_storeFailed){s_records=next;s_progressSaved=false;}
    refresh_talents();
}
void buy_talent(ModContext*,void* data) {
    if(!talent_editable())return;
    auto next=s_records;
    if(next.buy(*static_cast<Talent*>(data)))store_talents(next);
}
void refund_talent(ModContext*,void* data) {
    if(!talent_editable())return;
    auto next=s_records;
    if(next.talents.refund(*static_cast<Talent*>(data)))store_talents(next);
}
void reset_talents(ModContext*,void*) {
    if(!talent_editable() || !s_records.talents.spent())return;
    auto next=s_records;next.talents={};store_talents(next);
}
void describe_talent(ModContext*,void* data) {
    if(s_talentDialog)return;
    const auto t=*static_cast<Talent*>(data);
    const std::string body="<p>Rank "+talent_detail(t)+"</p><p>"+talent_help[unsigned(t)]+"</p>"+
        "<p>+ spends one point; - refunds one rank. Removing a prerequisite also refunds its dependent perks.</p>";
    UiDialogAction close=UI_DIALOG_ACTION_INIT;close.label="Close";
    close.on_pressed=[](ModContext*,UiDialogHandle,void*){s_talentDialog=0;s_talentDocument=nullptr;};
    UiDialogDesc dialog=UI_DIALOG_DESC_INIT;
    dialog.title=talent_names[unsigned(t)];dialog.body_rml=body.c_str();dialog.actions=&close;dialog.action_count=1;
    dialog.on_dismiss=close.on_pressed;
    if(svc_ui->dialog_push(mod_ctx,&dialog,&s_talentDialog)!=MOD_OK)s_talentDialog=0;
    else remember_menu_document(true);
}
ModResult update_talent_tab(ModContext*,void*,ModError*) {
    svc_ui->elem_set_text(mod_ctx,s_talentSummary,talent_summary().c_str());return MOD_OK;
}
ModResult build_talents(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    s_talentSummary=0;s_talentRows={};
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-talents",true);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_visible(mod_ctx,right,false);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_text(mod_ctx,left,talent_summary().c_str(),&s_talentSummary);if(result!=MOD_OK)return result;
    for(unsigned i=0;i<s_talentChoices.size();++i){
        UiRowDesc rowDesc=UI_ROW_DESC_INIT;UiElementHandle row=0,handle=0;
        result=svc_ui->pane_add_row(mod_ctx,left,&rowDesc,&row);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,row,"survivors-talent-row",true);if(result!=MOD_OK)return result;
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.label=talent_names[i];control.on_pressed=describe_talent;control.user_data=&s_talentChoices[i];
        result=svc_ui->pane_add_control(mod_ctx,row,&control,&handle);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,handle,"survivors-talent-name",true);if(result!=MOD_OK)return result;
        result=svc_ui->pane_add_text(mod_ctx,row,talent_detail(Talent(i)).c_str(),&s_talentRows[i]);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,s_talentRows[i],"survivors-talent-rank",true);if(result!=MOD_OK)return result;
        control.label="-";control.on_pressed=refund_talent;control.tooltip="Refund a rank (and dependent perks if needed)";
        control.is_disabled=[](ModContext*,void* data){return !talent_editable() || !s_records.talents.rank(*static_cast<Talent*>(data));};
        result=svc_ui->pane_add_control(mod_ctx,row,&control,&handle);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,handle,"survivors-talent-adjust",true);if(result!=MOD_OK)return result;
        control.label="+";control.on_pressed=buy_talent;control.tooltip="Spend one Unlock Point";
        control.is_disabled=[](ModContext*,void* data){return !talent_editable() || !s_records.available_points() || !s_records.talents.can_buy(*static_cast<Talent*>(data));};
        result=svc_ui->pane_add_control(mod_ctx,row,&control,&handle);if(result!=MOD_OK)return result;
        result=svc_ui->elem_set_class(mod_ctx,handle,"survivors-talent-adjust",true);if(result!=MOD_OK)return result;
    }
    UiControlDesc control=UI_CONTROL_DESC_INIT;control.label="Reset all perks";control.on_pressed=reset_talents;
    control.is_disabled=[](ModContext*,void*){return !talent_editable() || !s_records.talents.spent();};
    result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);if(result!=MOD_OK)return result;
    control.label="Save progress";control.on_pressed=save_progress;
    control.is_disabled=[](ModContext*,void*){return !talent_editable();};
    return svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
}

// Stored ranks are changed only in the setup menu; a run uses its own snapshot.
UiElementHandle s_talentSummary=0;
std::array<UiElementHandle,unsigned(Talent::Count)> s_talentRows{};
std::array<Talent,unsigned(Talent::Count)> s_talentChoices{
    Talent::MaxHealth,Talent::CritChance,Talent::Sprint,Talent::SprintSpeed,Talent::Magnet,
    Talent::ManualJump,Talent::Glider,Talent::Gale,Talent::GaleCharges,Talent::Rerolls,Talent::Banishes};
std::string talent_summary() {
    return "Level "+number(s_records.level())+" | XP "+std::to_string(s_records.level_xp())+" / "+
        number(s_records.next_level_xp())+" | Unlock Points: "+number(s_records.available_points())+
        (s_storeFailed ? "\nCould not store progress. Retry Save progress before leaving." : "\nEach rank costs 1 Unlock Point. Save progress before quitting.");
}
std::string talent_detail(Talent t) {
    return "Rank "+number(s_records.talents.rank(t))+" / "+number(talent_caps[unsigned(t)])+" — "+talent_help[unsigned(t)];
}
void buy_talent(ModContext*,void* data) {
    if(s_run.phase!=Phase::Ready || s_restart)return;
    const auto talent=*static_cast<Talent*>(data);
    auto next=s_records;
    if(!next.buy(talent))return;
    const auto bytes=next.encode();
    s_storeFailed=svc_save->set_blob(mod_ctx,kBlob,bytes.data(),bytes.size())!=MOD_OK;
    if(!s_storeFailed)s_records=next; // A failed write never consumes a point.
    svc_ui->elem_set_text(mod_ctx,s_talentSummary,talent_summary().c_str());
    for(unsigned i=0;i<s_talentRows.size();++i)
        svc_ui->elem_set_text(mod_ctx,s_talentRows[i],talent_detail(Talent(i)).c_str());
}
ModResult build_talents(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    s_talentSummary=0;s_talentRows={};
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-settings",true);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-settings",true);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_text(mod_ctx,left,talent_summary().c_str(),&s_talentSummary);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_text(mod_ctx,right,"Sprint > Sprint Speed\nManual Jump > Glider > Revali's Gale > Gale Charges",nullptr);
    if(result!=MOD_OK)return result;
    for(unsigned i=0;i<s_talentChoices.size();++i){
        const auto pane=i<5 ? left : right;
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.label=talent_names[i];control.help_rml=talent_help[i];control.on_pressed=buy_talent;control.user_data=&s_talentChoices[i];
        control.is_disabled=[](ModContext*,void* data) {
            return s_run.phase!=Phase::Ready || s_restart || !s_records.available_points() ||
                !s_records.talents.can_buy(*static_cast<Talent*>(data));
        };
        result=svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);if(result!=MOD_OK)return result;
        result=svc_ui->pane_add_text(mod_ctx,pane,talent_detail(Talent(i)).c_str(),&s_talentRows[i]);if(result!=MOD_OK)return result;
    }
    return button(right,"Save progress",save_results);
}

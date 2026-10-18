// The transparent preview pane shows the native scene itself. No captured game
// textures are bundled, and HUD/texture mods are reflected in the preview.
constexpr const char* kArenaStyle=R"(
window { background-color: transparent; backdrop-filter: none; }
window content pane.survivors-settings { background-color: #171d19f5; padding: 16dp; gap: 12dp; }
window content pane.survivors-talents { flex: 1 1 100%; border-right-width: 0dp; overflow: hidden auto; background-color: #171d19f5; padding: 16dp; gap: 12dp; }
.survivors-talent-row { width: 100%; flex-shrink: 0; align-items: center; gap: 8dp; }
button.survivors-talent-name { flex: 1 1 0; min-width: 0; font-size: 17dp; padding: 10dp; }
button.survivors-talent-adjust { flex: 0 0 44dp; min-width: 44dp; font-size: 22dp; padding: 8dp; }
.survivors-talent-rank { flex: 0 0 64dp; text-align: center; font-size: 16dp; }
window tab-bar { background-color: #171d19ee; }
window content pane.survivors-map-list { flex: 0 0 35%; background-color: #171d19ef; padding: 16dp; gap: 8dp; font-size: 15dp; }
window content pane.survivors-map-preview { flex: 1 1 0; padding: 16dp; justify-content: space-between; }
.survivors-map-preview-copy { background-color: #171d19e8; border-radius: 8dp; padding: 16dp; font-size: 16dp; }
.survivors-map-preview-title { display: block; font-family: var(--font-family-heading); font-size: 24dp; }
button.survivors-map-start { align-self: flex-end; padding: 12dp 28dp; font-size: 18dp; background-color: #171d19ee; }
button.survivors-map-option { padding: 12dp; font-size: 16dp; }
button.survivors-map-selected { background-color: #e5bc3b28; border-color: #ffdc75; }
@media (max-width: 640dp) {
    window content pane.survivors-map-list { flex-basis: 42%; padding: 8dp; font-size: 12dp; }
    window content pane.survivors-map-preview { padding: 8dp; }
    button.survivors-map-option { font-size: 13dp; padding: 9dp; }
    .survivors-map-preview-copy { padding: 10dp; font-size: 13dp; }
    .survivors-map-preview-title { font-size: 18dp; }
}
)";
std::array<unsigned,arenas.size()> s_mapChoices{0,1,2,3,4};
void select_arena(ModContext*, void* data) {
    const unsigned choice=*static_cast<unsigned*>(data);
    if (s_run.phase!=Phase::Ready || s_restart || s_progressSaving || choice>=arenas.size() || choice==s_map) return;
    s_requestedMap=choice;
    close_window(); s_restart=true;
}
ModResult build_arena_choices(UiElementHandle left, UiElementHandle right) {
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-map-list",true);
    if (result!=MOD_OK) return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-map-preview",true);
    if (result!=MOD_OK) return result;
    result=svc_ui->pane_add_text(mod_ctx,left,"Choose your arena",nullptr);
    if (result!=MOD_OK) return result;
    for (unsigned i=0;i<arenas.size();++i) {
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.label=arenas[i].name; control.on_pressed=select_arena; control.user_data=&s_mapChoices[i];
        UiElementHandle option=0;
        result=svc_ui->pane_add_control(mod_ctx,left,&control,&option);
        if (result!=MOD_OK) return result;
        result=svc_ui->elem_set_class(mod_ctx,option,"survivors-map-option",true);
        if (result!=MOD_OK) return result;
        if (i==s_map) {
            result=svc_ui->elem_set_class(mod_ctx,option,"survivors-map-selected",true);
            if (result!=MOD_OK) return result;
        }
    }
    if (!s_arenaMeshReady) result=svc_ui->pane_add_text(mod_ctx,left,
        "Arena collision could not be prepared. Please select another map.",nullptr);
    if (result!=MOD_OK) return result;
    result=svc_ui->pane_add_text(mod_ctx,left,
        "Survive 20 minutes. Elites appear every 5 minutes and drop free big chests. Weapons attack automatically. "
        "Collect XP, hearts and rupees. Chest prices: 20, 25, 32, 41, 52...",nullptr);
    if (result!=MOD_OK) return result;
    char records[160];
    std::snprintf(records,sizeof(records),"Runs %u | Wins %u | Meta level %u",
        s_records.runs,s_records.wins,s_records.level());
    result=svc_ui->pane_add_text(mod_ctx,left,records,nullptr);
    if (result!=MOD_OK) return result;
    const std::string copy="<div class=\"survivors-map-preview-copy\"><div class=\"survivors-map-preview-title\" style=\"color: "+
        std::string(arena().accent)+";\">"+treasure_rml_escape(arena().name)+"</div><div>High score: "+number(s_records.high_scores[s_map])+"</div></div>";
    result=svc_ui->pane_add_rml(mod_ctx,right,copy.c_str(),nullptr);
    if (result!=MOD_OK) return result;
    UiControlDesc start=UI_CONTROL_DESC_INIT;
    start.label="Start run"; start.on_pressed=start_run;
    start.is_disabled=[](ModContext*,void*) { return !s_arenaMeshReady || s_restart || s_progressSaving; };
    UiElementHandle startButton=0;
    result=svc_ui->pane_add_control(mod_ctx,right,&start,&startButton);
    if (result!=MOD_OK) return result;
    return svc_ui->elem_set_class(mod_ctx,startButton,"survivors-map-start",true);
}

struct RunOptionControl { const char* label; unsigned RunOptions::*field; unsigned min,max,step; const char* help; };
std::array<RunOptionControl,5> s_runControls{{
    {"Enemy HP",&RunOptions::enemy_hp,1,10,1,"Multiplies health of all enemies, including elites."},
    {"Enemy Damage",&RunOptions::enemy_damage,1,10,1,"Multiplies enemy damage to Link."},
    {"Enemy Speed",&RunOptions::enemy_speed,1,3,1,"Multiplies normal enemy locomotion only; attack movement and knockback remain native."},
    {"Horde",&RunOptions::horde,1,5,1,"More enemies. At x5, every fifth normal kill rolls loot and drops XP. Elite rewards and pots are unchanged."},
    {"Camera distance",&RunOptions::camera_distance,100,500,25,"100% is the normal distance. Default: 200%. Walls can limit the distance; event cameras remain native."}
}};
ModResult build_run_controls(UiElementHandle left, UiElementHandle right, unsigned first, unsigned last) {
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-settings",true);
    if (result!=MOD_OK) return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-settings",true);
    if (result!=MOD_OK) return result;
    for (unsigned i=first;i<last;++i) {
        auto& setting=s_runControls[i];
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.kind=UI_CONTROL_NUMBER; control.label=setting.label; control.help_rml=setting.help;
        control.user_data=&setting; control.min=setting.min; control.max=setting.max; control.step=setting.step;
        control.prefix=i==4 ? nullptr : "x"; control.suffix=i==4 ? "%" : nullptr;
        control.get=[](ModContext*,void* data,UiControlValue* value) {
            value->int_value=s_options.*static_cast<RunOptionControl*>(data)->field;
        };
        control.set=[](ModContext*,void* data,const UiControlValue* value) {
            if (s_run.phase!=Phase::Ready || s_restart) return;
            const auto& setting=*static_cast<RunOptionControl*>(data);
            s_options.*setting.field=unsigned(std::clamp<int64_t>(value->int_value,setting.min,setting.max));
        };
        result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
        if (result!=MOD_OK) return result;
    }
    return svc_ui->pane_add_text(mod_ctx,right,first==0 ?
        "Score bonus: +10% per Enemy HP step, +15% per Enemy Damage step, +25% per Enemy Speed step and +10% per Horde step. Bonuses add together (x1.00 to x4.15). Normal kill points are divided by Horde. Settings apply to the next run." :
        "Settings apply to the next run. Return to Maps to start.",nullptr);
}
ModResult build_difficulty(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    return build_run_controls(left,right,0,4);
}
ModResult build_run_settings(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    return build_run_controls(left,right,4,5);
}

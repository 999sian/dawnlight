// Development-only, last tab in the Ready menu. No debug options enter saves.
struct DebugControl { const char* label; unsigned DebugOptions::*field; const char* const* choices; unsigned count; };
constexpr const char* kDebugRarities[]{"Common","Uncommon","Rare","Epic","Legendary"};
constexpr const char* kDebugSwitch[]{"Off","On"};
constexpr const char* kDebugTimers[]{"20 min","10 min","5 min"};
std::array<DebugControl,5> s_debugControls{{
    {"Minimum Rarity",&DebugOptions::minimum_rarity,kDebugRarities,5},
    {"Player damage",&DebugOptions::player_damage,nullptr,0},
    {"Infinite rerolls",&DebugOptions::infinite_rerolls,kDebugSwitch,2},
    {"Infinite banishes",&DebugOptions::infinite_banishes,kDebugSwitch,2},
    {"Run timer",&DebugOptions::run_minutes,kDebugTimers,3}
}};
ModResult build_debug(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    s_options.debug.clamp();s_run.options.debug.clamp();
    if (!debug_enabled) return MOD_OK;
    for (auto& setting:s_debugControls) {
        UiControlDesc control=UI_CONTROL_DESC_INIT;
        control.kind=setting.choices ? UI_CONTROL_SELECT : UI_CONTROL_NUMBER;
        control.label=setting.label;control.user_data=&setting;
        control.options=setting.choices;control.option_count=setting.count;
        control.min=1;control.max=10;control.step=1;control.prefix=setting.choices?nullptr:"x";
        control.get=[](ModContext*,void* data,UiControlValue* value) {
            s_options.debug.clamp();
            const auto& setting=*static_cast<DebugControl*>(data);
            const auto current=s_options.debug.*setting.field;
            value->int_value=setting.field==&DebugOptions::run_minutes ? (current==20?0:current==10?1:2) : current;
        };
        control.set=[](ModContext*,void* data,const UiControlValue* value) {
            s_options.debug.clamp();
            if (!debug_enabled || s_run.phase!=Phase::Ready || s_restart) return;
            const auto& setting=*static_cast<DebugControl*>(data);
            if (setting.field==&DebugOptions::run_minutes) {
                constexpr unsigned minutes[]{20,10,5};
                s_options.debug.run_minutes=minutes[std::clamp<int64_t>(value->int_value,0,2)];
            } else s_options.debug.*setting.field=unsigned(std::clamp<int64_t>(value->int_value,
                setting.choices?0:1,setting.choices?setting.count-1:10));
            s_options.debug.clamp();
        };
        control.is_disabled=[](ModContext*,void*){return !debug_enabled || s_run.phase!=Phase::Ready || s_restart;};
        const auto result=svc_ui->pane_add_control(mod_ctx,left,&control,nullptr);
        if (result!=MOD_OK) return result;
    }
    UiControlDesc unlock=UI_CONTROL_DESC_INIT;
    unlock.kind=UI_CONTROL_TOGGLE;unlock.label="Unlock all maps";
    unlock.help_rml="Temporarily bypass map locks. Does not change saved clears. Default: off.";
    unlock.get=[](ModContext*,void*,UiControlValue* value){value->bool_value=debug_enabled && s_options.debug.unlock_all_maps;};
    unlock.set=[](ModContext*,void*,const UiControlValue* value){
        if(debug_enabled && settings_editable())s_options.debug.unlock_all_maps=value->bool_value;
    };
    unlock.is_disabled=[](ModContext*,void*){return !debug_enabled || !settings_editable();};
    auto result=svc_ui->pane_add_control(mod_ctx,left,&unlock,nullptr);
    if(result!=MOD_OK)return result;
    return svc_ui->pane_add_text(mod_ctx,right,
        "Debug options apply to the next run. Minimum Rarity covers level-ups and treasures. Player damage includes all attacks and burns. Infinite rerolls are free; chest prices still apply. Four bosses arrive at 25%, 50%, 75% and 100% of the run timer. Other progression uses real elapsed time.",nullptr);
}

// Presentation preferences are separate from immutable run difficulty/score data.
std::array<std::array<ConfigVarHandle,3>,unsigned(HudPart::Count)> s_hudConfig{};
UiWindowHandle s_hudEditorWindow=0;
unsigned s_hudEditorPart=0;
int64_t hud_value(unsigned part,unsigned axis) {
    auto value=hud_defaults[part][axis];
    if(s_hudConfig[part][axis])svc_config->get_int(mod_ctx,s_hudConfig[part][axis],&value);
    return clamp_hud_value(axis,value);
}
HudPlacement hud_position(HudPart part,float center) {
    const auto i=unsigned(part);
    return hud_placement(center,{hud_value(i,0),hud_value(i,1),hud_value(i,2)});
}
ModResult initialize_hud_config() {
    constexpr std::array axes{"x","y","scale"};
    for(unsigned i=0;i<hud_defaults.size();++i)for(unsigned axis=0;axis<3;++axis) {
        const auto name=std::string("survivors-hud-")+hud_part_keys[i]+"-"+axes[axis];
        ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;
        desc.name=name.c_str();desc.type=CONFIG_VAR_INT;desc.default_int=hud_defaults[i][axis];
        const auto result=register_owned_setting(&desc,&s_hudConfig[i][axis]);
        if(result!=MOD_OK)return result;
    }
    return MOD_OK;
}
void close_hud_editor() {
    const auto handle=s_hudEditorWindow;s_hudEditorWindow=0;
    if(handle)svc_ui->window_close(mod_ctx,handle);
}

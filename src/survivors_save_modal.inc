// Native save windows are not host RmlUi documents and do not consistently set
// the gameplay pause/window flags. Track their own lifetime, including loading,
// yes/no prompts, card errors and close animations, without consuming pad input.
std::vector<dMenu_save_c*> s_nativeSaveScreens;
bool save_modal_active() {
    if(!in_arena())return false;
    if(s_savePrompt!=kNone || s_progressSaving)return true;
    return std::any_of(s_nativeSaveScreens.begin(),s_nativeSaveScreens.end(),
        [](auto* screen){return screen->getSaveStatus()!=0;});
}
HookAction before_save_screen_open(ModContext*,void* args,void*,void*) {
    auto* screen=mods::arg<dMenu_save_c*>(args,0);
    if(screen && std::find(s_nativeSaveScreens.begin(),s_nativeSaveScreens.end(),screen)==s_nativeSaveScreens.end())
        s_nativeSaveScreens.push_back(screen);
    return HOOK_CONTINUE;
}
HookAction before_save_screen_delete(ModContext*,void* args,void*,void*) {
    auto* screen=mods::arg<dMenu_save_c*>(args,0);
    s_nativeSaveScreens.erase(std::remove(s_nativeSaveScreens.begin(),s_nativeSaveScreens.end(),screen),s_nativeSaveScreens.end());
    return HOOK_CONTINUE;
}
HookAction before_save_gameplay_menu(ModContext*,void*,void*,void*) {
    // Only gameplay entry points: the save window retains its complete input
    // for slot selection, confirmation, cancellation and retry.
    return (save_modal_active() || (in_arena() && (s_window || s_saveRequested)))?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;
}

// Explain owns the logical yes/no cursor, while 3Select pointerMove can
// independently overwrite the animated pane. GameCube Explain also omits
// Wii DPD-to-logical synchronization. Scope this repair to our post-run save.
const dusk::menu_pointer::State& (*s_savePointerState)() noexcept=nullptr;
struct SavePointerControl {
    ActorId prompt=kNone;
    dMsgScrn3Select_c* selection=nullptr;
    bool controller=false, sampled=false;
    float x=0,y=0;
} s_savePointerControl;
ModResult initialize_save_pointer(ModError* error) {
    void* address=nullptr;
    const auto result=svc_hook->resolve(mod_ctx,"dusk::menu_pointer::state",&address,nullptr);
    if(result!=MOD_OK||!address)
        return mods::set_error(error,MOD_UNAVAILABLE,"failed to resolve native save dialog pointer state");
    s_savePointerState=reinterpret_cast<decltype(s_savePointerState)>(address);
    return MOD_OK;
}
dMsgScrnExplain_c* post_run_save_explain(dMsgScrn3Select_c* selection) {
    if(!selection||s_savePrompt==kNone||!in_arena())return nullptr;
    for(auto* screen:s_nativeSaveScreens) {
        auto* explain=screen->mpScrnExplain;
        if(screen->getSaveStatus()==2&&explain&&explain->mpSelect_c==selection&&
           explain->getStatus()==dMsgScrnExplain_c::STATUS_MOVE_SELECT_e)return explain;
    }
    return nullptr;
}
HookAction before_save_pointer(ModContext*,void* args,void* result,void*) {
    auto* selection=mods::arg<dMsgScrn3Select_c*>(args,0);
    auto* explain=post_run_save_explain(selection);
    if(!explain||!s_savePointerState)return HOOK_CONTINUE;
    if(s_savePointerControl.prompt!=s_savePrompt||s_savePointerControl.selection!=selection)
        s_savePointerControl={s_savePrompt,selection};
    auto& control=s_savePointerControl;
    const auto& pointer=s_savePointerState();
    const bool moved=pointer.valid&&(!control.sampled||pointer.x!=control.x||pointer.y!=control.y||
        pointer.pressed||pointer.clicked||pointer.released);
    if(pointer.valid){control.x=pointer.x;control.y=pointer.y;control.sampled=true;}
    const bool navigation=explain->mpStick&&
        (explain->mpStick->mDirectionTrig&(STControl::TRIG_UP|STControl::TRIG_DOWN));
    if(navigation)control.controller=true;
    else if(moved)control.controller=false;
    if(!control.controller)return HOOK_CONTINUE;
    // Keep native pad selection/animation after releasing the navigation key.
    // A fresh pointer move or click can take ownership again.
    selection->mDPDPoint=0xff;
    *static_cast<bool*>(result)=false;
    return HOOK_SKIP_ORIGINAL;
}
void after_save_pointer(ModContext*,void* args,void* result,void*) {
    auto* selection=mods::arg<dMsgScrn3Select_c*>(args,0);
    auto* explain=post_run_save_explain(selection);
    if(explain&&*static_cast<bool*>(result)&&selection->mDPDPoint<2)
        explain->mSelCursor=selection->mDPDPoint;
}

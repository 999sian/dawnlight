// All native bindings (including the three-finger gesture) are translated into
// this command before dispatch. Do not intercept individual buttons/SDL events.
using MenuCommand = dusk::ui::NavCommand;
struct MenuHost {
    void* (*top)() = nullptr;
    MenuCommand (*command)(const void*) = nullptr;
    bool (*navigate)(void*, void*, MenuCommand) = nullptr;
} s_menuHost;

ModResult initialize_menu_handoff(ModError* error) {
    const auto resolve = [&](const char* name, auto& function) {
        void* address = nullptr;
        const auto result = svc_hook->resolve(mod_ctx, name, &address, nullptr);
        if (result != MOD_OK || !address)
            return mods::set_error(error, MOD_UNAVAILABLE, name);
        function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(address);
        return MOD_OK;
    };
    auto result = resolve("dusk::ui::top_document", s_menuHost.top);
    if (result == MOD_OK) result = resolve("dusk::ui::map_nav_event", s_menuHost.command);
    if (result == MOD_OK) result = resolve("dusk::ui::Document::handle_nav_command", s_menuHost.navigate);
    return result;
}
void remember_menu_document(bool talent) {
    (talent ? s_talentDocument : s_menuDocument) = s_menuHost.top();
}
HookAction before_survivors_menu_event(ModContext*, void* args, void* result, void*) {
    auto* document = mods::arg<void*>(args, 0);
    auto* event = mods::arg<void*>(args, 1);
    if (!in_arena() || transitioning() || s_run.phase != Phase::Ready || !s_window ||
        !document || (document != s_menuDocument && document != s_talentDocument) ||
        s_menuHost.top() != document || s_menuHost.command(event) != MenuCommand::Menu)
        return HOOK_CONTINUE;

    // pop() marks our document pending-close immediately, so the host's next
    // top document is already valid while the old window finishes its fade.
    // This also closes our talent description, without touching foreign dialogs.
    close_window();
    if (!ui_visible()) {
        if (auto* host = s_menuHost.top()) s_menuHost.navigate(host, event, MenuCommand::Menu);
    }
    // update_survivors/open_window already waits for all host documents to hide.
    // Keeping Phase::Ready preserves setup and reopens it only after that point.
    *static_cast<bool*>(result) = true;
    return HOOK_SKIP_ORIGINAL;
}

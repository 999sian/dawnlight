// All native bindings (including the three-finger gesture) are translated into
// this command before dispatch. Do not intercept individual buttons/SDL events.
using MenuCommand = dusk::ui::NavCommand;
struct MenuHost {
    void* (*top)() = nullptr;
    bool enabled = false;
    bool (*navigate)(void*, void*, MenuCommand) = nullptr;
} s_menuHost;

void remember_menu_document(bool talent) {
    (talent ? s_talentDocument : s_menuDocument) = s_menuHost.enabled ? s_menuHost.top() : nullptr;
}
HookAction before_survivors_menu_command(ModContext*, void* args, void* result, void*) {
    auto* document = mods::arg<void*>(args, 0);
    auto* event = mods::arg<void*>(args, 1);
    if (!s_menuHost.enabled || !in_arena() || transitioning() || s_run.phase != Phase::Ready || !s_window ||
        !document || (document != s_menuDocument && document != s_talentDocument) ||
        s_menuHost.top() != document || mods::arg<MenuCommand>(args, 2) != MenuCommand::Menu)
        return HOOK_CONTINUE;

    // pop() marks our document pending-close immediately, so the host's next
    // top document is already valid while the old window finishes its fade.
    // This also closes our talent description, without touching foreign dialogs.
    const auto hubMenu=s_hubMenu;
    close_window();
    s_hubMenu=hubMenu;
    if (!ui_visible()) {
        if (auto* host = s_menuHost.top()) s_menuHost.navigate(host, event, MenuCommand::Menu);
    }
    // update_survivors/open_window already waits for all host documents to hide.
    // Preserve the explicitly opened hub menu until the host menu is closed.
    *static_cast<bool*>(result) = true;
    return HOOK_SKIP_ORIGINAL;
}

void initialize_menu_handoff() {
    s_menuHost = {};
    s_menuDocument = s_talentDocument = nullptr;
    const auto unavailable = [](const char* name) {
        const auto message = std::string("Survivors menu handoff unavailable: ") + name +
            "; continuing without menu handoff";
        svc_log->warn(mod_ctx, message.c_str());
    };
    const auto resolve = [&](const char* name, auto& function) {
        void* address = nullptr;
        if (svc_hook->resolve(mod_ctx, name, &address, nullptr) != MOD_OK || !address) {
            unavailable(name);
            return false;
        }
        function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(address);
        return true;
    };
    // Virtual command handlers have callable vtable entries even when the host
    // inlines Document::handle_nav_event. No separate key mapping is required.
    if (!resolve("dusk::ui::top_document", s_menuHost.top) ||
        !resolve("dusk::ui::Document::handle_nav_command", s_menuHost.navigate)) return;
    if (!SurvivorsMenuWindow::resolved_target() || !SurvivorsMenuModal::resolved_target()) {
        unavailable("Window/Modal::handle_nav_command");
        return;
    }
    if (mods::hook::add_pre<SurvivorsMenuWindow>(svc_hook, before_survivors_menu_command) != MOD_OK ||
        mods::hook::add_pre<SurvivorsMenuModal>(svc_hook, before_survivors_menu_command) != MOD_OK) {
        // Any already installed callback remains a pass-through while disabled.
        // A private UI integration must never prevent the game mode from loading.
        unavailable("command hook installation");
        return;
    }
    s_menuHost.enabled = true;
}

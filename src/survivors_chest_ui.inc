// Native UI rows/buttons provide controller focus, touch activation and Cancel.
// The complete card is the button's hit target; its display uses native item icons.
constexpr const char* kTreasureStyle = R"(
window content pane.survivors-treasure-pane { border-right-width: 0; padding: 16dp; font-size: 16dp; }
ui-row.survivors-treasure-row { gap: 12dp; align-items: stretch; }
ui-row.survivors-treasure-card { display: block; position: relative; flex: 1 1 0; min-width: 0; background-color: #18251fee; border-radius: 8dp; }
.survivors-treasure-copy, .survivors-treasure-copy * { pointer-events: none; }
.survivors-treasure-copy { padding: 16dp; min-height: 330dp; text-align: center; font-size: 16dp; line-height: 1.35; }
.survivors-treasure-copy img { display: block; width: 80dp; height: 80dp; margin: 0 auto 12dp; }
.survivors-treasure-name { display: block; color: #f4d78b; font-size: 20dp; font-weight: bold; margin-bottom: 10dp; }
.survivors-treasure-effect { display: block; margin-bottom: 12dp; }
.survivors-treasure-change { display: block; margin-bottom: 5dp; }
.survivors-treasure-delta { color: #ff7373; white-space: nowrap; }
button.survivors-treasure-pick { position: absolute; left: 0; top: 0; width: 100%; height: 100%; box-sizing: border-box; z-index: 2; padding: 0; font-size: 0; color: transparent; background-color: transparent; border: 2dp #788873; border-radius: 8dp; }
button.survivors-treasure-pick:not(:disabled):hover,
button.survivors-treasure-pick:not(:disabled):focus-visible,
button.survivors-treasure-pick:not(:disabled):selected { color: transparent; background-color: #e5bc3b18; border-color: #ffdc75; }
button.survivors-treasure-pick:not(:disabled):active { color: transparent; background-color: #e5bc3b35; border-color: #fff1bd; }
@media (max-width: 640dp) {
    window content pane.survivors-treasure-pane { padding: 8dp; font-size: 14dp; }
    ui-row.survivors-treasure-row { gap: 6dp; }
    .survivors-treasure-copy { padding: 8dp; font-size: 13dp; }
    .survivors-treasure-copy img { width: 56dp; height: 56dp; }
    .survivors-treasure-name { font-size: 16dp; }
}
)";
std::string treasure_rml_escape(const std::string& text) {
    std::string escaped;
    for (char c : text) {
        if (c == '&') escaped += "&amp;";
        else if (c == '<') escaped += "&lt;";
        else if (c == '>') escaped += "&gt;";
        else if (c == '"') escaped += "&quot;";
        else escaped += c;
    }
    return escaped;
}
std::string treasure_card_rml(ChestReward reward, const Run& preview) {
    char icon[48];
    std::snprintf(icon, sizeof(icon), "<img src=\"item://item/%02x\"/>", unsigned(treasure_item(reward, preview)));
    std::string rml = "<div class=\"survivors-treasure-copy\">";
    rml += icon;
    rml += "<div class=\"survivors-treasure-name\">" + treasure_rml_escape(reward_name(reward, preview)) + "</div>";
    auto effect = item_description(reward, preview);
    effect = effect.substr(0, effect.find('\n'));
    rml += "<div class=\"survivors-treasure-effect\">" + treasure_rml_escape(effect) + "</div>";
    for (const auto& change : treasure_changes(reward, s_run, preview)) {
        rml += "<div class=\"survivors-treasure-change\">" + treasure_rml_escape(change.label) +
            ": <span class=\"survivors-treasure-delta\">" + treasure_rml_escape(change.before) +
            " -&gt; " + treasure_rml_escape(change.after) + "</span></div>";
    }
    return rml + "</div>";
}
ModResult build_treasure_choices(UiElementHandle left, UiElementHandle right) {
    if (s_selectedChest >= s_chests.size()) return MOD_INVALID_ARGUMENT;
    auto result = svc_ui->elem_set_visible(mod_ctx, right, false);
    if (result != MOD_OK) return result;
    result = svc_ui->elem_set_class(mod_ctx, left, "survivors-treasure-pane", true);
    if (result != MOD_OK) return result;
    const std::string hint = "Choose one reward for " + number(s_run.chest_cost()) +
        " rupees. Left/Right + Confirm, or tap a card. Cancel keeps these offers.";
    result = svc_ui->pane_add_text(mod_ctx, left, hint.c_str(), nullptr);
    if (result != MOD_OK) return result;
    UiRowDesc rowDesc = UI_ROW_DESC_INIT; // No wrapping: three cards side by side.
    UiElementHandle row = 0;
    result = svc_ui->pane_add_row(mod_ctx, left, &rowDesc, &row);
    if (result != MOD_OK) return result;
    result = svc_ui->elem_set_class(mod_ctx, row, "survivors-treasure-row", true);
    if (result != MOD_OK) return result;
    for (unsigned i = 0; i < 3; ++i) {
        const auto reward = s_chests[s_selectedChest].offers[i];
        Run preview = s_run; preview.phase = Phase::Treasure;
        unsigned rupees = dComIfGs_getRupee();
        if (!preview.buy_chest(rupees, reward, potion_slot() >= 0)) continue;
        UiElementHandle card = 0, pick = 0;
        result = svc_ui->pane_add_row(mod_ctx, row, &rowDesc, &card);
        if (result != MOD_OK) return result;
        result = svc_ui->elem_set_class(mod_ctx, card, "survivors-treasure-card", true);
        if (result != MOD_OK) return result;
        const auto rml = treasure_card_rml(reward, preview);
        result = svc_ui->pane_add_rml(mod_ctx, card, rml.c_str(), nullptr);
        if (result != MOD_OK) return result;
        UiControlDesc control = UI_CONTROL_DESC_INIT;
        control.label = reward_name(reward, preview);
        control.on_pressed = select_treasure; control.user_data = &s_choices[i];
        result = svc_ui->pane_add_control(mod_ctx, card, &control, &pick);
        if (result != MOD_OK) return result;
        result = svc_ui->elem_set_class(mod_ctx, pick, "survivors-treasure-pick", true);
        if (result != MOD_OK) return result;
    }
    return MOD_OK;
}

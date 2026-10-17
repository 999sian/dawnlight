// Native UI rows/buttons provide controller focus, touch activation and Cancel.
// The complete card is the button's hit target; its display uses native item icons.
constexpr const char* kTreasureStyle = R"(
window content pane.survivors-treasure-pane { border-right-width: 0; padding: 16dp; font-size: 16dp; }
ui-row.survivors-treasure-row { gap: 12dp; align-items: stretch; }
ui-row.survivors-treasure-card { display: block; position: relative; flex: 1 1 0; min-width: 0; background-color: #18251fee; border-radius: 8dp; }
.survivors-treasure-copy, .survivors-treasure-copy * { pointer-events: none; }
.survivors-treasure-copy { padding: 16dp; min-height: 330dp; text-align: center; font-size: 16dp; line-height: 1.35; }
.survivors-treasure-copy img { display: block; width: 80dp; height: 80dp; margin: 0 auto 12dp; border: 3dp #f4d78b; border-radius: 8dp; padding: 6dp; }
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
std::string choice_rml_start(const char* icon, const char* name, Rarity rarity) {
    const std::string color = rarity_colors[unsigned(rarity)];
    return "<div class=\"survivors-treasure-copy\"><img src=\"" + treasure_rml_escape(icon) +
        "\" style=\"border-color: " + color + ";\"/><div class=\"survivors-treasure-name\" style=\"color: " + color +
        ";\">" + treasure_rml_escape(name) + "</div><div class=\"survivors-treasure-effect\">" +
        rarity_names[unsigned(rarity)] + " x" + number(reward_units(rarity)) + "</div>";
}
ModResult begin_choice_cards(UiElementHandle left, UiElementHandle right, const std::string& hint,
                             UiElementHandle& row) {
    auto result = svc_ui->elem_set_visible(mod_ctx, right, false);
    if (result != MOD_OK) return result;
    result = svc_ui->elem_set_class(mod_ctx, left, "survivors-treasure-pane", true);
    if (result != MOD_OK) return result;
    result = svc_ui->pane_add_text(mod_ctx, left, hint.c_str(), nullptr);
    if (result != MOD_OK) return result;
    UiRowDesc rowDesc = UI_ROW_DESC_INIT;
    result = svc_ui->pane_add_row(mod_ctx, left, &rowDesc, &row);
    if (result != MOD_OK) return result;
    return svc_ui->elem_set_class(mod_ctx, row, "survivors-treasure-row", true);
}
ModResult add_choice_card(UiElementHandle row, const std::string& rml, const char* label,
                          UiPressedFn pressed, void* data) {
    UiRowDesc rowDesc = UI_ROW_DESC_INIT;
    UiElementHandle card = 0, pick = 0;
    auto result = svc_ui->pane_add_row(mod_ctx, row, &rowDesc, &card);
    if (result != MOD_OK) return result;
    result = svc_ui->elem_set_class(mod_ctx, card, "survivors-treasure-card", true);
    if (result != MOD_OK) return result;
    result = svc_ui->pane_add_rml(mod_ctx, card, rml.c_str(), nullptr);
    if (result != MOD_OK) return result;
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.label = label; control.on_pressed = pressed; control.user_data = data;
    result = svc_ui->pane_add_control(mod_ctx, card, &control, &pick);
    if (result != MOD_OK) return result;
    return svc_ui->elem_set_class(mod_ctx, pick, "survivors-treasure-pick", true);
}

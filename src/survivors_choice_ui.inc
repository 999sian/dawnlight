// Native UI rows/buttons provide controller focus, touch activation and Cancel.
// The complete card is the button's hit target; its display uses native item icons.
constexpr const char* kTreasureStyle = R"(
ui-row.survivors-choice-toolbar { flex-shrink: 0; justify-content: space-between; gap: 12dp; margin-bottom: 12dp; }
button.survivors-reroll { flex: 0 0 auto; font-size: 16dp; padding: 10dp 14dp; }
.survivors-choice-loadout { flex: 1 1 0; }
.survivors-choice-status { display: flex; justify-content: space-between; align-items: center; gap: 16dp; }
.survivors-equipment { display: flex; justify-content: center; gap: 8dp; margin: 0 auto; }
.survivors-choice-counts { display: flex; align-items: center; gap: 12dp; font-size: 16dp; white-space: nowrap; }
.survivors-choice-counts icon { font-size: 24dp; margin-right: 4dp; }
.survivors-banish-color { color: #ff7373; }
button.survivors-card-banish { position: absolute; top: 6dp; right: 6dp; width: 36dp; height: 36dp; box-sizing: border-box; padding: 4dp; z-index: 3; font-family: var(--font-family-icons); font-size: 24dp; color: #ff7373; background-color: #18251fee; }
button.survivors-card-banish:not(:disabled):hover, button.survivors-card-banish:not(:disabled):focus-visible { color: #ff7373; }
.survivors-equipment-slot { width: 40dp; height: 40dp; border: 2dp #788873; border-radius: 5dp; padding: 4dp; }
.survivors-equipment-slot img { width: 100%; height: 100%; }
.survivors-replaces { display: block; font-size: 12dp; color: #ffbd80; margin-bottom: 8dp; }
window content pane.survivors-treasure-pane { border-right-width: 0; padding: 16dp; font-size: 16dp; }
ui-row.survivors-treasure-row { gap: 12dp; align-items: stretch; }
ui-row.survivors-treasure-card { display: block; position: relative; flex: 1 1 0; min-width: 0; background-color: #18251fee; border-radius: 8dp; }
.survivors-treasure-copy, .survivors-treasure-copy * { pointer-events: none; }
.survivors-treasure-copy { padding: 16dp; min-height: 330dp; text-align: center; font-size: 16dp; line-height: 1.35; }
.survivors-treasure-copy img { display: block; width: 80dp; height: 80dp; margin: 0 auto 12dp; border: 3dp #f4d78b; border-radius: 8dp; padding: 6dp; }
.survivors-treasure-name { display: block; color: #f4d78b; font-size: 20dp; font-weight: bold; margin-bottom: 10dp; }
.survivors-treasure-effect { display: block; margin-bottom: 12dp; }
.survivors-treasure-change { display: block; margin-bottom: 5dp; }
.survivors-treasure-delta { color: #ff7373; white-space: normal; word-break: break-word; }
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
    ui-row.survivors-choice-toolbar { gap: 6dp; }
    button.survivors-reroll { font-size: 13dp; padding: 8dp; }
    .survivors-choice-status { gap: 6dp; }
    .survivors-equipment { gap: 4dp; }
    .survivors-equipment-slot { width: 28dp; height: 28dp; padding: 2dp; }
    .survivors-choice-counts { gap: 6dp; font-size: 13dp; }
    .survivors-choice-counts icon { font-size: 20dp; margin-right: 2dp; }
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
std::string choice_rml_start(const char* icon, const char* name, Rarity rarity, const char* replacement = "") {
    const std::string color = rarity_colors[unsigned(rarity)];
    return "<div class=\"survivors-treasure-copy\"><img src=\"" + treasure_rml_escape(icon) +
        "\" style=\"border-color: " + color + ";\"/><div class=\"survivors-treasure-name\" style=\"color: " + color +
        ";\">" + treasure_rml_escape(name) + "</div>" + (*replacement ? "<div class=\"survivors-replaces\">" + treasure_rml_escape(replacement) + "</div>" : "") + "<div class=\"survivors-treasure-effect\">" +
        rarity_names[unsigned(rarity)] + " x" + number(reward_units(rarity)) + "</div>";
}
ModResult add_choice_header(UiElementHandle pane);
void banish_choice(ModContext*,void*);
ModResult begin_choice_cards(UiElementHandle left, UiElementHandle right, const std::string& hint,
                             UiElementHandle& row) {
    auto result = svc_ui->elem_set_visible(mod_ctx, right, false);
    if (result != MOD_OK) return result;
    result = svc_ui->elem_set_class(mod_ctx, left, "survivors-treasure-pane", true);
    if (result != MOD_OK) return result;
    result=add_choice_header(left);
    if (result != MOD_OK) return result;
    if (!hint.empty()) result = svc_ui->pane_add_text(mod_ctx, left, hint.c_str(), nullptr);
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
    result=svc_ui->elem_set_class(mod_ctx, pick, "survivors-treasure-pick", true);
    if(result!=MOD_OK)return result;
    // Separate sibling above the pick overlay: touch/Confirm only banishes.
    control=UI_CONTROL_DESC_INIT;control.label="\uE14B";control.tooltip="Banish this choice";
    control.on_pressed=banish_choice;control.user_data=data;
    control.is_disabled=[](ModContext*,void*){return !s_run.banishes;};
    UiElementHandle banish=0;
    result=svc_ui->pane_add_control(mod_ctx,card,&control,&banish);
    if(result!=MOD_OK)return result;
    return svc_ui->elem_set_class(mod_ctx,banish,"survivors-card-banish",true);
}

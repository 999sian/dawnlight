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
    UiElementHandle row = 0;
    auto result = begin_choice_cards(left, right, "Choose one reward for " + number(s_run.chest_cost()) +
        " rupees. Left/Right + Confirm, or tap a card. Cancel keeps these offers.", row);
    if (result != MOD_OK) return result;
    for (unsigned i = 0; i < 3; ++i) {
        const auto reward = s_chests[s_selectedChest].offers[i];
        Run preview = s_run; preview.phase = Phase::Treasure;
        unsigned rupees = dComIfGs_getRupee();
        if (!preview.buy_chest(rupees, reward, potion_slot() >= 0)) continue;
        result = add_choice_card(row, treasure_card_rml(reward, preview), reward_name(reward, preview),
                                 select_treasure, &s_choices[i]);
        if (result != MOD_OK) return result;
    }
    return MOD_OK;
}

std::string treasure_card_rml(ChestReward reward, const Run& preview, Rarity rarity) {
    char icon[48];
    std::snprintf(icon, sizeof(icon), "item://item/%02x", unsigned(treasure_item(reward, preview)));
    const char* replacement=s_run.replaces_offhand(reward) ? (reward==ChestReward::OrdonSword ? "replaces Shield" : "replaces Dual Wield") : "";
    std::string rml = choice_rml_start(icon, reward==ChestReward::OrdonSword ? "Dual Wield" : reward_name(reward, preview), rarity, replacement);
    auto effect = item_description(reward, preview);
    effect = effect.substr(0, effect.find('\n'));
    rml += "<div class=\"survivors-treasure-effect\">" + treasure_rml_escape(effect) + "</div>";
    for (const auto& change : treasure_changes(reward, s_run, preview, rarity)) {
        rml += "<div class=\"survivors-treasure-change\">" + treasure_rml_escape(change.label) +
            ": <span class=\"survivors-treasure-delta\">" + treasure_rml_escape(change.before) +
            " -&gt; " + treasure_rml_escape(change.after) + "</span></div>";
    }
    return rml + "</div>";
}
ModResult build_treasure_choices(UiElementHandle left, UiElementHandle right) {
    if (s_selectedChest >= s_chests.size()) return MOD_INVALID_ARGUMENT;
    UiElementHandle row = 0;
    const bool free = s_chests[s_selectedChest].free;
    auto result = begin_choice_cards(left, right, "", row);
    if (result != MOD_OK) return result;
    for (unsigned i = 0; i < 3; ++i) {
        const auto reward = s_chests[s_selectedChest].offers[i];
        const auto rarity = s_chests[s_selectedChest].rarities[i];
        Run preview = s_run; preview.phase = Phase::Treasure;
        unsigned rupees = dComIfGs_getRupee();
        if (!preview.buy_chest(rupees, reward, potion_capacity(), rarity, free)) continue;
        result = add_choice_card(row, treasure_card_rml(reward, preview, rarity), reward_name(reward, preview),
                                 select_treasure, &s_choices[i]);
        if (result != MOD_OK) return result;
    }
    return add_choice_actions(left,true);
}

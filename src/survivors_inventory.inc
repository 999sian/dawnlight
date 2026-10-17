// Keep message handles alive until the next presentation has replaced them.
// Native message processing can retain the text after the reward camera ends.
mods::flow::RegisteredMessage s_treasureMessage, s_itemNameMessage, s_itemDescriptionMessage;
mods::flow::RegisteredMessage register_survivors_message(const mods::flow::MessageBuilder& builder) {
    std::vector<mods::flow::MessageVariant> variants;
    for (auto language : {MESSAGE_LANGUAGE_ENGLISH, MESSAGE_LANGUAGE_GERMAN,
            MESSAGE_LANGUAGE_FRENCH, MESSAGE_LANGUAGE_SPANISH, MESSAGE_LANGUAGE_ITALIAN,
            MESSAGE_LANGUAGE_JAPANESE}) variants.push_back(builder.build(language));
    return mods::flow::register_message(0, variants);
}
bool prepare_treasure_message(ChestReward reward, const Run& before, const Run& after) {
    auto changes = reward_changes(reward, before, after);
    if (reward == ChestReward::RedPotion) {
        unsigned bottles = 0, red = 0;
        for (int slot = SLOT_11; slot <= SLOT_14; ++slot) {
            const auto item = dComIfGs_getItem(slot, false);
            bottles += item != dItemNo_NONE_e; red += item == dItemNo_RED_BOTTLE_e;
        }
        if (bottles < 4) changes.push_back({"Bottles", number(bottles), number(bottles + 1)});
        changes.push_back({"Red potions", number(red), number(red + 1)});
    }
    mods::flow::MessageBuilder builder;
    builder.box_kind(MESSAGE_BOX_ITEM_GET).box_position(MESSAGE_POSITION_MIDDLE)
        .text(reward_name(reward, after));
    for (const auto& change : changes) {
        builder.text("\n").text(change.label).text(": ")
            .text_color(MESSAGE_COLOR_RED).text(change.before).text(" -> ").text(change.after)
            .text_color(MESSAGE_COLOR_DEFAULT);
    }
    auto message = register_survivors_message(builder);
    if (!message) return false; // No event or payment with an unavailable reward message.
    s_treasureMessage = std::move(message);
    return true;
}
unsigned treasure_message_id() { return s_treasureMessage.id(); }

// Existing item slots for weapons; unused quest slots for the two swords and
// shield. This inventory belongs only to the isolated Survivors save.
constexpr std::array<int, 7> kSurvivorsSlots{SLOT_4, SLOT_0, SLOT_15, SLOT_6, SLOT_20, SLOT_21, SLOT_22};
void sync_survivors_inventory() {
    const std::array<u8, 7> items{
        s_run.rank(Upgrade::Bow) ? dItemNo_BOW_e : dItemNo_NONE_e,
        s_run.rank(Upgrade::Boomerang) ? dItemNo_BOOMERANG_e : dItemNo_NONE_e,
        s_run.rank(Upgrade::Bombs) ? dItemNo_NORMAL_BOMB_e : dItemNo_NONE_e,
        s_run.rank(Upgrade::Ball) ? dItemNo_IRONBALL_e : dItemNo_NONE_e,
        s_run.master_sword ? (s_run.master_sword >= 5 ? dItemNo_LIGHT_SWORD_e : dItemNo_MASTER_SWORD_e) : dItemNo_NONE_e,
        s_run.dual_wield ? dItemNo_SWORD_e : dItemNo_NONE_e,
        u8(s_run.shield ? shield_item(s_run) : dItemNo_NONE_e)};
    for (unsigned i = 0; i < items.size(); ++i)
        if (dComIfGs_getItem(kSurvivorsSlots[i], false) != items[i])
            dComIfGs_setItem(kSurvivorsSlots[i], items[i]);
}
bool survivors_slot_assignable(u8 slot) {
    return slot == dItemNo_NONE_e || (slot >= SLOT_11 && slot <= SLOT_14 &&
        dComIfGs_getItem(slot, false) != dItemNo_NONE_e);
}
HookAction before_ring_assignment(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!in_arena() || !ring || ring->mPlayerIsWolf || ring->mCurrentSlot >= MAX_ITEM_SLOTS) return HOOK_CONTINUE;
    return survivors_slot_assignable(ring->mItemSlots[ring->mCurrentSlot]) ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}
HookAction before_item_binding(ModContext*, void* args, void*, void*) {
    if (!in_arena() || mods::arg<dSv_player_status_a_c*>(args, 0) !=
        &g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA()) return HOOK_CONTINUE;
    // Covers native X/Y and service/mod-driven extra bindings as well.
    return survivors_slot_assignable(mods::arg<u8>(args, 2)) ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}
HookAction before_item_mix(ModContext*, void* args, void*, void*) {
    if (!in_arena() || mods::arg<dSv_player_status_a_c*>(args, 0) !=
        &g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA()) return HOOK_CONTINUE;
    return mods::arg<u8>(args, 2) == dItemNo_NONE_e ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}
ChestReward inventory_reward(u8 slot) {
    for (unsigned i = 0; i < kSurvivorsSlots.size(); ++i) if (slot == kSurvivorsSlots[i]) {
        if (i < 4) return static_cast<ChestReward>(i);
        return i == 4 ? ChestReward::MasterSword : i == 5 ? ChestReward::OrdonSword : ChestReward::Shield;
    }
    return slot >= SLOT_11 && slot <= SLOT_14 && dComIfGs_getItem(slot, false) == dItemNo_RED_BOTTLE_e ?
        ChestReward::RedPotion : ChestReward::Count;
}
void after_item_explain(ModContext*, void* args, void* result, void*) {
    if (!in_arena() || !result || !*static_cast<u8*>(result)) return;
    auto* explain = mods::arg<dMenu_ItemExplain_c*>(args, 0);
    const auto reward = inventory_reward(mods::arg<u8>(args, 1));
    if (!explain || reward == ChestReward::Count) return;
    auto name = register_survivors_message(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_ITEM_NAME).text(reward_name(reward, s_run)));
    auto description = register_survivors_message(mods::flow::MessageBuilder{}
        .box_kind(MESSAGE_BOX_ITEM_NAME).text_scale(80).text(item_description(reward, s_run)));
    if (!name || !description) return;
    s_itemNameMessage = std::move(name); s_itemDescriptionMessage = std::move(description);
    explain->field_0xcc = s_itemNameMessage.id();
    explain->field_0xc8 = explain->field_0xc4 = s_itemDescriptionMessage.id();
    // Automatic weapons have unlimited run ammo, not the native save's counts.
    if (reward <= ChestReward::Ball) { explain->field_0xde = explain->field_0xdf = 0; explain->setNumber(); }
}

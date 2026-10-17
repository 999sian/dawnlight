// Native DEFAULT_GETITEM presentation, following the Glider reward's actor
// ownership protocol. The prop never grants inventory or story treasure flags.
struct TreasureDemo {
    ActorId item = kNone;
    bool started = false, poseStarted = false, retiring = false;
    unsigned waitFrames = 0;
} s_treasure;

daDitem_c* treasure_actor() {
    auto* actor = actor_by_id(s_treasure.item);
    return actor && fopAcM_GetName(actor) == fpcNm_Demo_Item_e ? static_cast<daDitem_c*>(actor) : nullptr;
}
bool owns_treasure_event() {
    return s_treasure.item != kNone && dComIfGp_getEvent()->mPt2 == s_treasure.item;
}
void retire_treasure_actor() {
    if (s_treasure.item != kNone && fpcM_IsCreating(s_treasure.item)) {
        for (auto* node = g_fpcCtTg_Queue.mpHead; node; node = NODE_GET_NEXT(node)) {
            auto* request = static_cast<create_request*>(reinterpret_cast<create_tag*>(node)->base.mpTagData);
            if (request && request->id == s_treasure.item) {
                if (fpcCtRq_IsDoing(request) || !fpcCtRq_Cancel(request)) return;
                break;
            }
        }
        if (fpcM_IsCreating(s_treasure.item)) return;
    }
    if (auto* item = treasure_actor(); item && !fopAcM_delete(item)) return;
    s_treasure = {};
}
void cancel_treasure_demo() {
    // A save/scene reset can already have installed another event. Only reset
    // our own event while its arena is still current.
    if (in_arena() && !transitioning() && owns_treasure_event()) dComIfGp_event_reset();
    s_treasure.retiring = true;
    retire_treasure_actor();
    if (s_selectedChest < s_chests.size() && !s_chests[s_selectedChest].opened) {
        s_chests[s_selectedChest].opening = false; s_chests[s_selectedChest].age = 0;
    }
    s_selectedChest = kChestCount;
}
bool chest_interactable(const Chest& chest, daAlink_c* link) {
    if (!chest.active || chest.opened || !s_loot.ready || s_run.phase != Phase::Running ||
        !playable() || s_treasure.item != kNone || !s_run.chest_rewards_left() ||
        dComIfGs_getRupee() < s_run.chest_cost() || !sword_action_available(link)) return false;
    const cXyz delta = chest.position - link->current.pos;
    const s16 facing = static_cast<s16>(cM_atan2s(delta.x, delta.z) - link->shape_angle.y);
    if (delta.absXZ() >= 150 || std::abs(delta.y) >= 20 || std::abs(int(facing)) > 0x2000) return false;
    const cXyz from = link->current.pos + cXyz(0, 50, 0), to = chest.position + cXyz(0, 50, 0);
    dBgS_LinChk wall; wall.Set(&from, &to, link);
    return !dComIfG_Bgsp().LineCross(&wall);
}
bool begin_treasure_demo(unsigned index, daAlink_c* link) {
    if (index >= s_chests.size() || !chest_interactable(s_chests[index], link)) return false;
    auto* layer = link->layer_tag.layer;
    if (!layer || layer == fpcLy_RootLayer() || fpcLy_IsDeletingMesg(layer)) return false;
    Run preview = s_run; preview.phase = Phase::Treasure;
    unsigned rupees = dComIfGs_getRupee(); Upgrade reward = Upgrade::Count;
    if (!preview.buy_chest(rupees, reward)) return false;
    constexpr std::array<u8, 4> items{dItemNo_BOW_e, dItemNo_BOOMERANG_e, dItemNo_BOMB_5_e, dItemNo_IRONBALL_e};
    auto* previous = fpcLy_CurrentLayer(); fpcLy_SetCurrentLayer(layer);
    // 0x01 is native presentation-only: daDitem_c skips execItemGet.
    const auto item = fopAcM_createDemoItem(&link->current.pos, items[unsigned(reward) - unsigned(Upgrade::Bow)],
                                          -1, nullptr, fopAcM_GetRoomNo(link), nullptr, 0x01);
    fpcLy_SetCurrentLayer(previous);
    if (item == kNone) return false;
    s_treasure = {}; s_treasure.item = item;
    s_selectedChest = index; s_chests[index].reward = reward;
    s_run.phase = Phase::Treasure;
    clear_attacks();
    return true;
}
HookAction before_chest_action(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !in_arena() || fopAcM_GetID(link) != s_player) return HOOK_CONTINUE;
    for (unsigned i = 0; i < s_chests.size(); ++i) {
        if (!chest_interactable(s_chests[i], link)) continue;
        link->setDoStatus(BUTTON_STATUS_OPEN);
        if (!link->doTrigger()) return HOOK_CONTINUE;
        begin_treasure_demo(i, link);
        *static_cast<int*>(result) = 1; // Consume Open; never turn it into a roll.
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void retain_treasure_actor(fopAc_ac_c* actor, void* args) {
    if (!s_treasure.started || s_treasure.retiring || !owns_treasure_event() ||
        fopAcM_GetID(actor) != s_treasure.item || !actor->sub_method) return;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (mods::arg<process_method_func>(args, 0) != methods->execute_method) return;
    auto* item = static_cast<daDitem_c*>(actor);
    // The event's partner must survive until the camera/event finishes.
    if (item->mAction == daDitem_c::ACTION_END_e) item->setAction(daDitem_c::ACTION_WAIT_LIGHT_END_e);
}
HookAction before_treasure_get(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* item = treasure_actor();
    if (!link || !item || s_treasure.retiring || !owns_treasure_event() ||
        !item->eventInfo.checkCommandItem() || fopAcM_GetID(link) != s_player) return HOOK_CONTINUE;
    s_treasure.started = true;
    dComIfGp_event_setItemPartnerId(s_treasure.item);
    dComIfGp_event_setGtItm(item->getItemNo());
    link->mDemo.setParam0(0); link->mDemo.setParam1(0);
    return HOOK_CONTINUE;
}
void after_treasure_get(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !s_treasure.started || s_treasure.poseStarted || !owns_treasure_event() ||
        fopAcM_GetID(link) != s_player || link->mProcID != daAlink_c::PROC_GET_ITEM) return;
    link->setSingleAnimeBase(daAlink_c::ANM_GET_A);
    link->mProcVar1.field_0x300a = 0;
    s_treasure.poseStarted = true;
}
void update_treasure_demo(daAlink_c* link) {
    if (s_run.phase != Phase::Treasure) return;
    auto* item = treasure_actor();
    if (item && owns_treasure_event() && item->eventInfo.checkCommandItem()) s_treasure.started = true;
    if (s_treasure.started && owns_treasure_event()) {
        auto& chest = s_chests[s_selectedChest];
        chest.opening = true; ++chest.age;
        if (!dComIfGp_evmng_endCheck("DEFAULT_GETITEM")) return;
        // RNG and ranks are frozen throughout the demo. Commit the same reward
        // exactly once, only after the native animation/message completes.
        unsigned rupees = dComIfGs_getRupee();
        if (s_run.buy_chest(rupees, chest.reward)) {
            dComIfGs_setRupee(rupees); chest.opened = true; chest.age = 0;
        }
        cancel_treasure_demo(); s_run.phase = Phase::Running;
        return;
    }
    if (s_treasure.started || ++s_treasure.waitFrames > 300 ||
        (!item && !fpcM_IsCreating(s_treasure.item))) {
        cancel_treasure_demo(); s_run.phase = Phase::Running;
        return; // Failed/canceled demo: no charge, rank or price change.
    }
    if (item && playable() && sword_action_available(link)) {
        item->eventInfo.onCondition(dEvtCnd_CANGETITEM_e);
        fopAcM_orderItemEvent(item, 0, 0);
    }
}

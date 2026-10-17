// Native upper-body animations with run-owned held models/projectiles. No
// native item actor, inventory assignment or ammo consumption is synthesized.
struct RangedAction {
    bool active = false, released = false;
    Upgrade weapon = Upgrade::Bow;
    u16 animation = 0;
    cXyz target{};
    unsigned projectile = projectile_capacity;
    bool pulling = false, catching = false;
} s_ranged;
unsigned s_rangedRest = 0;
Upgrade s_lastRanged = Upgrade::Ball;

void cancel_ranged() {
    auto* link = daAlink_getAlinkActorClass();
    if (s_ranged.active && link && fopAcM_GetID(link) == s_player &&
        link->checkUpperAnime(s_ranged.animation)) {
        link->resetUpperAnime(daAlink_c::UPPER_2, 3);
        link->mBodyAngle.x = link->mBodyAngle.y = 0;
    }
    s_rangedRest = s_ranged.active ? 6 : 0;
    s_ranged = {};
}
void ranged_anime(daAlink_c* link, u16 animation, unsigned ticks) {
    s_ranged.animation = animation;
    link->setUpperAnimeBaseSpeed(animation, 1, 2);
    auto& frame = link->mUpperFrameCtrl[2];
    frame.setRate(std::max(1.0f, float(frame.getEnd()) / std::max(3u, ticks)));
}
bool queue_ranged(daAlink_c* link, Upgrade weapon) {
    if (s_ranged.active || s_rangedRest || !s_loot.ready || !playable() || link->checkEquipAnime() ||
        (link->mEquipItem != dItemNo_NONE_e && link->mEquipItem != 0x103)) return false;
    // Round-robin among all due item weapons so high-rate arrows cannot starve throws.
    for (unsigned step = 1; step <= 4; ++step) {
        const unsigned candidate = unsigned(Upgrade::Bow) + (unsigned(s_lastRanged) - unsigned(Upgrade::Bow) + step) % 4;
        if (s_run.ranks[candidate] && !s_cooldowns[candidate]) {
            if (candidate != unsigned(weapon)) return false;
            break;
        }
    }
    const bool recover = normal_sword_proc(link) && link->mUnderFrameCtrl[0].getFrame() >
        std::max(link->field_0x347c, link->field_0x3484);
    if (!sword_action_available(link) && !recover) return false;
    cXyz target;
    if (!nearest_item_target(link, target)) return false;
    if (recover) link->procWaitInit();
    link->deleteEquipItem(FALSE, FALSE); // Sheathe the sword while the held item is visible.
    s_swordAction = SwordAction::None; s_wavePending = false;
    // Resume the sword combo after this automatic item animation.
    s_lastRanged = weapon;
    s_ranged.active = true; s_ranged.weapon = weapon; s_ranged.target = target;
    const unsigned interval = s_run.item_cooldown(weapon);
    s_cooldowns[unsigned(weapon)] = interval;
    const u16 animation = weapon == Upgrade::Bow ? dRes_ID_ALANM_BCK_ARELORD_e :
        weapon == Upgrade::Boomerang ? dRes_ID_ALANM_BCK_BOOMTHROW_e :
        weapon == Upgrade::Bombs ? dRes_ID_ALANM_BCK_BOMBTHROW_e : dRes_ID_ALANM_BCK_IBTHROW_e;
    ranged_anime(link, animation,
                 interval / (weapon == Upgrade::Bow ? 3 : 2));
    return true;
}
void release_ranged(daAlink_c* link) {
    cXyz target;
    if (!nearest_item_target(link, target)) return;
    s_ranged.target = target;
    if (!launch_item(link, s_ranged.weapon, target)) return;
    if (s_ranged.weapon == Upgrade::Ball) {
        for (unsigned i = kItemBegin + 3 * kSlotsPerItem; i < s_attacks.size(); ++i)
            if (s_attacks[i].active && s_attacks[i].age == 0) { s_ranged.projectile = i; break; }
    }
    if (s_ranged.weapon == Upgrade::Bow && s_run.rank(Upgrade::Bow) >= 3) {
        launch_item(link, Upgrade::Bow, target, 0.14f);
        if (s_run.rank(Upgrade::Bow) >= 5) launch_item(link, Upgrade::Bow, target, -0.14f);
    }
}
void update_ranged(daAlink_c* link) {
    if (s_rangedRest) --s_rangedRest;
    if (!s_ranged.active) return;
    if (!sword_action_available(link) || !link->checkUpperAnime(s_ranged.animation)) {
        cancel_ranged(); return; // Damage, drinking, jumping or another native action wins.
    }
    auto& frame = link->mUpperFrameCtrl[2];
    if (s_ranged.weapon == Upgrade::Ball && s_ranged.released) {
        const auto* ball = s_ranged.projectile < s_attacks.size() ? &s_attacks[s_ranged.projectile] : nullptr;
        if (s_ranged.catching) { if (link->checkAnmEnd(&frame)) cancel_ranged(); return; }
        if (!ball || !ball->active) {
            s_ranged.catching = true;
            ranged_anime(link, dRes_ID_ALANM_BCK_IBCATCH_e, s_run.item_cooldown(Upgrade::Ball) / 4);
        } else if (ball->returning && !s_ranged.pulling) {
            s_ranged.pulling = true;
            ranged_anime(link, dRes_ID_ALANM_BCK_IBPULL_e, s_run.item_cooldown(Upgrade::Ball) / 4);
        } else if (link->checkAnmEnd(&frame)) {
            if (s_ranged.pulling) frame.setFrame(0);
            else frame.setRate(0); // Hold the throw pose until the chain starts retracting.
        }
        return;
    }
    if (s_ranged.weapon == Upgrade::Bow && !s_ranged.released) {
        if (!link->checkAnmEnd(&frame)) return;
        ranged_anime(link, dRes_ID_ALANM_BCK_ASHOOT_e, s_run.item_cooldown(Upgrade::Bow) / 3);
        s_ranged.released = true; release_ranged(link);
    } else {
        // Native release frames: boomerang 5.5, side-thrown bomb 4, iron ball 6.
        const float release = s_ranged.weapon == Upgrade::Bombs ? 4 : s_ranged.weapon == Upgrade::Ball ? 6 : 5.5f;
        if (!s_ranged.released && frame.getFrame() >= release) {
            s_ranged.released = true; release_ranged(link);
        }
        if (link->checkAnmEnd(&frame)) cancel_ranged();
    }
}
HookAction before_ranged_native(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_ranged.active || !in_arena() || !link || fopAcM_GetID(link) != s_player ||
        !link->checkUpperAnime(s_ranged.animation)) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 0;
    // IBTHROW also passes checkIronBallAnime(). Its native action handler uses
    // the locomotion frame as a wind-up timer, and its next-action handler
    // enters IRON_BALL_MOVE/SUBJECT without an equipped ball. Both must yield
    // to our release/pull/catch scheduler, just like manual bow targeting.
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_ranged_aim(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_ranged.active || !link || fopAcM_GetID(link) != s_player || !playable() ||
        !sword_action_available(link) || !link->checkUpperAnime(s_ranged.animation)) return HOOK_CONTINUE;
    if (!s_ranged.released) nearest_item_target(link, s_ranged.target);
    const cXyz delta = s_ranged.target - link->current.pos - cXyz(0, 70, 0);
    link->shape_angle.y = cM_atan2s(delta.x, delta.z);
    link->mBodyAngle.y = 0;
    link->mBodyAngle.x = -cM_atan2s(delta.y, std::max(1.0f, delta.absXZ()));
    // current.angle controls travel; changing only shape/body preserves movement.
    return HOOK_CONTINUE;
}
void draw_ranged() {
    auto* link = daAlink_getAlinkActorClass();
    if (!s_ranged.active || !s_loot.ready || !link || fopAcM_GetID(link) != s_player ||
        !link->checkUpperAnime(s_ranged.animation)) return;
    const bool bow = s_ranged.weapon == Upgrade::Bow;
    if (!bow && s_ranged.released && !s_ranged.catching) return;
    auto* model = bow ? s_loot.bow : s_ranged.weapon == Upgrade::Bombs ? s_loot.heldBomb :
        s_ranged.weapon == Upgrade::Ball ? s_loot.heldBall : s_loot.heldBoomerang;
    mDoMtx_stack_c::copy(bow ? link->getRightItemMatrix() : link->getLeftItemMatrix());
    if (s_ranged.weapon == Upgrade::Boomerang) { // Native hand attachment.
        mDoMtx_stack_c::transM(32, -5, -6);
        mDoMtx_stack_c::XYZrotM(cM_deg2s(-4), cM_deg2s(39), cM_deg2s(-9));
    }
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    auto* data = model->getModelData(); auto* root = data->getJointNodePointer(0);
    auto* previous = root->getMtxCalc();
    if (bow) {
        auto* animation = s_loot.bowAnims[s_ranged.released ? 1 : 0];
        animation->entry(data, std::min(link->mUpperFrameCtrl[2].getFrame(), animation->getEndFrame()));
    }
    dKy_tevstr_c tev{}; g_env_light.settingTevStruct(0, &link->current.pos, &tev);
    g_env_light.setLightTevColorType_MAJI(model, &tev);
    mDoExt_modelUpdateDL(model);
    root->setMtxCalc(previous);
}

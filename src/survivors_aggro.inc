// Awareness is unlimited for living arena enemies. Keep real player distances
// for native movement, attack reach, lunges, collision and obstacle handling.
bool arena_aggro(fopAc_ac_c* actor, ProfileName profile) {
    if (!actor || actor->health <= 0 || s_run.phase != Phase::Running || !playable()) return false;
    const auto* enemy = tracked(fopAcM_GetID(actor));
    return enemy && enemy->profile == profile && enemy->initialized && !enemy->retiring && !enemy->credited && !enemy->dormant;
}
HookAction before_bokoblin_aggro(ModContext*, void* args, void* result, void*) {
    auto* actor = mods::arg<daE_OC_c*>(args, 0);
    if (actor && in_arena()) if (const auto* enemy = tracked(fopAcM_GetID(actor)); enemy && enemy->dormant) {
        *static_cast<bool*>(result) = false; return HOOK_SKIP_ORIGINAL;
    }
    if (!arena_aggro(actor, fpcNm_E_OC_e)) return HOOK_CONTINUE;
    *static_cast<bool*>(result) = true;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_rat_aggro(ModContext*, void* args, void* result, void*) {
    auto* actor = mods::arg<e_ms_class*>(args, 0);
    if (!arena_aggro(actor, fpcNm_E_MS_e)) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_keese_aggro(ModContext*, void* args, void* result, void*) {
    auto* bat = mods::arg<e_ba_class*>(args, 0);
    if (!bat || bat->mIsDying || !arena_aggro(&bat->mEnemy, fpcNm_E_BA_e)) return HOOK_CONTINUE;
    const auto range = mods::arg<f32>(args, 1);
    // pl_check also gates the close-range hover/attack transition. Override
    // acquisition and disengagement only, otherwise distant bats stop chasing.
    if (range != bat->mFightFlyDistance && range != bat->mFightFlyDistance + 50.0f) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = TRUE;
    return HOOK_SKIP_ORIGINAL;
}
void engage_enemy(fopAc_ac_c* actor, const Enemy& enemy) {
    if (!arena_aggro(actor, enemy.profile)) return;
    if (enemy.profile == fpcNm_E_MS_e) {
        auto* rat = static_cast<e_ms_class*>(actor);
        if (rat->mAction == 0) { rat->mAction = 1; rat->mMode = 0; }
    } else if (enemy.profile == fpcNm_E_OC_e) {
        auto* bokoblin = static_cast<daE_OC_c*>(actor);
        bokoblin->mMoveRange = 0; // Disable the native home-position leash.
        bokoblin->field_0x6e3 = 0;
        if (bokoblin->getActionMode() <= 2) bokoblin->setActionMode(3, 0);
    } else if (enemy.profile == fpcNm_E_BA_e) {
        auto* bat = reinterpret_cast<e_ba_class*>(actor);
        if (!bat->mIsDying && (bat->mAction == e_ba_class::ACT_ROOF || bat->mAction == e_ba_class::ACT_RETURN ||
            bat->mAction == e_ba_class::ACT_FLY || bat->mAction == e_ba_class::ACT_PATH_FLY)) {
            bat->mAction = e_ba_class::ACT_FIGHT_FLY; bat->mMode = 0;
        }
    }
}

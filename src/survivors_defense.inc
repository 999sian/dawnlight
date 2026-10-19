bool shield_pose(const daAlink_c* link) {
    return link && (link->mProcID == daAlink_c::PROC_GUARD_ATTACK ||
        link->mProcID == daAlink_c::PROC_GUARD_SLIP || link->mProcID == daAlink_c::PROC_GUARD_BREAK);
}
void interrupt_auto_actions(daAlink_c* link) {
    cancel_ranged(); s_swordAction = SwordAction::None; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
    for (auto& collider : link->mAtCps) { collider.OffAtSetBit(); collider.ResetAtHit(); }
    link->mAtSph.OffAtSetBit(); link->mAtSph.ResetAtHit();
    link->resetUpperAnime(daAlink_c::UPPER_2, 0);
}
HookAction before_shield_block(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (s_victory && in_arena() && link && fopAcM_GetID(link)==s_player) {
        for (auto& collider : link->mTgCyls) collider.ResetTgHit();
        *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
    }
    begin_player_damage(args,link);
    if (!link || fopAcM_GetID(link) != s_player || !playable() || s_run.phase != Phase::Running ||
        link->checkWolf() || link->mDamageTimer) return HOOK_CONTINUE;
    for (auto& target : link->mTgCyls) {
        if (!target.ChkTgHit()) continue;
        auto* attacker = target.GetTgHitAc();
        auto* attack = target.GetTgHitGObj();
        if (!attacker || (!tracked(fopAcM_GetID(attacker)) && !active_enemy_arrow(fopAcM_GetID(attacker))) || !attack) continue;
        if (s_run.roll_chance(s_run.talents.dodge_basis_points())) {
            for (auto& collider : link->mTgCyls) collider.ResetTgHit();
            combat_feedback(link,CombatText::Dodge);
            *static_cast<int*>(result)=1;
            return HOOK_SKIP_ORIGINAL;
        }
        if (!s_run.owns(ChestReward::Shield) || auto_attacks_suspended(link) ||
            s_run.next_random() % 100 >= s_run.block_chance()) return HOOK_CONTINUE;
        interrupt_auto_actions(link);
        const cXyz direction = attacker->current.pos - link->current.pos;
        link->shape_angle.y = cM_atan2s(direction.x, direction.z);
        link->field_0x2fd5 = 0; // Chance-based blocks do not use vanilla's fourth-hit guard break.
        link->setGuardSe(&target);
        link->procGuardSlipInit(attack->GetAtSpl(), &target);
        combat_feedback(link,CombatText::Block);
        for (auto& collider : link->mTgCyls) collider.ResetTgHit();
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
HookAction before_survivors_guard(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!in_arena() || !link || fopAcM_GetID(link) != s_player) return HOOK_CONTINUE;
    if (s_run.owns(ChestReward::Shield) && shield_pose(link)) return HOOK_CONTINUE;
    link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    return HOOK_SKIP_ORIGINAL; // Idle lock-on must not replace the probabilistic block with free guarding.
}
// Knockdown is a reaction, not immunity. Enable targets before native cc_set
// registers them: changing Tg bits after Execute is too late for cCcS::Set.
HookAction before_bokoblin_targets(ModContext*, void* args, void*, void*) {
    auto* actor=mods::arg<daE_OC_c*>(args,0);
    if (!actor || !in_arena() || s_run.phase!=Phase::Running || actor->health<=0 ||
        actor->mActionMode!=6) return HOOK_CONTINUE; // Native E_OC_ACTION_BIG_DAMAGE.
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if (!enemy || enemy->profile!=fpcNm_E_OC_e || !enemy->initialized || enemy->retiring || enemy->credited)
        return HOOK_CONTINUE;
    for (auto& sphere:actor->mSphs_cc) sphere.OnTgSetBit();
    return HOOK_CONTINUE;
}
struct RatRecovery { void* args; e_ms_class* actor; s16 cooldown; };
std::vector<RatRecovery> s_ratRecoveries;
HookAction before_rat_recovery(ModContext*, void* args, void*, void*) {
    auto* actor=mods::arg<e_ms_class*>(args,0);
    if (!actor || !in_arena() || s_run.phase!=Phase::Running || actor->health<=0 ||
        actor->mAction!=4 || actor->mMode>1) return HOOK_CONTINUE;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if (enemy && enemy->profile==fpcNm_E_MS_e && enemy->initialized && !enemy->retiring && !enemy->credited)
        s_ratRecoveries.push_back({args,actor,actor->mCooldown1});
    return HOOK_CONTINUE;
}
void after_rat_recovery(ModContext*, void* args, void*, void*) {
    if (s_ratRecoveries.empty() || s_ratRecoveries.back().args!=args) return;
    const auto state=s_ratRecoveries.back();s_ratRecoveries.pop_back();
    // Preserve the real hit cooldown as it counts down; e_ms_damage otherwise
    // writes 6 every frame. Native landing, lethal and water-death states remain.
    if (state.actor->health>0 && state.actor->mAction==4 && state.actor->mMode<=1)
        state.actor->mCooldown1=state.cooldown;
}

// Native hit processing still owns damage, hit particles/sounds and lethal
// reactions. Restore only nonlethal reaction state before native action dispatch.
struct HitState {
    void* args = nullptr; fopAc_ac_c* actor = nullptr;
    ProfileName profile{}; bool suppress = false;
    int action = 0, mode = 0, oldAction = 0;
    s16 timer0 = 0, timer1 = 0, direction = 0;
    float knockback = 0;
    std::array<bool, 2> target{}, collide{};
};
std::vector<HitState> s_hitStates;
bool causes_stagger(const cCcD_Obj* collider) {
    if(effect_staggers(collider)) return true;
    auto* link = daAlink_getAlinkActorClass();
    if (link && collider == &link->mGuardAtCps) return true;
    for (const auto& attack : s_attacks)
        if (collider == &attack.collider) return attack.weapon == unsigned(Upgrade::Ball) || attack.weapon == unsigned(Upgrade::ShieldAttack);
    return false;
}
// One hit per enemy per leg. A fresh return hit bypasses native recovery from
// the outbound hit, so enemies at the turnaround also take return damage.
int ball_contact(fopAc_ac_c* actor, const cCcD_Obj* collider) {
    for (auto& attack : s_attacks) {
        if (collider != &attack.collider || attack.weapon != unsigned(Upgrade::Ball)) continue;
        const auto id = fopAcM_GetID(actor);
        const auto end = attack.ballVictims.begin() + attack.ballVictimCount;
        if (std::find(attack.ballVictims.begin(), end, id) != end || attack.ballVictimCount == attack.ballVictims.size()) return 0;
        attack.ballVictims[attack.ballVictimCount++] = id;
        return 1;
    }
    return -1;
}
HookAction before_enemy_hit(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    if (!actor || !in_arena() || s_run.phase != Phase::Running) return HOOK_CONTINUE;
    auto* enemy = tracked(fopAcM_GetID(actor));
    if (!enemy || !enemy->initialized || enemy->retiring || enemy->credited) return HOOK_CONTINUE;
    prepare_effect_hit(actor, *enemy);
    HitState state; state.args = args; state.actor = actor; state.profile = enemy->profile;
    cCcD_Obj* attack = nullptr;
    if (state.profile == fpcNm_E_OC_e) {
        auto* e = static_cast<daE_OC_c*>(actor);
        state.action = e->mActionMode; state.mode = e->mOcState; state.oldAction = e->mOldActionMode;
        state.timer0 = e->field_0x6c0; state.timer1 = e->field_0x6c2;
        for (unsigned i = 0; i < 2; ++i) {
            state.target[i] = e->mSphs_cc[i].ChkTgSet(); state.collide[i] = e->mSphs_cc[i].ChkCoSet();
            if (e->mSphs_cc[i].ChkTgHit()) attack = e->mSphs_cc[i].GetTgHitObj();
        }
        const int ball = ball_contact(actor, attack);
        if (ball == 1) e->field_0x6cc = 0;
        if (ball == 0) for (auto& sphere : e->mSphs_cc) if (sphere.GetTgHitObj() == attack) sphere.ResetTgHit();
        if (e->field_0x6cc) attack = nullptr;
    } else if (state.profile == fpcNm_E_BA_e) {
        auto* e = reinterpret_cast<e_ba_class*>(actor);
        state.action = e->mAction; state.mode = e->mMode;
        state.knockback = e->mKnockbackSpeed; state.direction = e->mKnockbackAngle;
        if (e->mSph.ChkTgHit()) attack = e->mSph.GetTgHitObj();
        const int ball = ball_contact(actor, attack);
        if (ball == 1) e->mIFrames = 0;
        if (ball == 0) e->mSph.ResetTgHit();
        if (e->mIFrames) attack = nullptr;
    } else if (state.profile == fpcNm_E_MS_e) {
        auto* e = static_cast<e_ms_class*>(actor);
        state.action = e->mAction; state.mode = e->mMode; state.direction = e->current.angle.y;
        if (e->mSph1.ChkTgHit()) attack = e->mSph1.GetTgHitObj();
        const int ball = ball_contact(actor, attack);
        if (ball == 1) e->mCooldown1 = 0;
        if (ball == 0) e->mSph1.ResetTgHit();
        if (e->mCooldown1) attack = nullptr;
    }
    // All owned impacts use the bounded run knockback, not species-specific
    // native launches whose distance can dwarf the small heavy-weapon push.
    state.suppress = attack != nullptr;
    s_hitStates.push_back(state);
    return HOOK_CONTINUE;
}
void after_enemy_hit(ModContext*, void* args, void*, void*) {
    if (s_hitStates.empty() || s_hitStates.back().args != args) return;
    const auto state = s_hitStates.back(); s_hitStates.pop_back();
    finish_effect_hit(state.actor, state.profile);
    if (state.profile == fpcNm_E_BA_e && state.actor->health <= 0) {
        auto* bat=reinterpret_cast<e_ba_class*>(state.actor);
        // Native shield hits clear mIsDying even when our linear bash damage is
        // lethal. Enter its normal knockback/deletion path before AI resumes.
        if (!bat->mIsDying) { bat->mIsDying=true; bat->mKnockbackSpeed=80.f; }
    }
    if (!state.suppress || state.actor->health <= (state.profile == fpcNm_E_OC_e ? 1 : 0)) return;
    if (state.profile == fpcNm_E_OC_e) {
        auto* e = static_cast<daE_OC_c*>(state.actor);
        e->mActionMode = state.action; e->mOcState = state.mode; e->mOldActionMode = state.oldAction;
        e->field_0x6c0 = state.timer0; e->field_0x6c2 = state.timer1;
        for (unsigned i = 0; i < 2; ++i) {
            if (state.target[i]) e->mSphs_cc[i].OnTgSetBit(); else e->mSphs_cc[i].OffTgSetBit();
            if (state.collide[i]) e->mSphs_cc[i].OnCoSetBit(); else e->mSphs_cc[i].OffCoSetBit();
        }
    } else if (state.profile == fpcNm_E_BA_e) {
        auto* e = reinterpret_cast<e_ba_class*>(state.actor);
        if (e->mIsDying) return;
        e->mAction = state.action; e->mMode = state.mode;
        e->mKnockbackSpeed = state.knockback; e->mKnockbackAngle = state.direction;
    } else if (state.profile == fpcNm_E_MS_e) {
        auto* e = static_cast<e_ms_class*>(state.actor);
        e->mAction = state.action; e->mMode = state.mode; e->current.angle.y = state.direction;
    }
}

void after_light_sword(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (in_arena() && link && fopAcM_GetID(link) == s_player && s_run.master_sword >= 5)
        link->onNoResetFlg3(daPy_py_c::FLG3_UNK_100000);
}
struct HitMaterial { void* args; dCcD_GObjInf* collider; };
std::vector<HitMaterial> s_hitMaterials;
HookAction before_hit_material(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    auto* info = mods::arg<dCcU_AtInfo*>(args, 1);
    if (!in_arena() || !actor || !tracked(fopAcM_GetID(actor)) || !info || !info->mpCollider) return HOOK_CONTINUE;
    begin_damage_feedback(args,actor,info);
    auto* collider = static_cast<dCcD_GObjInf*>(info->mpCollider);
    auto* link = daAlink_getAlinkActorClass();
    bool owned = link && collider == &link->mAtSph;
    if (link) for (auto& capsule : link->mAtCps) owned = owned || collider == &capsule;
    if (!owned || collider->GetAtMtrl() != dCcD_MTRL_LIGHT) return HOOK_CONTINUE;
    s_hitMaterials.push_back({args, collider});
    collider->SetAtMtrl(dCcD_MTRL_NONE); // Native LIGHT forces 100 damage after at_power_check.
    return HOOK_CONTINUE;
}
void after_hit_material(ModContext*, void* args, void*, void*) {
    end_damage_feedback(args);
    record_status_hit(mods::arg<fopAc_ac_c*>(args, 0), mods::arg<dCcU_AtInfo*>(args, 1));
    if (s_hitMaterials.empty() || s_hitMaterials.back().args != args) return;
    s_hitMaterials.back().collider->SetAtMtrl(dCcD_MTRL_LIGHT);
    s_hitMaterials.pop_back();
}

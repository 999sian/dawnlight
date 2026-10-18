// Run-owned enemy health/model setup and absolute collider sizes.
void initialize_enemy(Enemy* enemy, fopAc_ac_c* actor) {
    if (!enemy || enemy->initialized) return;
    enemy->initialized = true;
    if (enemy->elite) {
        // These native matrix paths use HIO scale, ignoring actor scale.
        if (enemy->profile == fpcNm_E_OC_e) static_cast<daE_OC_c*>(actor)->getOcModel()->setBaseScale(actor->scale);
        else if (enemy->profile == fpcNm_E_MS_e) static_cast<e_ms_class*>(actor)->mpModelMorf->getModel()->setBaseScale(actor->scale);
        else if (enemy->profile == fpcNm_E_BA_e) reinterpret_cast<e_ba_class*>(actor)->mpMorf->getModel()->setBaseScale(actor->scale);
        // Bulblin's native matrix already includes actor->scale.
        actor->field_0x560 = actor->health = 120;
    } else if (enemy->profile == fpcNm_E_OC_e || enemy->profile == fpcNm_E_RD_e) {
        actor->field_0x560 = actor->health = static_cast<s16>(8 + std::min(24u, s_run.ticks / 900));
    }
    actor->field_0x560 = actor->health = static_cast<s16>(
        std::clamp(int(actor->health) * int(s_run.options.enemy_hp), 1, 32767));
}
void scale_elite_colliders(const Enemy* enemy, fopAc_ac_c* actor) {
    if (enemy && actor && enemy->elite && enemy->profile == fpcNm_E_OC_e) {
        // cc_set follows the scaled joints but writes fixed radii each Execute.
        // Assign absolute values so paused callbacks cannot compound the scale.
        auto* bokoblin = static_cast<daE_OC_c*>(actor);
        bokoblin->mSphs_cc[0].SetR(50.0f * actor->scale.x);
        bokoblin->mSphs_cc[1].SetR(45.0f * actor->scale.x);
        bokoblin->mSphs_at[0].SetR(45.0f * actor->scale.x);
        bokoblin->mSphs_at[1].SetR(30.0f * actor->scale.x);
    }
    if (enemy && actor && enemy->elite) {
        if (enemy->profile == fpcNm_E_MS_e) {
            auto* rat=static_cast<e_ms_class*>(actor);
            rat->mSph1.SetR(35.f*actor->scale.x); rat->mSph2.SetR(20.f*actor->scale.x);
        } else if (enemy->profile == fpcNm_E_BA_e) {
            reinterpret_cast<e_ba_class*>(actor)->mSph.SetR(30.f*actor->scale.x);
        } else if (enemy->profile == fpcNm_E_RD_e) {
            auto* bulblin=reinterpret_cast<e_rd_class*>(actor);
            for (auto& sphere:bulblin->cc_sph) sphere.SetR(35.f*actor->scale.x);
        }
    }
}

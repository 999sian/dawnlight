// Run-local burns and secondary melee hits enter the owned damage path:
// native handlers for the original roster, the shared adapter for new species.
struct EnemyEffect {
    ActorId id = kNone;
    unsigned burn = 0, pulse = ticks_per_second, pending = 0, meleeSerial = 0;
    unsigned damage = 0;
    bool injected = false, pendingCritical = false, critical = false;
    cXyz velocity{};
    std::array<u32, 2> emitters{};
    dCcD_Stts status;
    dCcD_Sph collider;
};
std::array<EnemyEffect, enemy_capacity> s_effects;
unsigned s_meleeSerial = 1, s_spreadSerial = 0;
void stop_burn(EnemyEffect& effect) {
    for (auto& key : effect.emitters) {
        if (auto* emitter = key ? dComIfGp_particle_getEmitter(key) : nullptr) {
            emitter->stopCreateParticle(); emitter->quitImmortalEmitter();
            emitter->becomeInvalidEmitter();
        }
        key = 0;
    }
    effect.burn = 0; effect.pulse = ticks_per_second;
}
void clear_enemy_effects() {
    for (auto& effect : s_effects) {
        stop_burn(effect); effect.id = kNone; effect.pending = effect.damage = effect.meleeSerial = 0;
        effect.injected = false; effect.pendingCritical = effect.critical = false;
    }
}
bool effect_collider(const cCcD_Obj* collider) {
    for (const auto& effect : s_effects) if (collider == &effect.collider) return true;
    return false;
}
EnemyEffect& effect_for(Enemy& enemy) {
    auto& effect = s_effects[unsigned(&enemy - s_enemies.data())];
    if (effect.id != enemy.id) {
        stop_burn(effect); effect.id = enemy.id;
        effect.pending = effect.damage = effect.meleeSerial = 0; effect.injected = false; effect.pendingCritical = effect.critical = false;
    }
    return effect;
}
std::array<dCcD_Sph*, 3> enemy_spheres(fopAc_ac_c* actor, ProfileName profile) {
    if (profile == fpcNm_E_OC_e) {
        auto* e = static_cast<daE_OC_c*>(actor); return {&e->mSphs_cc[0], &e->mSphs_cc[1]};
    }
    if (profile == fpcNm_E_BA_e) return {&reinterpret_cast<e_ba_class*>(actor)->mSph, nullptr};
    if (profile == fpcNm_E_MS_e) return {&static_cast<e_ms_class*>(actor)->mSph1, nullptr};
    if (profile == fpcNm_E_RD_e) {
        auto* e=reinterpret_cast<e_rd_class*>(actor); return {&e->cc_sph[0], &e->cc_sph[1], &e->cc_sph[2]};
    }
    return {};
}
void prepare_effect_hit(fopAc_ac_c* actor, Enemy& enemy) {
    auto& effect = effect_for(enemy);
    const auto spheres = enemy_spheres(actor, enemy.profile);
    if (!spheres[0] || actor->health <= 0) return;
    bool natural = false;
    for (auto* sphere : spheres) if (sphere && sphere->ChkTgHit()) {
        if (s_run.dual_wield && effect.meleeSerial == s_meleeSerial &&
            s_swordAction == SwordAction::Normal && native_sword_collider(sphere->GetTgHitObj()))
            sphere->ResetTgHit(); // A queued/accepted multihit cannot also hit via another capsule.
        else natural = true;
    }
    if (!effect.pending || natural) return; // Preserve real hits; queued damage waits its turn.
    auto* link = daAlink_getAlinkActorClass();
    if (!link || fopAcM_GetID(link) != s_player) return;
    effect.damage = effect.pending; effect.pending = 0; effect.injected = true;
    effect.critical=effect.pendingCritical; effect.pendingCritical=false;
    effect.status.Init(120, 0xff, link);
    effect.collider.Set(kAttackSource); effect.collider.SetStts(&effect.status);
    effect.collider.SetAtType(AT_TYPE_ARROW); // No native sword/ball/bomb special-case damage.
    effect.collider.SetAtAtp(1); effect.collider.SetAtMtrl(dCcD_MTRL_NONE);
    auto point = actor->eyePos;
    spheres[0]->SetTgHit(&effect.collider); spheres[0]->SetTgHitApid(s_player); spheres[0]->SetTgHitPos(point);
    if (enemy.profile == fpcNm_E_OC_e) static_cast<daE_OC_c*>(actor)->field_0x6cc = 0;
    else if (enemy.profile == fpcNm_E_BA_e) reinterpret_cast<e_ba_class*>(actor)->mIFrames = 0;
    else if (enemy.profile == fpcNm_E_MS_e) static_cast<e_ms_class*>(actor)->mCooldown1 = 0;
    else if (enemy.profile == fpcNm_E_RD_e) reinterpret_cast<e_rd_class*>(actor)->damage_timer = 0;
}
void finish_effect_hit(fopAc_ac_c* actor, ProfileName profile) {
    auto* enemy = tracked(fopAcM_GetID(actor)); if (!enemy) return;
    auto& effect = effect_for(*enemy); if (!effect.injected) return;
    for (auto* sphere : enemy_spheres(actor, profile))
        if (sphere && sphere->GetTgHitObj() == &effect.collider) sphere->ResetTgHit();
    dCcU_AtInfo* info = profile == fpcNm_E_OC_e ? &static_cast<daE_OC_c*>(actor)->mAtInfo :
        profile == fpcNm_E_BA_e ? &reinterpret_cast<e_ba_class*>(actor)->mAtInfo :
        profile == fpcNm_E_RD_e ? &reinterpret_cast<e_rd_class*>(actor)->at_info : &static_cast<e_ms_class*>(actor)->mAtInfo;
    if (info->mpCollider == &effect.collider) info->mpCollider = nullptr;
    effect.injected = false; effect.critical = false; effect.damage = 0;
}
void spread_melee_hit(fopAc_ac_c* primary) {
    if (!s_run.dual_wield || s_spreadSerial == s_meleeSerial) return;
    s_spreadSerial = s_meleeSerial;
    if (auto* e = tracked(fopAcM_GetID(primary))) effect_for(*e).meleeSerial = s_meleeSerial;
    auto* link = daAlink_getAlinkActorClass(); if (!link) return;
    for (unsigned n = 0; n + 1 < s_run.melee_targets(); ++n) {
        Enemy* best = nullptr; float distance = s_run.radius(280) * s_run.radius(280);
        const cXyz from = link->current.pos + cXyz(0, 70, 0);
        for (auto& e : s_enemies) {
            if (!e.initialized || e.retiring || e.credited || effect_for(e).meleeSerial == s_meleeSerial) continue;
            auto* actor = actor_by_id(e.id); if (!actor || actor->health <= 0 || actor == primary) continue;
            const auto delta = actor->current.pos - link->current.pos;
            const float d = delta.x * delta.x + delta.z * delta.z;
            if (d >= distance || std::abs(delta.y) > 180) continue;
            cXyz to = actor->eyePos; dBgS_LinChk wall; wall.Set(&from, &to, link);
            if (dComIfG_Bgsp().LineCross(&wall)) continue;
            best = &e; distance = d;
        }
        if (!best) break;
        auto& effect = effect_for(*best); effect.meleeSerial = s_meleeSerial;
        // Roll once per secondary target before combining with non-critical burn ticks.
        bool critical=false;
        effect.pending += s_run.critical_power(s_run.melee_power(),&critical);
        effect.pendingCritical = effect.pendingCritical || critical;
    }
}
void record_status_hit(fopAc_ac_c* actor, dCcU_AtInfo* info) {
    if (!in_arena() || s_run.phase != Phase::Running || !actor || !info || !info->mAttackPower) return;
    auto* enemy = tracked(fopAcM_GetID(actor));
    if (!enemy || !enemy->initialized || enemy->retiring || enemy->credited) return;
    // Collisions from enemies/the environment do not provoke a waiting guard.
    if (info->mpCollider && info->mpCollider->GetAc() == daAlink_getAlinkActorClass())
        enemy->dormant = false;
    if (s_run.burn_damage() && actor->health > 0) for (const auto& arrow : s_attacks) {
        if (info->mpCollider == &arrow.collider && arrow.weapon == unsigned(Upgrade::Bow)) {
            auto& effect = effect_for(*enemy);
            if (!effect.burn) effect.pulse = ticks_per_second;
            effect.burn = s_run.burn_duration(); // Refresh duration, not the damage clock.
            break;
        }
    }
    if (s_swordAction == SwordAction::Normal && native_sword_collider(info->mpCollider)) spread_melee_hit(actor);
}
void update_enemy_effects() {
    for (unsigned i = 0; i < s_enemies.size(); ++i) {
        auto& enemy = s_enemies[i]; auto& effect = s_effects[i];
        auto* actor = actor_by_id(enemy.id);
        if (!in_arena() || !enemy.initialized || enemy.retiring || enemy.credited || !actor ||
            actor->health <= 0 || effect.id != enemy.id) {
            stop_burn(effect); effect.pending = 0; effect.pendingCritical=effect.critical=false; continue;
        }
        if (!effect.burn) continue;
        if (!--effect.pulse) { effect.pending += s_run.burn_damage(); effect.pulse = ticks_per_second; }
        if (!--effect.burn) { stop_burn(effect); continue; }
        // Same native BURNS_A/B, trace callback, anchor fallback and scale as
        // baxlek/Fire-Arrows-Standalone. No external texture/model assets.
        effect.velocity = actor->speed;
        cXyz point = actor->eyePos.abs(actor->current.pos) <= 300 ? actor->eyePos : actor->current.pos + cXyz(0, 40, 0);
        constexpr std::array<u16, 2> particles{ID_ZI_J_LK_BURNS_A, ID_ZI_J_LK_BURNS_B};
        for (unsigned j = 0; j < particles.size(); ++j) {
            auto& key = effect.emitters[j];
            auto* old = key ? dComIfGp_particle_getEmitter(key) : nullptr;
            if (!old || old->isEnableDeleteEmitter()) key = 0;
            key = dComIfGp_particle_set(key, particles[j], &point, nullptr, nullptr);
            if (auto* emitter = dComIfGp_particle_getEmitter(key)) {
                emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                emitter->setUserWork(reinterpret_cast<uintptr_t>(&effect.velocity));
                emitter->setGlobalScale(cXyz(3, 3, 3));
            }
        }
    }
}

// Capture every accepted geometry pair before the native single-hit pointer or
// actor-wide no-consecutive-hit filter can discard simultaneous attacks. The
// existing owned damage adapter consumes the sum once on the next enemy update.
// Deduplication belongs to an individual projectile/swing, never to the enemy.
HookAction before_concurrent_hit(ModContext*,void* args,void*,void*) {
    if(!in_arena() || s_run.phase!=Phase::Running) return HOOK_CONTINUE;
    auto* attack=mods::arg<cCcD_Obj*>(args,1);
    auto* target=mods::arg<cCcD_Obj*>(args,2);
    if(!attack || !target) return HOOK_CONTINUE;
    auto* actor=target->GetAc();
    if(actor && fopAcM_GetName(actor)==fpcNm_E_MM_MT_e) actor=actor_by_id(actor->parentActorID);
    auto* enemy=actor?tracked(fopAcM_GetID(actor)):nullptr;
    if(!enemy || !enemy->initialized || enemy->retiring || enemy->credited || actor->health<=0) return HOOK_CONTINUE;
    auto* link=daAlink_getAlinkActorClass();
    if(!link || fopAcM_GetID(link)!=s_player) return HOOK_CONTINUE;
    if(block_boss_contact(*enemy,actor,attack,target))return HOOK_SKIP_ORIGINAL;
    auto& effect=effect_for(*enemy);
    DamageSource source=DamageSource::Other;
    unsigned base=0;bool owned=false,duplicate=false,stagger=false;
    for(auto& projectile:s_attacks) if(attack==&projectile.collider && projectile.active) {
        source=weapon_damage_source(projectile.weapon);
        owned=true;base=projectile.collider.GetAtAtp();
        const auto end=projectile.ballVictims.begin()+projectile.ballVictimCount;
        duplicate=std::find(projectile.ballVictims.begin(),end,enemy->id)!=end ||
            projectile.ballVictimCount==projectile.ballVictims.size() ||
            (projectile.weapon==unsigned(Upgrade::Bow) && projectile.ballVictimCount>=projectile.arrowTargets);
        if(!duplicate) {
            projectile.ballVictims[projectile.ballVictimCount++]=enemy->id;
            if(projectile.weapon==unsigned(Upgrade::Bow) && projectile.bowVolley) {
                if(effect.bowVolleys.size()<=projectile.bowVolleySlot) effect.bowVolleys.resize(size_t(projectile.bowVolleySlot)+1);
                auto& last=effect.bowVolleys[projectile.bowVolleySlot];
                if(last==projectile.bowVolley) base=std::max(1u,(base+2)/4);
                last=projectile.bowVolley;
            }
        }
        stagger=projectile.weapon==unsigned(Upgrade::Ball) || projectile.weapon==unsigned(Upgrade::ShieldAttack);
        break;
    }
    if(!owned && native_sword_collider(attack) &&
       (s_swordAction==SwordAction::Normal || s_swordAction==SwordAction::Jump || s_swordAction==SwordAction::Spin)) {
        source=s_swordAction==SwordAction::Jump?DamageSource::Jump:
            s_swordAction==SwordAction::Spin?DamageSource::Spin:DamageSource::Sword;
        owned=true;
        const bool splash=s_swordAction==SwordAction::Jump && attack==&link->mAtSph;
        auto& serial=splash?effect.jumpSerial:effect.meleeSerial;
        duplicate=serial==s_meleeSerial;serial=s_meleeSerial;
        base=splash?s_run.jump_splash_power():s_swordAction==SwordAction::Spin?s_run.attack_power(unsigned(Upgrade::Spin)):s_run.melee_power();
    }
    if(!owned && attack==&link->mGuardAtCps) {
        owned=true;source=DamageSource::Shield;stagger=true;
        duplicate=effect.meleeSerial==s_meleeSerial;effect.meleeSerial=s_meleeSerial;
        base=s_run.attack_power(unsigned(Upgrade::ShieldAttack));
    }
    if(!owned) return HOOK_CONTINUE;
    if(!duplicate) {
        bool critical=false;
        queue_effect_damage(effect,source,s_run.critical_power(base,&critical));
        effect.pendingCritical=effect.pendingCritical||critical;
        effect.pendingStagger=effect.pendingStagger||stagger;
        attack->SetAtHit(target); // Arrow stop, boomerang hop and native combo contact.
        // dCcD_GObjInf::ChkAtHit also requires the native target actor ID.
        // SetAtHit alone only fills the base collision object; our hook skips
        // SetAtTgGObjInf, which normally supplies this second half of the hit.
        // Keep the actual collider owner here (including Helmasaur armor).
        static_cast<dCcD_GObjInf*>(attack->GetGObjInf())->SetAtHitApid(fopAcM_GetID(target->GetAc()));
        dCcU_AtInfo info{};info.mpCollider=attack;info.mAttackPower=base;
        record_status_hit(actor,&info); // Burn refresh / distinct Dual Wield targets.
    }
    return HOOK_SKIP_ORIGINAL;
}

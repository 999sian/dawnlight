// Own an actor ID, never a pointer or the camera's native attention target.
// Keep a reachable enemy for the swing so orbiting does not switch targets.
fopAc_ac_c* melee_target(daAlink_c* link) {
    fopAc_ac_c* nearest = nullptr;
    float best = s_run.radius(560) * s_run.radius(560);
    const float retainedRange = s_run.radius(660);
    const cXyz origin = link->current.pos + cXyz(0, 70, 0);
    for (const auto& enemy : s_enemies) {
        if (!enemy.initialized || enemy.retiring || enemy.credited) continue;
        auto* actor = actor_by_id(enemy.id);
        if (!actor || actor->health <= 0) continue;
        const cXyz delta = actor->current.pos - link->current.pos;
        const float distance = delta.x * delta.x + delta.z * delta.z;
        if (std::abs(delta.y) > 180 || distance < 1 || distance > retainedRange * retainedRange) continue;
        cXyz point = actor->eyePos;
        dBgS_LinChk wall; wall.Set(&origin, &point, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        if (enemy.id == s_meleeTarget) return actor;
        if (distance < best) { best = distance; nearest = actor; }
    }
    // Enemies retain priority; otherwise normal sword swings can break our pots.
    if (!nearest) for (const auto& pot:s_pots) {
        if (!pot.initialized || pot.retiring || pot.broken) continue;
        auto* actor=actor_by_id(pot.id);
        if (!actor || fopAcM_checkCarryNow(actor) || fopAcM_checkHookCarryNow(actor)) continue;
        const cXyz delta=actor->current.pos-link->current.pos;
        const float distance=delta.x*delta.x+delta.z*delta.z;
        if (std::abs(delta.y)>80 || distance<1 || distance>best) continue;
        cXyz point=actor->current.pos+cXyz(0,30,0);
        dBgS_LinChk wall;wall.Set(&origin,&point,link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        nearest=actor;best=distance;
    }
    s_meleeTarget = nearest ? fopAcM_GetID(nearest) : kNone;
    return nearest;
}
void track_melee_facing(daAlink_c* link) {
    if (!s_trackMelee || s_swordAction != SwordAction::Normal || !normal_sword_proc(link) ||
        !link->mLinkAcch.ChkGroundHit() || !playable() || s_run.phase != Phase::Running ||
        fopAcM_GetID(link) != s_player) return;
    if(manual_attack(link)||s_manualSwing) {
        const auto delta=manual_direction(link);
        link->shape_angle.y=cM_atan2s(delta.x,delta.z);
        if(link->mProcID==daAlink_c::PROC_CUT_NORMAL)link->mProcVar2.field_0x300c=link->shape_angle.y;
        return;
    }
    if (auto* target = melee_target(link)) {
        const auto delta = target->current.pos - link->current.pos;
        link->shape_angle.y = cM_atan2s(delta.x, delta.z);
        // Native normal cuts steer back to this saved heading; finishers use
        // the same union member as a frame threshold, so leave those intact.
        if (link->mProcID == daAlink_c::PROC_CUT_NORMAL)
            link->mProcVar2.field_0x300c = link->shape_angle.y;
    }
    // current.angle / mMoveAngle remain the player's independent travel axis.
}

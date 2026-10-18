// Correct facing before native attack/root motion and again after floor
// correction, before this frame's model matrices and weapon colliders are built.
void track_enemy_facing(fopAc_ac_c* actor) {
    if (!actor || !playable() || s_run.phase!=Phase::Running || actor->health<=0) return;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    auto* link=daAlink_getAlinkActorClass();
    if (!enemy || !enemy->initialized || enemy->retiring || enemy->credited || enemy->dormant ||
        !link || fopAcM_GetID(link)!=s_player) return;
    bool combat=false;
    if (enemy->profile==fpcNm_E_OC_e) {
        const auto* e=static_cast<daE_OC_c*>(actor);
        combat=e->mActionMode==3 || e->mActionMode==4;
    } else if (enemy->profile==fpcNm_E_MS_e) {
        combat=static_cast<e_ms_class*>(actor)->mAction==1;
    } else if (enemy->profile==fpcNm_E_BA_e) {
        const auto* e=reinterpret_cast<e_ba_class*>(actor);
        combat=!e->mIsDying && e->mKnockbackSpeed<=0.1f &&
            (e->mAction==e_ba_class::ACT_FIGHT_FLY || e->mAction==e_ba_class::ACT_FIGHT || e->mAction==e_ba_class::ACT_ATTACK);
    } else if (enemy->profile==fpcNm_E_RD_e) {
        const auto* e=reinterpret_cast<e_rd_class*>(actor);
        combat=!e->actor_set && !e->ride_mode && e->field_0x9f0<=0.1f &&
            (e->action==3 || e->action==4 || e->action==5 || e->action==6);
    }
    const cXyz delta=link->current.pos-actor->current.pos;
    const float range=(enemy->profile==fpcNm_E_MS_e ? 300.f : 400.f)*std::max(1.f,actor->scale.x);
    if (!combat || delta.absXZ()>range || delta.absXZ()<1 || std::abs(delta.y)>250*std::max(1.f,actor->scale.y)) return;
    const s16 yaw=cM_atan2s(delta.x,delta.z);
    actor->current.angle.y=actor->shape_angle.y=yaw;
    if (enemy->profile==fpcNm_E_MS_e) {
        auto* e=static_cast<e_ms_class*>(actor);
        e->field_0x5c8=e->mPlayerAngleY=yaw; // Native leap otherwise keeps its windup heading.
    } else if (enemy->profile==fpcNm_E_BA_e) {
        auto* e=reinterpret_cast<e_ba_class*>(actor);
        e->mPlayerAngleY=yaw;
        if (e->mAction==e_ba_class::ACT_ATTACK) e->mTargetPos=link->current.pos+cXyz(0,120,0);
    } else if (enemy->profile==fpcNm_E_RD_e) {
        reinterpret_cast<e_rd_class*>(actor)->angleY=yaw;
    }
}
void after_enemy_facing(ModContext*, void* args, void*, void*) {
    auto* acch=mods::arg<dBgS_Acch*>(args,0);
    if (acch) track_enemy_facing(acch->getMyAc());
}

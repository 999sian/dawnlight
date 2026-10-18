// Run-local difficulty; native collision, health reduction and death paths remain in charge.
HookAction before_enemy_speed(ModContext*, void* args, void*, void*) {
    auto* acch=mods::arg<dBgS_Acch*>(args,0);
    auto* actor=acch ? acch->getMyAc() : nullptr;
    if (!actor || !playable() || s_run.phase!=Phase::Running || s_run.options.enemy_speed==1) return HOOK_CONTINUE;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if (!enemy || !enemy->initialized || enemy->retiring || enemy->credited || enemy->dormant || actor->health<=0)
        return HOOK_CONTINUE;
    // Whitelist locomotion, not a numeric range which also contains attacks.
    // Bokoblin attack root motion is separate from speed and must stay native,
    // including the frame transitioning from attack back to the find state.
    if (enemy->profile==fpcNm_E_OC_e) {
        auto* bok=static_cast<daE_OC_c*>(actor);
        const bool walk=bok->mActionMode==1 && bok->mOcState==1 && bok->checkBck(0x1d);
        const bool chase=bok->mActionMode==3 && bok->mOcState==4 &&
            (bok->checkBck(0x1e) || bok->checkBck(0x0b));
        if (!walk && !chase) return HOOK_CONTINUE;
    } else if (enemy->profile==fpcNm_E_MS_e) {
        auto* rat=static_cast<e_ms_class*>(actor);
        const bool walk=rat->mAction==0 && (rat->mMode==1 || rat->mMode==6);
        const bool chase=rat->mAction==1 && rat->mMode==2;
        if (!walk && !chase) return HOOK_CONTINUE; // Windup, leap and recovery stay native.
    } else if (enemy->profile==fpcNm_E_BA_e) {
        auto* bat=reinterpret_cast<e_ba_class*>(actor);
        const bool fly=bat->mAction==e_ba_class::ACT_FIGHT_FLY || bat->mAction==e_ba_class::ACT_RETURN ||
            bat->mAction==e_ba_class::ACT_FLY || bat->mAction==e_ba_class::ACT_PATH_FLY;
        if (!fly || bat->mIsDying || bat->mKnockbackSpeed>0.1f) return HOOK_CONTINUE;
    } else return HOOK_CONTINUE;
    const float extra=float(std::clamp(s_run.options.enemy_speed,1u,3u)-1);
    // Add only horizontal locomotion velocity before native wall correction.
    // Do not multiply animation translations, collision pushes, falls or teleports.
    actor->current.pos.x+=actor->speed.x*extra;
    actor->current.pos.z+=actor->speed.z*extra;
    acch->OnLineCheck();
    return HOOK_CONTINUE;
}
HookAction before_enemy_damage(ModContext*, void* args, void*, void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!link || fopAcM_GetID(link)!=s_player || !playable() || s_run.phase!=Phase::Running ||
        mods::arg<int>(args,1)<=0 || mods::arg<int>(args,4)!=0) return HOOK_CONTINUE;
    for (auto& target:link->mTgCyls) {
        auto* attacker=target.ChkTgHit() ? target.GetTgHitAc() : nullptr;
        const auto* enemy=attacker ? tracked(fopAcM_GetID(attacker)) : nullptr;
        if (!enemy || !enemy->initialized || enemy->retiring) continue;
        mods::arg_ref<int>(args,1)=std::min(mods::arg<int>(args,1),32767)*int(s_run.options.enemy_damage);
        break;
    }
    return HOOK_CONTINUE;
}
// Scale only the final rendered camera. Restore its native input before the next
// Run, so zoom cannot compound or feed back into native orbit smoothing.
dCamera_c* s_zoomCamera=nullptr;
cXyz s_nativeEye{}, s_zoomEye{};
HookAction before_run_camera(ModContext*, void* args, void*, void*) {
    auto* camera=mods::arg<dCamera_c*>(args,0);
    if (camera && camera==s_zoomCamera && camera->mEye.abs2(s_zoomEye)<0.001f) {
        camera->mEye=s_nativeEye;
        camera->mDirection.Val(camera->mEye-camera->mCenter);
    }
    s_zoomCamera=nullptr;
    return HOOK_CONTINUE;
}
void after_run_camera(ModContext*, void* args, void*, void*) {
    auto* camera=mods::arg<dCamera_c*>(args,0);
    if (!camera || !in_arena() || transitioning() || s_run.phase==Phase::Loading ||
        dComIfGp_event_runCheck() || dCamera_c::isAimActive() || camera->mPadID!=0) return;
    const unsigned percent=s_run.phase==Phase::Ready ? s_options.camera_distance : s_run.options.camera_distance;
    if (percent==100) return;
    const cXyz delta=camera->mEye-camera->mCenter;
    if (delta.abs2()<1) return;
    cXyz eye=camera->mCenter+delta*(float(percent)/100.f);
    dBgS_CamLinChk wall;
    if (camera->lineBGCheck(&camera->mCenter,&eye,&wall,0x40b7))
        eye=wall.GetCross()-delta*(5.f/delta.abs());
    s_nativeEye=camera->mEye; s_zoomEye=eye; s_zoomCamera=camera;
    camera->mEye=eye; camera->mDirection.Val(eye-camera->mCenter);
}

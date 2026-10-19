// Read the mapped sword input: controller, keyboard and touch share this path.
// No cooldown reset or queued input survives a pause/scene change.
bool manual_attack(daAlink_c* link) {
    return link && fopAcM_GetID(link)==s_player && playable() &&
        s_run.phase==Phase::Running && (link->swordButton() || link->swordTrigger());
}
cXyz manual_direction(daAlink_c* link) {
    if(auto* camera=dComIfGp_getCamera(0)) {
        const cXyz delta=camera->mCamera.Center()-camera->mCamera.Eye();
        const float length=delta.abs();
        if(std::isfinite(length)&&length>0.001f)return delta*(1.f/length);
    }
    return cXyz(cM_ssin(link->shape_angle.y),0,cM_scos(link->shape_angle.y));
}
cXyz manual_target(daAlink_c* link,float range) {
    return link->current.pos+cXyz(0,70,0)+manual_direction(link)*range;
}
HookAction before_manual_charge(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link||!in_arena()||fopAcM_GetID(link)!=s_player)return HOOK_CONTINUE;
    link->mProcVar5.mCutTurnChargeCheckTimer=0;
    return HOOK_SKIP_ORIGINAL;
}

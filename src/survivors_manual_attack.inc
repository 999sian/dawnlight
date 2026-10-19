// Read the mapped sword input: controller, keyboard and touch share this path.
// No cooldown reset or queued input survives a pause/scene change.
bool manual_attack(daAlink_c* link) {
    return link && fopAcM_GetID(link)==s_player && playable() &&
        s_run.phase==Phase::Running && (link->swordButton() || link->swordTrigger());
}
bool manual_skill(daAlink_c* link,Upgrade weapon) {
    return s_run.options.manual_enabled(weapon)&&manual_attack(link);
}
bool manual_sword(daAlink_c* link) {
    return s_run.options.manual_enabled(ManualAbility::Sword)&&manual_attack(link);
}
#include "survivors_sword_aim.inc"
cXyz manual_direction(daAlink_c* link) {
    cXyz target,forward;
    if(sword_aim_target(link,target,forward)) {
        const cXyz delta=target-link->current.pos-cXyz(0,65,0);
        const float length=delta.abs();
        if(std::isfinite(length)&&length>.001f&&delta.inprod(forward)>0)return delta*(1.f/length);
        return forward;
    }
    return cXyz(cM_ssin(link->shape_angle.y),0,cM_scos(link->shape_angle.y));
}
HookAction before_manual_charge(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link||!in_arena()||fopAcM_GetID(link)!=s_player)return HOOK_CONTINUE;
    link->mProcVar5.mCutTurnChargeCheckTimer=0;
    return HOOK_SKIP_ORIGINAL;
}

// Correct running and automatic combat at the start of native posMove, after
// action handlers have applied their slowdown/lunge. Native gravity, collision
// and floor resolution still run. Damage, jumps and events retain native control.
bool auto_movement(daAlink_c* link) {
    if (!link || fopAcM_GetID(link) != s_player || !playable() ||
        s_run.phase != Phase::Running || !link->mLinkAcch.ChkGroundHit()) return false;
    // checkCutCancelNextMode clears speed and enters locomotion before posMove.
    // The sword scheduler then clears ownership while the old attack still
    // blends out. Cover locomotion too, without relying on a recovery timer.
    return sword_action_available(link) ||
        (s_swordAction == SwordAction::Spin && link->mProcID == daAlink_c::PROC_CUT_TURN) ||
        (s_swordAction == SwordAction::Normal && normal_sword_proc(link)) ||
        (s_run.owns(ChestReward::Shield) && shield_pose(link));
}
struct MovementSample {
    daAlink_c* link = nullptr;
    cXyz position, anchor;
} s_movement;
daAlink_c* s_guardMovement = nullptr;
float s_guardSpeed = 0;
HookAction before_auto_movement(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    s_movement.link = s_guardMovement = nullptr;
    if (!auto_movement(link)) return HOOK_CONTINUE;
    s_movement = {link, link->current.pos, link->field_0x3798};
    if (shield_pose(link)) { s_guardMovement = link; s_guardSpeed = link->mNormalSpeed; }
    link->current.angle.y = link->mMoveAngle;
    link->mMaxSpeed = link->mpHIO->mMove.m.mMaxSpeed;
    // setStickData scales mStickValue for the iron-ball heavy state. mMoveValue
    // retains the player's input before that penalty; keep the native deadzone.
    link->mNormalSpeed = link->mMoveValue > 0.05f ? link->mMaxSpeed * link->mMoveValue : 0;
    link->mSpeedModifier = 0; // Do not derive travel speed from attacking feet.
    return HOOK_CONTINUE;
}
void after_auto_movement(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || s_movement.link != link) return;
    auto* previousRoot = mods::arg<cXyz*>(args, 1);
    // Preserve animation bookkeeping and vertical translation, but remove both
    // direct XZ root motion and the delta consumed later by posMove.
    link->current.pos.x = s_movement.position.x;
    link->current.pos.z = s_movement.position.z;
    link->field_0x3798.x = s_movement.anchor.x;
    link->field_0x3798.z = s_movement.anchor.z;
    previousRoot->x = link->field_0x34d4.x;
    previousRoot->z = link->field_0x34d4.z;
    s_movement.link = nullptr;
}
void after_auto_position(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    // posMove has finished: aim from the new position before setMatrix builds
    // this frame's body/sword and collision capsules. Never redirect movement.
    if (link) track_melee_facing(link);
    if (link && link == s_guardMovement) {
        // GuardSlip finishes once its native recoil speed reaches zero. Keep
        // that timer independent from the player's physical travel speed.
        link->mNormalSpeed = s_guardSpeed;
        s_guardMovement = nullptr;
    }
}

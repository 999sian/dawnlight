// Native ledge checks compare travel against body facing, then launch along the
// body axis. During Survivors combat those axes differ. Align only for that
// check; keep its native terrain, water, speed, heavy-gear and no-jump vetoes.
struct LedgeJumpScope {
    daAlink_c* link=nullptr;
    int proc=0;
    s16 facing=0,travel=0;
    float normalSpeed=0,stick=0;
};
std::vector<LedgeJumpScope> s_ledgeJumpScopes;
bool ledge_movement_action(daAlink_c* link) {
    // sword_action_available intentionally requires ground; the edge check runs
    // after ground contact was lost, including while a ranged upper pose plays.
    switch(link->mProcID) {
    case daAlink_c::PROC_WAIT: case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE: case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE: case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN: return true;
    default:return auto_movement_action(link);
    }
}
HookAction before_survivors_ledge_jump(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    s_ledgeJumpScopes.push_back({});
    if(!link || fopAcM_GetID(link)!=s_player || !playable() || s_run.phase!=Phase::Running ||
       link->checkWolf() || link->mLinkAcch.ChkGroundHit() || !ledge_movement_action(link) ||
       jump_abilities_busy_for(link) ||
       link->mMoveValue<=0.85f || link->speedF<=0)return HOOK_CONTINUE;
    s_ledgeJumpScopes.back()={link,int(link->mProcID),link->shape_angle.y,link->current.angle.y,link->mNormalSpeed,link->mStickValue};
    link->shape_angle.y=link->current.angle.y=link->mMoveAngle;
    link->mStickValue=link->mMoveValue;
    // Guard recoil has a separate native timer; actual travel speed was retained
    // in speedF by posMove even after we restored that timer for GuardSlip.
    link->mNormalSpeed=link->speedF;
    return HOOK_CONTINUE;
}
void after_survivors_ledge_jump(ModContext*,void*,void*,void*) {
    if(s_ledgeJumpScopes.empty())return;
    const auto scope=s_ledgeJumpScopes.back();s_ledgeJumpScopes.pop_back();
    if(!scope.link)return;
    auto* link=scope.link;
    link->mStickValue=scope.stick;
    // A successful jump/fall now owns its heading and speed. Otherwise restore
    // the attack pose and its recoil state without disturbing melee tracking.
    if(int(link->mProcID)==scope.proc) {
        link->shape_angle.y=scope.facing;link->current.angle.y=scope.travel;
        link->mNormalSpeed=scope.normalSpeed;
    }
}

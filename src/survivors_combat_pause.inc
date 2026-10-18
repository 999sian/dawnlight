bool auto_attacks_suspended(daAlink_c* link) {
    return (s_swordAction == SwordAction::Jump && jump_strike_pose(link)) || !link->mLinkAcch.ChkGroundHit() || link->checkModeFlg(daAlink_c::MODE_PLAYER_FLY) ||
        jump_abilities_busy_for(link) || link->mProcID==daAlink_c::PROC_SCREAM_WAIT ||
        link->checkEndResetFlg1(daAlink_c::ERFLG1_NS_SCREAM);
}
void suspend_auto_attacks(daAlink_c* link) {
    cancel_ranged(); // Only removes our own still-matching upper animation.
    if(s_swordAction!=SwordAction::None) {
        for(auto& collider:link->mAtCps){collider.OffAtSetBit();collider.ResetAtHit();}
        link->mAtSph.OffAtSetBit();link->mAtSph.ResetAtHit();
    }
    s_swordAction=SwordAction::None;s_wavePending=false;
    s_trackMelee=false;s_meleeTarget=kNone;s_comboStep=s_comboTimeout=0;
    // Do not call procWaitInit/resetUpperAnime here: jumps, Gale and glide own
    // the native body/upper pose, as does Gibdo paralysis. Released projectiles finish normally.
}

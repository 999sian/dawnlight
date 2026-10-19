// A wave has its own cooldown, but its release belongs to a visible normal
// sword swing. Other automatic weapons and pulses continue during the wind-up.
void update_sword_wave(daAlink_c* link) {
    if (!s_run.rank(Upgrade::Wave) || s_cooldowns[unsigned(Upgrade::Wave)]) { s_wavePending=false; return; }
    if (s_wavePending) {
        if (s_swordAction!=SwordAction::Normal || link->mProcID!=daAlink_c::PROC_CUT_NORMAL) {
            s_wavePending=false; // Interrupted casts never release from damage/jump poses.
        } else {
            if (link->mUnderFrameCtrl[0].getFrame()>=link->field_0x3478) {
                release_sword_wave(link);
                s_cooldowns[unsigned(Upgrade::Wave)]=s_run.cooldown(60);
                s_wavePending=false;
            }
            return;
        }
    }
    if (!manual_skill(link,Upgrade::Wave) && !enemy_in_range(link,1600)) return;
    if (s_swordAction==SwordAction::Normal && link->mProcID==daAlink_c::PROC_CUT_NORMAL) {
        if (link->mUnderFrameCtrl[0].getFrame()<link->field_0x347c) {s_wavePending=true;s_waveManual=manual_skill(link,Upgrade::Wave);}
        return; // Reuse an ongoing normal swing; never restart it or a finisher.
    }
    if (!sword_action_available(link)) return;
    if (!link->checkItemSwordEquip()) {
        if (link->mEquipItem==dItemNo_NONE_e && !link->checkEquipAnime()) link->swordEquip(0);
        return;
    }
    cancel_ranged();
    const auto direction=manual_skill(link,Upgrade::Wave)?manual_direction(link):target_direction(link);
    link->shape_angle.y=cM_atan2s(direction.x,direction.z);
    s_comboStep=0; // Ranged-only casting always starts with a normal cut.
    if (!begin_sword_combo(link)) return;
    s_swordAction=SwordAction::Normal;s_wavePending=true;s_waveManual=manual_skill(link,Upgrade::Wave);s_trackMelee=false;s_meleeTarget=kNone;
    new_melee_swing();
    link->mUnderFrameCtrl[0].setRate(link->mUnderFrameCtrl[0].getRate()*s_run.melee_speed());
    link->mUpperFrameCtrl[0].setRate(link->mUpperFrameCtrl[0].getRate()*s_run.melee_speed());
    s_meleeCooldown=s_run.melee_cooldown();
}

// Spin owns the native body animation; shield/projectile collisions remain independent.
float parallel_spin_base_radius(const daAlink_c* link) {
    const auto& spin = link->mpHIO->mCut.mCutTurn.m;
    return s_run.great_spin(Upgrade::Spin) ? spin.mLargeAttackRadius : spin.mAttackRadius;
}
void update_parallel_pulse(Attack& a, daAlink_c* link) {
    a.position = link->current.pos + cXyz(0,65,0);
    if (a.weapon == unsigned(Upgrade::ShieldAttack)) a.position += (a.manualFlight?manual_direction(link):target_direction(link)) * 100.f;
    for (auto key : a.emitters) if (auto* emitter=key?dComIfGp_particle_getEmitter(key):nullptr)
        emitter->setGlobalTranslation(a.position);
}
void launch_shockwave(daAlink_c* link) {
    auto& wave=acquire_projectile(s_attacks,unsigned(Upgrade::Shockwave),0,0);
    stop_attack(wave);wave={};wave.active=true;wave.weapon=unsigned(Upgrade::Shockwave);
    wave.life=30;wave.radius=s_run.shockwave_radius();wave.effectBaseRadius=260.f;
    wave.position=link->current.pos+cXyz(0,65,0);
    const bool manual=manual_skill(link,Upgrade::Shockwave);
    wave.velocity=(manual?manual_direction(link):target_direction(link))*45.f;wave.aimPending=manual;
    wave.status.Init(120,0xff,link);wave.collider.Set(kAttackSource);wave.collider.SetStts(&wave.status);
    wave.collider.SetAtType(AT_TYPE_NORMAL_SWORD);wave.collider.SetAtMtrl(dCcD_MTRL_NONE);
    wave.collider.SetAtAtp(static_cast<u8>(s_run.melee_power()));
    wave.collider.OnAtNoConHit();wave.collider.ResetAtHit();wave.collider.OnAtSetBit();
}
bool launch_parallel_pulse(daAlink_c* link, Upgrade weapon) {
    if (weapon==Upgrade::Spin) {
        if (!manual_skill(link,weapon) && !enemy_in_range(link,s_run.area_radius(parallel_spin_base_radius(link)))) return false;
        return begin_spin_attack(link); // Native animation, hit window and effects own this activation.
    }
    const float range=100+s_run.shield_radius();
    const bool manual=manual_skill(link,weapon);
    if ((!manual && !enemy_in_range(link,range)) || ranged_pose_active() || !ranged_pose_available(link) ||
        !link->mLinkAcch.ChkGroundHit() || link->checkEquipAnime())return false;
    const auto direction=manual?manual_direction(link):target_direction(link);
    link->shape_angle.y=cM_atan2s(direction.x,direction.z);
    if(!link->procGuardAttackInit())return false;
    s_swordAction=SwordAction::None;s_wavePending=s_trackMelee=false;
    s_meleeTarget=kNone;s_comboStep=s_comboTimeout=0;new_melee_swing();
    link->mGuardAtCps.SetR(s_run.shield_radius());
    if (s_run.rank(Upgrade::Shockwave) &&
        (manual_skill(link,Upgrade::Shockwave)||enemy_in_range(link,range))) launch_shockwave(link);
    return true;
}
void update_parallel_skills(daAlink_c* link) {
    if(s_run.rank(Upgrade::Spin) && !s_cooldowns[unsigned(Upgrade::Spin)] && launch_parallel_pulse(link,Upgrade::Spin))
        s_cooldowns[unsigned(Upgrade::Spin)]=s_run.cooldown(120);
    update_sword_wave(link); // A normal wave-casting cut yields to a due Spin.
    if(s_run.owns(ChestReward::Shield) && s_run.rank(Upgrade::ShieldAttack) &&
       !s_cooldowns[unsigned(Upgrade::ShieldAttack)]) {
        bool fired=launch_parallel_pulse(link,Upgrade::ShieldAttack);
        // Shockwave remains tied to the shield's cadence, but its manual toggle
        // does not force an automatic-only shield bash to fire without a target.
        if(!fired && !manual_skill(link,Upgrade::ShieldAttack) && !enemy_in_range(link,100+s_run.shield_radius()) &&
           s_run.rank(Upgrade::Shockwave) && manual_skill(link,Upgrade::Shockwave)) {
            launch_shockwave(link);fired=true;
        }
        if(fired)s_cooldowns[unsigned(Upgrade::ShieldAttack)]=s_run.shield_cooldown();
    }
}

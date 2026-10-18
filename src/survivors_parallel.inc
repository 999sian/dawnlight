// Spin and shield bash own separate, stable collision slots. They do not replace
// Link's body procedure, so normal cuts, waves and item projectiles can overlap.
constexpr unsigned kSpinSlot = 40, kShieldSlot = 41;
float parallel_spin_base_radius(const daAlink_c* link) {
    const auto& spin = link->mpHIO->mCut.mCutTurn.m;
    return s_run.master_sword >= 5 ? spin.mLightAttackRadius : spin.mAttackRadius;
}
void update_parallel_pulse(Attack& a, daAlink_c* link) {
    a.position = link->current.pos + cXyz(0,65,0);
    if (a.weapon == unsigned(Upgrade::ShieldAttack)) a.position += target_direction(link) * 100.f;
    for (auto key : a.emitters) if (auto* emitter=key?dComIfGp_particle_getEmitter(key):nullptr)
        emitter->setGlobalTranslation(a.position);
}
bool launch_parallel_pulse(daAlink_c* link, Upgrade weapon) {
    const bool spin=weapon==Upgrade::Spin;
    const float base=spin?parallel_spin_base_radius(link):60+8*s_run.rank(weapon);
    const float range=spin?s_run.radius(base):s_run.radius(200);
    if (!enemy_in_range(link,range)) return false;
    auto& a=s_attacks[spin?kSpinSlot:kShieldSlot];
    stop_attack(a);a={};
    a.active=true;a.weapon=unsigned(weapon);a.life=6; // Six registered frames; minimum cooldown is six.
    a.radius=s_run.radius(base);
    // Keep pulse centers attached to Link without adding projectile movement.
    a.status.Init(120,0xff,link);a.collider.Set(kAttackSource);a.collider.SetStts(&a.status);
    a.collider.SetAtType(AT_TYPE_NORMAL_SWORD);a.collider.SetAtMtrl(dCcD_MTRL_NONE);
    a.collider.SetAtAtp(static_cast<u8>(s_run.attack_power(unsigned(weapon))));
    a.collider.OnAtNoConHit();a.collider.ResetAtHit();a.collider.OnAtSetBit();
    a.position=link->current.pos+cXyz(0,65,0);
    if (!spin) a.position += target_direction(link)*100.f;
    if (spin) {
        constexpr std::array<u16,2> effects{ID_ZI_J_KAITENGIRI_A,ID_ZI_J_KAITENGIRI_B};
        // Normal-spin particles are authored for the native normal-spin radius.
        const float size=a.radius/std::max(1.f,link->mpHIO->mCut.mCutTurn.m.mAttackRadius);
        const cXyz scale(size,size,size);
        for(unsigned i=0;i<effects.size();++i)
            a.emitters[i]=dComIfGp_particle_set(0,effects[i],&a.position,&link->tevStr,
                nullptr,&scale,0xff,nullptr,-1,nullptr,nullptr,nullptr);
    }
    return true;
}
void update_parallel_skills(daAlink_c* link) {
    if(s_run.rank(Upgrade::Wave) && !s_cooldowns[unsigned(Upgrade::Wave)] && enemy_in_range(link,1600)) {
        release_sword_wave(link);
        s_cooldowns[unsigned(Upgrade::Wave)]=s_run.cooldown(40);
    }
    if(s_run.rank(Upgrade::Spin) && !s_cooldowns[unsigned(Upgrade::Spin)] && launch_parallel_pulse(link,Upgrade::Spin))
        s_cooldowns[unsigned(Upgrade::Spin)]=s_run.cooldown(100);
    if(s_run.owns(ChestReward::Shield) && s_run.rank(Upgrade::ShieldAttack) &&
       !s_cooldowns[unsigned(Upgrade::ShieldAttack)] && launch_parallel_pulse(link,Upgrade::ShieldAttack))
        s_cooldowns[unsigned(Upgrade::ShieldAttack)]=s_run.shield_cooldown();
}

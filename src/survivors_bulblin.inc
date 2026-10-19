// Keep native damage, effects and death/knockdown handling. Only Survivors-owned
// on-foot Bulblins participate; story riders and King Bulblin stay native.
struct BulblinAnimation {
    mDoExt_McaMorfSO* morph=nullptr;
    J3DAnmTransform* animation=nullptr;
    J3DFrameCtrl frames;
    float current=0,previous=0,step=0;
    void capture(mDoExt_McaMorfSO* value) {
        morph=value;
        if (!morph) return;
        animation=morph->getAnm(); frames=morph->mFrameCtrl;
        current=morph->mCurMorf; previous=morph->mPrevMorf; step=morph->mMorfStep;
    }
    void restore() const {
        if (!morph) return;
        morph->setAnm(animation,frames.getAttribute(),0,frames.getRate(),frames.getStart(),frames.getEnd());
        morph->mFrameCtrl=frames;
        morph->mCurMorf=current; morph->mPrevMorf=previous; morph->mMorfStep=step;
    }
};
struct BulblinHit {
    void* args=nullptr; e_rd_class* actor=nullptr;
    bool suppress=false,powerReady=false,critical=false;
    cCcD_Obj* collider=nullptr;
    unsigned basePower=0,scaledPower=0;
    unsigned power=0; int hitType=0;
    s16 action=0,mode=0,oldAction=0,timers[4]{},knockbackAngle=0,spin=0;
    int animation=0;
    float speedF=0,jump=0,knockback=0;
    cXyz speed{}; csXyz jumpAngle{};
    s8 damageFlag=0;
    BulblinAnimation body,bow;
};
std::vector<BulblinHit> s_bulblinHits;
HookAction before_bulblin_hit(ModContext*, void* args, void*, void*) {
    auto* bulblin=mods::arg<e_rd_class*>(args,0);
    if (!bulblin || !in_arena() || s_run.phase!=Phase::Running || bulblin->actor_set || bulblin->ride_mode) return HOOK_CONTINUE;
    auto* actor=&bulblin->enemy;
    auto* enemy=tracked(fopAcM_GetID(actor));
    if (!enemy || enemy->profile!=fpcNm_E_RD_e || !enemy->initialized || enemy->retiring || enemy->credited || actor->health<=0) return HOOK_CONTINUE;
    bulblin->one_hit_kill=0;
    // big_damage uses a 1000-frame lockout. Keep the real short hit cooldown,
    // including while airborne/prone; new ball return hits may bypass it.
    bulblin->damage_timer=std::min<s16>(bulblin->damage_timer,10);
    prepare_effect_hit(actor,*enemy);
    cCcD_Obj* attack=nullptr;
    for (auto& sphere:bulblin->cc_sph) if (sphere.ChkTgHit()) {attack=sphere.GetTgHitObj();break;}
    const int ball=ball_contact(actor,attack);
    if (ball==1) bulblin->damage_timer=0;
    if (ball==0) for (auto& sphere:bulblin->cc_sph) if (sphere.GetTgHitObj()==attack) sphere.ResetTgHit();
    if (!attack || ball==0 || bulblin->damage_timer) return HOOK_CONTINUE;
    BulblinHit hit;
    hit.args=args; hit.actor=bulblin; hit.collider=attack; hit.suppress=true;
    if (hit.suppress) {
        hit.action=bulblin->action; hit.mode=bulblin->mode; hit.oldAction=bulblin->old_action;
        std::copy(std::begin(bulblin->timer),std::end(bulblin->timer),hit.timers);
        hit.animation=bulblin->anm; hit.speedF=actor->speedF; hit.speed=actor->speed;
        hit.jump=bulblin->jump_z; hit.jumpAngle=bulblin->jump_angle;
        hit.knockback=bulblin->field_0x9f0; hit.knockbackAngle=bulblin->field_0x9f4;
        hit.spin=bulblin->field_0x9f6; hit.damageFlag=bulblin->damage_flag;
        hit.body.capture(bulblin->anm_p); hit.bow.capture(bulblin->bow_anm);
    }
    s_bulblinHits.push_back(hit);
    return HOOK_CONTINUE;
}
void after_bulblin_hit(ModContext*, void* args, void*, void*) {
    if (s_bulblinHits.empty() || s_bulblinHits.back().args!=args) return;
    const auto hit=s_bulblinHits.back(); s_bulblinHits.pop_back();
    auto* bulblin=hit.actor; auto* actor=&bulblin->enemy;
    finish_effect_hit(actor,fpcNm_E_RD_e);
    if (actor->health<=0) return; // Preserve all native lethal state and effects.
    bulblin->damage_timer=std::min<s16>(bulblin->damage_timer,10);
    if (!hit.suppress) return; // Living actors use the bounded run knockback.
    bulblin->action=hit.action; bulblin->mode=hit.mode; bulblin->old_action=hit.oldAction;
    std::copy(std::begin(hit.timers),std::end(hit.timers),bulblin->timer);
    bulblin->anm=hit.animation; actor->speedF=hit.speedF; actor->speed=hit.speed;
    bulblin->jump_z=hit.jump; bulblin->jump_angle=hit.jumpAngle;
    bulblin->field_0x9f0=hit.knockback; bulblin->field_0x9f4=hit.knockbackAngle;
    bulblin->field_0x9f6=hit.spin; bulblin->damage_flag=hit.damageFlag;
    hit.body.restore(); hit.bow.restore();
}
BulblinHit* bulblin_power_hit(dCcU_AtInfo* info) {
    if (s_bulblinHits.empty()) return nullptr;
    auto& hit=s_bulblinHits.back();
    return info==&hit.actor->at_info && info->mpCollider==hit.collider ? &hit : nullptr;
}
bool reuse_bulblin_power(dCcU_AtInfo* info) {
    const auto* hit=bulblin_power_hit(info);
    if (!hit || !hit->powerReady) return false;
    info->mAttackPower=hit->power; info->mHitType=hit->hitType;
    return true;
}
struct BulblinPowerCache {
    dCcU_AtInfo* info;
    ~BulblinPowerCache() {
        auto* hit=bulblin_power_hit(info);
        if (!hit) return;
        // Native arrows use HIT_TYPE_ARROW=15, which damage_check kills before
        // cc_at_check can subtract HP. Keep damage, change only that classifier.
        if (info->mHitType==HIT_TYPE_ARROW) info->mHitType=HIT_TYPE_LINK_HEAVY_ATTACK;
        // damage_check calls at_power_check twice for one hit. Roll crit once.
        hit->power=info->mAttackPower; hit->hitType=info->mHitType; hit->powerReady=true;
    }
};

// Independent projectiles with optional native upper-body presentation. No
// native item actor, inventory assignment or ammo consumption is synthesized.
struct RangedAction {
    bool active = false, released = false;
    Upgrade weapon = Upgrade::Bow;
    u16 animation = 0;
    cXyz target{};
    bool pulling = false, catching = false;
} s_ranged;
unsigned s_rangedRest = 0;
Upgrade s_lastRanged = Upgrade::Ball;
uint64_t s_bowVolleySerial = 0;

void cancel_ranged() {
    auto* link = daAlink_getAlinkActorClass();
    if (s_ranged.active && link && fopAcM_GetID(link) == s_player &&
        link->checkUpperAnime(s_ranged.animation)) {
        link->resetUpperAnime(daAlink_c::UPPER_2, 3);
        link->mBodyAngle.x = link->mBodyAngle.y = 0;
    }
    s_rangedRest = s_ranged.active ? 6 : 0;
    s_ranged = {};
}
void ranged_anime(daAlink_c* link, u16 animation, unsigned ticks) {
    s_ranged.animation = animation;
    link->setUpperAnimeBaseSpeed(animation, 1, 2);
    auto& frame = link->mUpperFrameCtrl[2];
    frame.setRate(std::max(1.0f, float(frame.getEnd()) / std::max(3u, ticks)));
}
bool queue_ranged(daAlink_c* link, Upgrade weapon) {
    if (!s_loot.ready || !playable() || !s_run.rank(weapon) || s_cooldowns[unsigned(weapon)]) return false;
    cXyz target;
    const bool manual=manual_attack(link);
    if(manual)target=manual_target(link,s_run.item_range(weapon));
    else if (!nearest_item_target(link, target, s_run.item_range(weapon))) return false;
    if (weapon == Upgrade::Bow) {
        const unsigned count=s_run.bow_shots();
        // Grow the group index space as needed; never shorten or block a volley.
        // Reuse only after every arrow from the previous volley has expired.
        unsigned group=0;
        while(std::any_of(s_attacks.begin(),s_attacks.end(),[&](const auto& a){
            return a.active && a.weapon==unsigned(Upgrade::Bow) && a.bowVolleySlot==group;
        })) ++group;
        const uint64_t volley=++s_bowVolleySerial;
        for(unsigned i=0;i<count;++i)
            if(!launch_item(link,weapon,target,s_run.arrow_spread(i),group,volley))return false;
    } else {
        std::array<ActorId,4> chosen{};
        const float range=weapon==Upgrade::Ball ? std::min(s_run.item_range(weapon),s_run.radius(900.f)) : s_run.item_range(weapon);
        for (unsigned i=0; i<s_run.ranged_shots(); ++i) {
            cXyz destination; ActorId id=kNone;
            if(manual)destination=manual_target(link,range);
            else if (!nearest_item_target(link,destination,range,std::span<const ActorId>(chosen.data(),i),&id) &&
                !nearest_item_target(link,destination,range,{},&id)) return false;
            if (!launch_item(link,weapon,destination,0,20,0,id)) return false;
            chosen[i]=id;
        }
    }
    s_cooldowns[unsigned(weapon)] = s_run.item_cooldown(weapon);
    // One native upper-body pose can be displayed at a time. It is cosmetic:
    // other weapons and the normal sword combo do not wait for it to finish.
    if (!s_ranged.active && !s_rangedRest && sword_action_available(link) && !link->checkEquipAnime()) {
        s_lastRanged = weapon;
        s_ranged.active = true; s_ranged.released = true;
        s_ranged.weapon = weapon; s_ranged.target = target;
        const u16 animation = weapon == Upgrade::Bow ? dRes_ID_ALANM_BCK_ASHOOT_e :
            weapon == Upgrade::Boomerang ? dRes_ID_ALANM_BCK_BOOMTHROW_e :
            weapon == Upgrade::Bombs ? dRes_ID_ALANM_BCK_BOMBTHROW_e : dRes_ID_ALANM_BCK_IBTHROW_e;
        ranged_anime(link, animation, std::max(3u,s_run.item_cooldown(weapon)/3));
    }
    return true;
}
void update_ranged(daAlink_c* link) {
    if (s_rangedRest) --s_rangedRest;
    if (!s_ranged.active) return;
    if (!sword_action_available(link) || !link->checkUpperAnime(s_ranged.animation) ||
        link->checkAnmEnd(&link->mUpperFrameCtrl[2])) cancel_ranged();
}
HookAction before_ranged_native(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_ranged.active || !in_arena() || !link || fopAcM_GetID(link) != s_player ||
        !link->checkUpperAnime(s_ranged.animation)) return HOOK_CONTINUE;
    *static_cast<int*>(result) = 0;
    // IBTHROW also passes checkIronBallAnime(). Its native action handler uses
    // the locomotion frame as a wind-up timer, and its next-action handler
    // enters IRON_BALL_MOVE/SUBJECT without an equipped ball. Both must yield
    // to our release/pull/catch scheduler, just like manual bow targeting.
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_ranged_aim(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_ranged.active || !link || fopAcM_GetID(link) != s_player || !playable() ||
        !sword_action_available(link) || !link->checkUpperAnime(s_ranged.animation)) return HOOK_CONTINUE;
    if (!s_ranged.released) nearest_item_target(link, s_ranged.target, s_run.item_range(s_ranged.weapon));
    const cXyz delta = manual_attack(link) ? manual_direction(link) : s_ranged.target - link->current.pos - cXyz(0, 70, 0);
    link->shape_angle.y = cM_atan2s(delta.x, delta.z);
    link->mBodyAngle.y = 0;
    link->mBodyAngle.x = s_ranged.weapon == Upgrade::Ball ? 0 :
        -cM_atan2s(delta.y, std::max(1.0f, delta.absXZ()));
    // current.angle controls travel; changing only shape/body preserves movement.
    return HOOK_CONTINUE;
}
void draw_ranged() {
    auto* link = daAlink_getAlinkActorClass();
    if (!s_ranged.active || !s_loot.ready || !link || fopAcM_GetID(link) != s_player ||
        !link->checkUpperAnime(s_ranged.animation)) return;
    const bool bow = s_ranged.weapon == Upgrade::Bow;
    if (!bow && s_ranged.released && !s_ranged.catching) return;
    auto* model = bow ? s_loot.bow : s_ranged.weapon == Upgrade::Bombs ? s_loot.heldBomb :
        s_ranged.weapon == Upgrade::Ball ? s_loot.heldBall : s_loot.heldBoomerang;
    mDoMtx_stack_c::copy(bow ? link->getRightItemMatrix() : link->getLeftItemMatrix());
    if (s_ranged.weapon == Upgrade::Boomerang) { // Native hand attachment.
        mDoMtx_stack_c::transM(32, -5, -6);
        mDoMtx_stack_c::XYZrotM(cM_deg2s(-4), cM_deg2s(39), cM_deg2s(-9));
    }
    model->setBaseTRMtx(mDoMtx_stack_c::get());
    const float size=s_ranged.weapon==Upgrade::Boomerang ? s_run.hurricane_scale() : 1.f;
    model->setBaseScale(cXyz(size,size,size));
    auto* data = model->getModelData(); auto* root = data->getJointNodePointer(0);
    auto* previous = root->getMtxCalc();
    if (bow) {
        auto* animation = s_loot.bowAnims[s_ranged.released ? 1 : 0];
        animation->entry(data, std::min(link->mUpperFrameCtrl[2].getFrame(), animation->getEndFrame()));
    }
    dKy_tevstr_c tev{}; g_env_light.settingTevStruct(0, &link->current.pos, &tev);
    g_env_light.setLightTevColorType_MAJI(model, &tev);
    mDoExt_modelUpdateDL(model);
    root->setMtxCalc(previous);
}

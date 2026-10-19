// Track only pickups created by this run. Native actors still own resources,
// collision, sounds, currency/healing and the heart-piece Get Item event.
struct RunPickup {
    ActorId id=kNone;
    int item=0;
    bool retiring=false, summoned=false;
};
std::vector<RunPickup> s_pickups;
struct ExpMagnet { bool pending=false, active=false, collected=false; cXyz position{}; };
// The first slots belong to timed bosses; reusable extra slots hold rare drops.
std::vector<ExpMagnet> s_expMagnets(timed_elite_count);
float magnet_range() { return float(s_run.magnet_range()); }
RunPickup* tracked_pickup(ActorId id) {
    if (id==kNone) return nullptr;
    for (auto& pickup:s_pickups) if (pickup.id==id) return &pickup;
    return nullptr;
}
ActorId create_run_pickup(cXyz position, int item) {
    struct Allow { bool previous=s_allowDrop; Allow(){s_allowDrop=true;} ~Allow(){s_allowDrop=previous;} } allow;
    const auto id=fopAcM_createItem(&position,item,-1,arena().room,nullptr,nullptr,
        daItem_c::TYPE_LAUNCH_e);
    if (id!=kNone) s_pickups.push_back({id,item});
    return id;
}
void retire_pickups() {
    s_expMagnets.assign(timed_elite_count,{});
    for (auto& pickup:s_pickups) pickup.retiring=true;
}
void cleanup_pickups() {
    for (auto& pickup:s_pickups) {
        if (pickup.id==kNone) continue;
        auto* actor=actor_by_id(pickup.id);
        if (!actor && !fpcM_IsCreating(pickup.id)) pickup.id=kNone;
        else if (actor && pickup.retiring) svc_actor->delete_actor(mod_ctx,pickup.id);
    }
    std::erase_if(s_pickups,[](const RunPickup& p){return p.id==kNone;});
}
bool pickup_waiting(const RunPickup& pickup, fopAc_ac_c* actor) {
    if (pickup.item==dItemNo_KAKERA_HEART_e)
        return static_cast<daObjLife_c*>(actor)->chkStatus(daObjLife_c::STATUS_WAIT_e);
    return static_cast<daItem_c*>(actor)->getStatus()==daItem_c::STATUS_NORMAL_e;
}
bool heart_or_rupee(const RunPickup& pickup) {
    return pickup.item>=dItemNo_HEART_e && pickup.item<=dItemNo_SILVER_RUPEE_e;
}
void move_pickup(RunPickup& pickup, fopAc_ac_c* actor) {
    if (pickup.retiring) return;
    if (heart_or_rupee(pickup)) {
        auto* item=static_cast<daItem_c*>(actor);
        item->setFlag(daItem_c::FLAG_NO_TIMER_e);
        // Keep the wait phase nonzero too, preventing native expiry flashing.
        item->mWaitTimer=std::max<decltype(item->mWaitTimer)>(item->mWaitTimer,1);
    }
    if (!playable() || s_run.phase!=Phase::Running || !pickup_waiting(pickup,actor)) return;
    auto* link=daAlink_getAlinkActorClass();
    if (!link || fopAcM_GetID(link)!=s_player) return;
    const cXyz delta=link->current.pos+cXyz(0,20,0)-actor->current.pos;
    const float distance=delta.abs();
    if (distance<1 || (!pickup.summoned && distance>magnet_range())) return;
    const float speed=pickup.summoned?std::max(35.f,distance*.08f):25.f;
    actor->current.pos+=delta*(std::min(speed,distance)/distance);
    actor->old.pos=actor->current.pos;
    actor->speedF=0; actor->speed=cXyz(0,0,0);
}
// Rendered like ordinary XP, without a native story-item actor or Get Item event.
void queue_exp_magnet(const Enemy& enemy, fopAc_ac_c* actor) {
    unsigned slot=0;
    if (enemy.elite) {
        if (enemy.eliteSlot<0 || unsigned(enemy.eliteSlot)>=timed_elite_count) return;
        slot=unsigned(enemy.eliteSlot);
    } else {
        slot=timed_elite_count;
        while (slot<s_expMagnets.size() && !s_expMagnets[slot].collected) ++slot;
        if (slot==s_expMagnets.size()) s_expMagnets.emplace_back();
        else s_expMagnets[slot]={};
    }
    auto& drop=s_expMagnets[slot];
    if (drop.pending || drop.active || drop.collected) return;
    cXyz position=actor->current.pos;
    if (!loot_floor_position(position)) position=enemy.lootPosition;
    drop.position=position; drop.pending=true;
}
void update_exp_magnets(daAlink_c* link) {
    if (!playable() || s_run.phase!=Phase::Running || !link) return;
    for (auto& drop:s_expMagnets) {
        if (drop.collected) continue;
        if (drop.pending) {
            // Keep the large crystal beside the chest on reachable, dry ground.
            for (unsigned i=0;i<8;++i) {
                const s16 yaw=s16(i*0x2000);
                cXyz pos=drop.position+cXyz(cM_ssin(yaw)*160,0,cM_scos(yaw)*160);
                if (!walkable_spawn_path(drop.position,pos,20)) continue;
                drop.position=pos+cXyz(0,50,0); drop.pending=false; drop.active=true;
                break;
            }
        }
        if (drop.active && (link->current.pos+cXyz(0,20,0)-drop.position).abs()<100) {
            for (auto& orb:s_orbs) if (orb.value) orb.summoned=true;
            for (auto& pickup:s_pickups)
                if (pickup.id!=kNone && !pickup.retiring && heart_or_rupee(pickup)) pickup.summoned=true;
            drop.active=false; drop.collected=true;
        }
    }
}

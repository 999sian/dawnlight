// Track only pickups created by this run. Native actors still own resources,
// collision, sounds, currency/healing and the heart-piece Get Item event.
struct RunPickup {
    ActorId id=kNone;
    int item=0;
    bool retiring=false;
};
std::vector<RunPickup> s_pickups;
struct ExpMagnet { bool pending=false, active=false, collected=false; cXyz position{}; };
std::array<ExpMagnet,timed_elite_count> s_expMagnets;
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
    s_expMagnets={};
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
void move_pickup(RunPickup& pickup, fopAc_ac_c* actor) {
    if (!playable() || s_run.phase!=Phase::Running || !pickup_waiting(pickup,actor)) return;
    auto* link=daAlink_getAlinkActorClass();
    if (!link || fopAcM_GetID(link)!=s_player) return;
    const cXyz delta=link->current.pos+cXyz(0,20,0)-actor->current.pos;
    const float distance=delta.abs();
    if (distance<1 || distance>magnet_range()) return;
    actor->current.pos+=delta*(std::min(25.f,distance)/distance);
    actor->old.pos=actor->current.pos;
    actor->speedF=0; actor->speed=cXyz(0,0,0);
}
// Rendered like ordinary XP, without a native story-item actor or Get Item event.
void queue_exp_magnet(const Enemy& enemy, fopAc_ac_c* actor) {
    if (!enemy.elite || enemy.eliteSlot<0 || unsigned(enemy.eliteSlot)>=s_expMagnets.size()) return;
    auto& drop=s_expMagnets[enemy.eliteSlot];
    if (drop.pending || drop.active || drop.collected) return;
    cXyz position=actor->current.pos;
    if (!dry_arena_floor(position,250,240)) position=enemy.lootPosition;
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
            drop.active=false; drop.collected=true;
        }
    }
}

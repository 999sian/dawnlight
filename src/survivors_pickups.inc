// Track only pickups created by this run. Native actors still own resources,
// collision, sounds, currency/healing and the heart-piece Get Item event.
struct RunPickup {
    ActorId id=kNone;
    int item=0;
    bool retiring=false, enlarged=false;
};
std::vector<RunPickup> s_pickups;
struct FluteDrop { bool pending=false, collected=false; ActorId id=kNone; cXyz position{}; };
std::array<FluteDrop,timed_elite_count> s_flutes;
float magnet_range() { return float(s_run.magnet_range()); }
RunPickup* tracked_pickup(ActorId id) {
    if (id==kNone) return nullptr;
    for (auto& pickup:s_pickups) if (pickup.id==id) return &pickup;
    return nullptr;
}
ActorId create_run_pickup(cXyz position, int item) {
    struct Allow { bool previous=s_allowDrop; Allow(){s_allowDrop=true;} ~Allow(){s_allowDrop=previous;} } allow;
    const auto id=fopAcM_createItem(&position,item,-1,arena().room,nullptr,nullptr,
        item==dItemNo_HORSE_FLUTE_e ? daItem_c::TYPE_FIXED_PLACE_e : daItem_c::TYPE_LAUNCH_e);
    if (id!=kNone) s_pickups.push_back({id,item});
    return id;
}
void retire_pickups() {
    s_flutes={};
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
    if (pickup.item==dItemNo_HORSE_FLUTE_e) {
        auto* item=static_cast<daItem_c*>(actor);
        if (!pickup.enlarged) {
            item->mItemScale*=5.f; item->scale=item->mItemScale;
            pickup.enlarged=true;
        }
        item->setFlag(daItem_c::FLAG_NO_TIMER_e);
        item->mWaitTimer=100; // Do not enter native blink/delete even with a zero resource timer.
        if (distance<80) {
            for (auto& orb:s_orbs) if (orb.value) orb.summoned=true;
            for (auto& flute:s_flutes) if (flute.id==pickup.id) flute.collected=true;
            pickup.retiring=true; item->hide();
        }
        return;
    }
    if (distance<1 || distance>magnet_range()) return;
    actor->current.pos+=delta*(std::min(25.f,distance)/distance);
    actor->old.pos=actor->current.pos;
    actor->speedF=0; actor->speed=cXyz(0,0,0);
}
HookAction before_flute_get(ModContext*, void* args, void*, void*) {
    auto* item=mods::arg<daItem_c*>(args,0);
    auto* pickup=item ? tracked_pickup(fopAcM_GetID(item)) : nullptr;
    // Never equip a story flute, mutate an item flag or start a native Get Item
    // event for this presentation-only pickup (including during retirement).
    return pickup && pickup->item==dItemNo_HORSE_FLUTE_e ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}
void queue_flute(const Enemy& enemy, fopAc_ac_c* actor) {
    if (!enemy.elite || enemy.eliteSlot<0 || unsigned(enemy.eliteSlot)>=s_flutes.size()) return;
    auto& flute=s_flutes[enemy.eliteSlot];
    if (flute.pending || flute.id!=kNone || flute.collected) return;
    cXyz position=actor->current.pos;
    if (!dry_arena_floor(position,250,240)) position=enemy.lootPosition;
    flute.position=position; flute.pending=true;
}
void update_flutes(daAlink_c*) {
    if (!playable() || s_run.phase!=Phase::Running) return;
    for (auto& flute:s_flutes) {
        if (flute.collected) continue;
        if (flute.id!=kNone && !tracked_pickup(flute.id)) {flute.id=kNone;flute.pending=true;}
        if (!flute.pending) continue;
        // Small offset from the corpse/chest; stay on connected dry ground.
        for (unsigned i=0;i<8;++i) {
            const s16 yaw=s16(i*0x2000);
            cXyz pos=flute.position+cXyz(cM_ssin(yaw)*160,0,cM_scos(yaw)*160);
            if (!walkable_spawn_path(flute.position,pos,20)) continue;
            pos.y+=15;
            flute.id=create_run_pickup(pos,dItemNo_HORSE_FLUTE_e);
            if (flute.id!=kNone) flute.pending=false;
            break;
        }
    }
}

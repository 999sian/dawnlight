// The first slots belong to the five-minute schedule, the rest to map guards.
// Actor IDs are checked on every spawn pass; failed creation/unload is retried
// without treating a deletion as a kill or issuing another chest.
struct EliteEncounter {
    bool placed = false, defeated = false;
    cXyz position{};
    s16 yaw = 0;
};
std::array<EliteEncounter, elite_capacity> s_eliteEncounters;
void reset_elite_encounters() {
    s_eliteEncounters = {};
    if (auto* link = daAlink_getAlinkActorClass())
        s_eliteEncounters[timed_elite_count].position = link->current.pos;
}

bool elite_floor_position(cXyz& pos) {
    cXyz probe = pos + cXyz(0, 250, 0);
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float floor = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(floor) || floor < -1.e8f || std::abs(floor-pos.y) > 240 ||
        dComIfG_Bgsp().GetRoomId(ground) != arena().room) return false;
    const int code = dComIfG_Bgsp().GetGroundCode(ground);
    if (code == 4 || code == 5 || code == 10) return false;
    pos.y = floor;
    // A doubled Bokoblin / large chest must fit on real floor, away from exits.
    constexpr float offsets[4][2]{{-100,-100},{100,-100},{100,100},{-100,100}};
    for (const auto& offset : offsets) {
        cXyz corner = pos + cXyz(offset[0], 0, offset[1]);
        cXyz above = corner + cXyz(0, 150, 0); ground.SetPos(&above);
        const float h = dComIfG_Bgsp().GroundCross(&ground);
        if (!std::isfinite(h) || std::abs(h-floor) > 45 || dComIfG_Bgsp().GetRoomId(ground) != arena().room) return false;
        cXyz from = pos + cXyz(0, 100, 0), to = corner + cXyz(0, 100, 0);
        dBgS_LinChk wall; wall.Set(&from, &to, nullptr);
        if (dComIfG_Bgsp().LineCross(&wall) || !arena_path_clear(from,to,100)) return false;
    }
    return true;
}
bool separated_guard(cXyz pos, float distance) {
    for (unsigned i=timed_elite_count; i<s_eliteEncounters.size(); ++i)
        if (s_eliteEncounters[i].placed && s_eliteEncounters[i].position.absXZ(pos) < distance) return false;
    return true;
}
bool random_guard_position(cXyz& pos) {
    if (s_eliteFloors.empty() || s_eliteFloorArea <= 0) return false;
    const float roll = float(s_run.next_random() & 0xffffff) / 16777216.f * s_eliteFloorArea;
    auto tile = std::lower_bound(s_eliteFloors.begin(),s_eliteFloors.end(),roll,
        [](const EliteFloor& f,float value){return f.cumulative < value;});
    if (tile == s_eliteFloors.end()) tile = s_eliteFloors.end()-1;
    float a = float(s_run.next_random() & 0xffff)/65536.f;
    float b = float(s_run.next_random() & 0xffff)/65536.f;
    if (a+b>1) {a=1-a;b=1-b;}
    pos = tile->a + (tile->b-tile->a)*a + (tile->c-tile->a)*b;
    return elite_floor_position(pos) && separated_guard(pos,1800);
}
bool landmark_guard_position(const EliteLandmark& site, cXyz& pos, bool door) {
    // Test both door faces; the native room check selects the exterior side.
    // Reject the hidden/blocked side of the exit curtain, but allow a guard
    // on a distant platform without requiring line of sight to Link.
    for (unsigned attempt=0;attempt<16;++attempt) {
        const s16 yaw = s16(site.yaw + (attempt%8)*0x2000);
        const float radius = door ? (attempt<8 ? 450.f : 650.f) : (attempt ? 160.f : 0.f);
        pos = site.position + cXyz(cM_ssin(yaw)*radius,0,cM_scos(yaw)*radius);
        if (elite_floor_position(pos) && separated_guard(pos,500)) return true;
    }
    return false;
}
bool throne_guard_position(unsigned guard, daAlink_c* link, cXyz& pos) {
    // Capture the entrance at run start, then select the opposite end from
    // real room floor. The score is independent of Link's later movement.
    const auto& entry = s_eliteEncounters[timed_elite_count].position;
    if (guard == 0) {
        return landmark_guard_position({entry, link->shape_angle.y},pos,true);
    }
    cXyz low=entry, high=entry;
    for (const auto& tile : s_eliteFloors) for (const auto& point : {tile.a,tile.b,tile.c}) {
        low.x=std::min(low.x,point.x);low.z=std::min(low.z,point.z);
        high.x=std::max(high.x,point.x);high.z=std::max(high.z,point.z);
    }
    const bool alongX=high.x-low.x > high.z-low.z;
    const float middle=alongX ? (low.z+high.z)*0.5f : (low.x+high.x)*0.5f;
    const float axisCenter=alongX ? (low.x+high.x)*0.5f : (low.z+high.z)*0.5f;
    const float direction=(alongX ? entry.x : entry.z)<axisCenter ? 1.f : -1.f;
    float best=-1.e30f; bool found=false;
    for (const auto& tile : s_eliteFloors) {
        const cXyz center=(tile.a+tile.b+tile.c)*(1.f/3);
        // Probe near the vertices too: a large floor triangle's centroid can
        // otherwise leave the far guard halfway down the throne room.
        for (const auto& point : {center,tile.a,tile.b,tile.c}) {
            cXyz candidate=center*0.2f+point*0.8f;
            const float score=direction*(alongX ? candidate.x : candidate.z) -
                0.5f*std::abs((alongX ? candidate.z : candidate.x)-middle);
            if (score<=best || !separated_guard(candidate,600) || !elite_floor_position(candidate)) continue;
            pos=candidate;best=score;found=true;
        }
    }
    return found;
}
void update_elite_spawns(daAlink_c* link) {
    const unsigned guards = s_map==0 ? 1 : s_map==1 || s_map==2 ? 4 :
        s_map==4 ? 2 : std::min<unsigned>(guard_capacity,s_eliteDoorSites.size());
    for (unsigned i=0;i<timed_elite_count+guards;++i) {
        auto& encounter = s_eliteEncounters[i];
        if (encounter.defeated) continue;
        const bool guard = i>=timed_elite_count;
        if (!guard && s_run.ticks < (i+1)*elite_interval) continue;
        bool exists = false;
        for (const auto& enemy : s_enemies) if (enemy.id!=kNone && enemy.eliteSlot==int(i)) {exists=true;break;}
        if (exists) continue;
        if (!guard) {
            if (!spawn_enemy(link,true,int(i))) ++s_spawnFailures;
            continue;
        }
        if (!encounter.placed) {
            const unsigned n=i-timed_elite_count;
            bool found=false; cXyz pos;
            if (s_map==1 || s_map==2) {
                for (unsigned attempt=0;attempt<64 && !found;++attempt) found=random_guard_position(pos);
            } else if (s_map==0 && !s_eliteChestSites.empty()) {
                found=landmark_guard_position(s_eliteChestSites.front(),pos,false);
            } else if (s_map==3) {
                found=landmark_guard_position(s_eliteDoorSites[n],pos,true);
            } else if (s_map==4) found=throne_guard_position(n,link,pos);
            if (!found) {++s_spawnFailures;continue;}
            encounter.position=pos; encounter.yaw=s16(s_run.next_random()); encounter.placed=true;
        }
        if (!create_enemy_at(encounter.position,encounter.yaw,0,true,int(i),true)) ++s_spawnFailures;
    }
}
// Skip only the idle AI, never Execute: native damage, animation, physics and
// collision registration continue, so a resting guard remains hittable.
HookAction before_bokoblin_wait(ModContext*, void* args, void*, void*) {
    auto* actor=mods::arg<daE_OC_c*>(args,0);
    auto* enemy=actor ? tracked(fopAcM_GetID(actor)) : nullptr;
    if (!in_arena() || !enemy || !enemy->dormant || enemy->retiring || enemy->credited) return HOOK_CONTINUE;
    if (!enemy->idleAnimation) {actor->setBck(0x18,2,5.f,1.f);enemy->idleAnimation=true;}
    actor->speedF=0; actor->speed.x=actor->speed.z=0;
    return HOOK_SKIP_ORIGINAL;
}

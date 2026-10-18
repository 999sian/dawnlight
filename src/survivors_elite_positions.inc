// Keep authored landmarks before filtering story actors. Floor samples come
// from the loaded room's DZB/KCL, so no region-specific world coordinates leak in.
struct EliteLandmark { cXyz position; s16 yaw; };
struct EliteFloor { cXyz a, b, c; float cumulative; };
std::vector<EliteLandmark> s_eliteChestSites, s_eliteDoorSites;
std::vector<EliteFloor> s_eliteFloors;
float s_eliteFloorArea = 0;
void clear_elite_sites() {
    s_eliteChestSites.clear(); s_eliteDoorSites.clear(); s_eliteFloors.clear(); s_eliteFloorArea = 0;
}
void add_elite_site(std::vector<EliteLandmark>& sites, cXyz position, s16 yaw) {
    for (const auto& site : sites)
        if (site.position.absXZ(position) < 500 && std::abs(site.position.y-position.y) < 500) return;
    sites.push_back({position,yaw});
}
void capture_elite_site(int profile, const fopAcM_prm_class& params) {
    const bool chest = profile == fpcNm_TBOX_e || profile == fpcNm_TBOX2_e;
    const bool door = profile == fpcNm_DOOR20_e || profile == fpcNm_DBDOOR_e ||
        profile == fpcNm_BOSS_DOOR_e || profile == fpcNm_L1BOSS_DOOR_e;
    if (chest) {
        if (params.room_no == arena().room)
            add_elite_site(s_eliteChestSites,params.base.position,params.base.angle.y);
    } else if (door) {
        // Shared stage doors can belong to the neighbouring room. Their two
        // room IDs, not the actor's owning room, identify the arena doorway.
        const unsigned front = (params.base.parameters >> 13) & 63;
        const unsigned back = (params.base.parameters >> 19) & 63;
        if (front != unsigned(arena().room) && back != unsigned(arena().room)) return;
        const s16 yaw = s16(s16(params.base.angle.y) + (front == unsigned(arena().room) ? 0 : 0x8000));
        add_elite_site(s_eliteDoorSites,params.base.position,yaw);
    }
}
void capture_elite_actor(const char* rawName, const fopAcM_prm_class& params) {
    char name[9]{}; std::memcpy(name,rawName,8);
    // The Darknut arena's existing dStage_searchName hook deliberately returns
    // null for story chests. Read their authored name BEFORE that lookup.
    if (std::strncmp(name,"tbox",4) == 0) {
        capture_elite_site(fpcNm_TBOX_e,params);
    } else if (const auto* info = dStage_searchName(name)) {
        capture_elite_site(info->procname,params);
    }
}
void refresh_elite_sites() {
    // Read retained records as well: stage doors may be decoded before the
    // Survivors arena becomes active, or skipped by native suspension checks.
    auto* room = dComIfGp_roomControl_getStatusRoomDt(arena().room);
    if (room) if (auto* list = room->getTresure(); list && list->num > 0 && list->num < 1024) {
        for (int i=0;i<list->num;++i) {
            const auto& entry = list->m_entries[i];
            fopAcM_prm_class params{}; params.base=entry.base; params.room_no=arena().room;
            capture_elite_actor(entry.name,params);
        }
    }
    const auto doors = [](const stage_tgsc_data_class* entries, int count) {
        if (!entries || count<0 || count>1024) return;
        for (int i=0;i<count;++i) {
            fopAcM_prm_class params{}; params.base=entries[i].base; params.room_no=-1;
            // Door/DRTG tables already identify door records. Map-only entries
            // need not resolve to a live actor profile at all.
            capture_elite_site(fpcNm_DOOR20_e,params);
        }
    };
    const auto list = [&](const stage_tgsc_class* entries) {
        if (entries) doors(entries->m_entries,entries->num);
    };
    if (room) { list(room->getDrTg()); list(room->getDoor()); }
    if (auto* stage = dComIfGp_getStage()) list(stage->getDrTg());
    for (const auto* keep : {dStage_GetKeepDoorInfo(),dStage_GetRoomKeepDoorInfo()})
        if (keep && keep->mNum>=0 && keep->mNum<=64) doors(keep->mDrTgData,keep->mNum);
}
void capture_elite_floor(cXyz a, cXyz b, cXyz c, float normal, bool exit) {
    if (normal < 0.75f || exit) return;
    const float area = std::abs((b.x-a.x)*(c.z-a.z) - (b.z-a.z)*(c.x-a.x)) * 0.5f;
    if (area < 1 || !std::isfinite(area)) return;
    s_eliteFloorArea += area;
    s_eliteFloors.push_back({a, b, c, s_eliteFloorArea});
}

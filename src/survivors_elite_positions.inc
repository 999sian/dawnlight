// Keep authored landmarks before filtering story actors. Floor samples come
// from the loaded room's DZB/KCL, so no region-specific world coordinates leak in.
struct EliteLandmark { cXyz position; s16 yaw; };
struct EliteFloor { cXyz a, b, c; float cumulative; };
std::vector<EliteLandmark> s_eliteDoorSites;
std::vector<EliteFloor> s_eliteFloors;
float s_eliteFloorArea = 0;
void clear_elite_sites() {
    s_eliteDoorSites.clear(); s_eliteFloors.clear(); s_eliteFloorArea = 0;
}
void add_elite_site(std::vector<EliteLandmark>& sites, cXyz position, s16 yaw) {
    for (const auto& site : sites)
        if (site.position.absXZ(position) < 500 && std::abs(site.position.y-position.y) < 500) return;
    sites.push_back({position,yaw});
}
void capture_elite_site(int profile, const fopAcM_prm_class& params) {
    const bool door = profile == fpcNm_DOOR20_e || profile == fpcNm_DBDOOR_e ||
        profile == fpcNm_BOSS_DOOR_e || profile == fpcNm_L1BOSS_DOOR_e;
    if (door) {
        // Shared stage doors can belong to the neighbouring room. Their two
        // room IDs, not the actor's owning room, identify the arena doorway.
        const unsigned front = (params.base.parameters >> 13) & 63;
        const unsigned back = (params.base.parameters >> 19) & 63;
        if (front != unsigned(arena().room) && back != unsigned(arena().room)) return;
        const s16 yaw = s16(s16(params.base.angle.y) + (front == unsigned(arena().room) ? 0 : 0x8000));
        add_elite_site(s_eliteDoorSites,params.base.position,yaw);
    }
}
void refresh_elite_sites() {
    // Retained door records also cover suspended/uncreated Palace entrances.
    auto* room = dComIfGp_roomControl_getStatusRoomDt(arena().room);
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

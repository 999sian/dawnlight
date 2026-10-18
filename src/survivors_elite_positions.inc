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
void capture_elite_site(int profile, const fopAcM_prm_class& params) {
    if (params.room_no != arena().room && params.room_no != -1) return;
    auto* sites = (profile == fpcNm_TBOX_e || profile == fpcNm_TBOX2_e) ? &s_eliteChestSites :
        (profile == fpcNm_DOOR20_e || profile == fpcNm_DBDOOR_e || profile == fpcNm_BOSS_DOOR_e ||
         profile == fpcNm_L1BOSS_DOOR_e) ? &s_eliteDoorSites : nullptr;
    if (!sites) return;
    for (const auto& site : *sites)
        if (site.position.absXZ(params.base.position) < 500) return;
    sites->push_back({params.base.position, params.base.angle.y});
}
void capture_elite_floor(cXyz a, cXyz b, cXyz c, float normal, bool exit) {
    if (normal < 0.75f || exit) return;
    const float area = std::abs((b.x-a.x)*(c.z-a.z) - (b.z-a.z)*(c.x-a.x)) * 0.5f;
    if (area < 1 || !std::isfinite(area)) return;
    s_eliteFloorArea += area;
    s_eliteFloors.push_back({a, b, c, s_eliteFloorArea});
}

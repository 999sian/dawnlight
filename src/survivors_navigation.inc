// Field entrance meshes can include player-through collision faces. Native
// Bokoblin probes have no Link pass flag, so they can see a wall Link cannot.
// Never change the map polygons or the collision behavior of story actors.
void after_enemy_navigation_line(ModContext*, void* args, void*, void*) {
    if (!in_arena() || s_map!=1 || s_run.phase!=Phase::Running || s_processFrames.empty()) return;
    const auto* enemy=tracked(s_processFrames.back().id);
    if (!enemy || enemy->profile!=fpcNm_E_OC_e || !enemy->initialized || enemy->retiring || enemy->credited) return;
    auto* query=mods::arg<dBgS_LinChk*>(args,0);
    if (!query || query->ChkCam() || query->ChkArrow() || query->ChkBomb() || query->ChkBoomerang()) return;
    query->SetLink(); // Solid walls still block; only Link-through faces are ignored.
}

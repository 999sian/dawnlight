// Field entrance meshes can include player-through collision faces. Native
// Bokoblin probes have no Link pass flag, so they can see a wall Link cannot.
// Never change the map polygons or the collision behavior of story actors.
HookAction before_enemy_navigation_line(ModContext*, void* args, void*, void*) {
    if (!in_arena() || s_map!=1 || s_run.phase!=Phase::Running || s_processFrames.empty()) return HOOK_CONTINUE;
    const auto* enemy=tracked(s_processFrames.back().id);
    if (!enemy || enemy->profile!=fpcNm_E_OC_e || !enemy->initialized || enemy->retiring || enemy->credited) return HOOK_CONTINUE;
    // The dBgS_LinChk::Set wrapper also starts with an unrelocatable short
    // branch on Windows. Apply the pass flag at the actual collision query.
    auto* line=mods::arg<cBgS_LinChk*>(args,1);
    // dBgW::ChkPolyThrough uses this same pass-filter type for game queries.
    auto* query=line?static_cast<dBgS_PolyPassChk*>(line->GetPolyPassChk()):nullptr;
    if (!query || query->ChkCam() || query->ChkArrow() || query->ChkBomb() || query->ChkBoomerang()) return HOOK_CONTINUE;
    query->SetLink(); // Solid walls still block; only Link-through faces are ignored.
    return HOOK_CONTINUE;
}

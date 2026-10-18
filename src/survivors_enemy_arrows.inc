// Native bow projectiles outlive their shooter. Retain ownership independently
// so pause, difficulty and teardown still apply after the Bulblin dies.
struct EnemyArrow { ActorId id=kNone; bool retiring=false; };
std::array<EnemyArrow, enemy_capacity*3> s_enemyArrows;
EnemyArrow* tracked_enemy_arrow(ActorId id) {
    for (auto& arrow:s_enemyArrows) if (arrow.id!=kNone && arrow.id==id) return &arrow;
    return nullptr;
}
bool active_enemy_arrow(ActorId id) {
    const auto* arrow=tracked_enemy_arrow(id);
    return arrow && !arrow->retiring;
}
bool owned_arrow_spawn(void* args) {
    if (!in_arena() || mods::arg<s16>(args,0)!=fpcNm_E_ARROW_e) return false;
    const auto* enemy=tracked(mods::arg<ActorId>(args,1));
    return enemy && enemy->profile==fpcNm_E_RD_e;
}
HookAction before_enemy_arrow(ModContext*, void* args, void* result, void*) {
    if (!owned_arrow_spawn(args)) return HOOK_CONTINUE;
    const auto* enemy=tracked(mods::arg<ActorId>(args,1));
    const bool full=std::none_of(s_enemyArrows.begin(),s_enemyArrows.end(),[](const auto& a){return a.id==kNone;});
    if (enemy->retiring || enemy->credited || s_run.phase!=Phase::Running || full) {
        *static_cast<ActorId*>(result)=kNone;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_enemy_arrow(ModContext*, void* args, void* result, void*) {
    const auto id=*static_cast<ActorId*>(result);
    if (id==kNone || !owned_arrow_spawn(args) || tracked_enemy_arrow(id)) return;
    for (auto& arrow:s_enemyArrows) if (arrow.id==kNone) {arrow={id,false};return;}
}
void retire_enemy_arrows() {
    for (auto& arrow:s_enemyArrows) if (arrow.id!=kNone) arrow.retiring=true;
}
void cleanup_enemy_arrows() {
    for (auto& arrow:s_enemyArrows) {
        if (arrow.id==kNone) continue;
        auto* actor=actor_by_id(arrow.id);
        if (!actor && !fpcM_IsCreating(arrow.id)) arrow={};
        else if (actor && arrow.retiring) svc_actor->delete_actor(mod_ctx,arrow.id);
    }
}

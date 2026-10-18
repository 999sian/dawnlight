// Arrows/eggs outlive their shooter. Armor, weapons, cloaks and Phantom Zant
// children retire with their owner; all share run pause, damage and teardown.
struct EnemyArrow { ActorId id=kNone; bool retiring=false; ActorId parent=kNone; bool shell=false; };
std::array<EnemyArrow, enemy_capacity*3> s_enemyArrows;
EnemyArrow* tracked_enemy_arrow(ActorId id) {
    for (auto& arrow:s_enemyArrows) if (arrow.id!=kNone && arrow.id==id) {
        if(arrow.shell) {
            const auto* parent=tracked(arrow.parent);
            if(!parent||parent->retiring||parent->credited)arrow.retiring=true;
        }
        return &arrow;
    }
    return nullptr;
}
bool active_enemy_arrow(ActorId id) {
    const auto* arrow=tracked_enemy_arrow(id);
    return arrow && !arrow->retiring;
}
bool structural_enemy_child(s16 profile) {
    return profile==fpcNm_E_MM_MT_e||profile==fpcNm_E_TH_BALL_e||profile==fpcNm_MANT_e;
}
bool attached_enemy_child(s16 profile) {return structural_enemy_child(profile)||profile==fpcNm_E_PZ_e;}
bool owned_arrow_spawn(void* args) {
    if (!in_arena()) return false;
    const auto* enemy=tracked(mods::arg<ActorId>(args,1));if(!enemy)return false;
    const auto profile=mods::arg<s16>(args,0);
    return ((enemy->profile==fpcNm_E_RD_e||enemy->profile==fpcNm_E_RDY_e)&&profile==fpcNm_E_ARROW_e) ||
        (enemy->profile==fpcNm_E_CR_e&&profile==fpcNm_E_CR_EGG_e) ||
        (enemy->profile==fpcNm_E_MM_e&&profile==fpcNm_E_MM_MT_e) ||
        (enemy->profile==fpcNm_E_TH_e&&profile==fpcNm_E_TH_BALL_e) ||
        (enemy->profile==fpcNm_B_GND_e&&profile==fpcNm_MANT_e) ||
        (enemy->profile==fpcNm_E_PZ_e&&profile==fpcNm_E_PZ_e) ||
        (enemy->profile==fpcNm_B_DS_e&&profile==fpcNm_B_DS_e&&
         ((mods::arg<u32>(args,2)&0xff)==daB_DS_c::TYPE_BULLET_B||
          (mods::arg<u32>(args,2)&0xff)==daB_DS_c::TYPE_BULLET_C));
}
HookAction before_enemy_arrow(ModContext*, void* args, void* result, void*) {
    if (!owned_arrow_spawn(args)) return HOOK_CONTINUE;
    const auto* enemy=tracked(mods::arg<ActorId>(args,1));
    const bool shell=structural_enemy_child(mods::arg<s16>(args,0));
    const auto used=std::count_if(s_enemyArrows.begin(),s_enemyArrows.end(),[](const auto& a){return a.id!=kNone;});
    // Structural children must finish asynchronous creation during menus too.
    // Reserve one armor slot per enemy; projectiles cannot exhaust that reserve.
    const bool full=used>=int(s_enemyArrows.size()-(shell?0:enemy_capacity));
    if (enemy->retiring || enemy->credited || (!shell&&s_run.phase!=Phase::Running) || full) {
        *static_cast<ActorId*>(result)=kNone;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_enemy_arrow(ModContext*, void* args, void* result, void*) {
    const auto id=*static_cast<ActorId*>(result);
    if (id==kNone || !owned_arrow_spawn(args) || tracked_enemy_arrow(id)) return;
    for (auto& arrow:s_enemyArrows) if (arrow.id==kNone) {arrow={id,false,mods::arg<ActorId>(args,1),attached_enemy_child(mods::arg<s16>(args,0))};return;}
}
void retire_enemy_arrows() {
    for (auto& arrow:s_enemyArrows) if (arrow.id!=kNone) arrow.retiring=true;
}
void cleanup_enemy_arrows() {
    for (auto& arrow:s_enemyArrows) {
        if (arrow.id==kNone) continue;
        if(arrow.shell) {
            const auto* parent=tracked(arrow.parent);
            if(!parent||parent->retiring||parent->credited)arrow.retiring=true;
        }
        auto* actor=actor_by_id(arrow.id);
        if (!actor && !fpcM_IsCreating(arrow.id)) arrow={};
        else if (actor && arrow.retiring) svc_actor->delete_actor(mod_ctx,arrow.id);
    }
}

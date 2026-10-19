// Filter only physical Link/enemy pairs. Attack/target collisions, terrain and
// object contacts (including pots and chests) keep their native processing.
HookAction before_enemy_contact(ModContext*,void* args,void*,void*) {
    if(!in_arena() || s_hub || s_run.options.enemy_collision)return HOOK_CONTINUE;
    auto* first=mods::arg<cCcD_Obj*>(args,1);
    auto* second=mods::arg<cCcD_Obj*>(args,3);
    auto* a=first?first->GetAc():nullptr;
    auto* b=second?second->GetAc():nullptr;
    auto* link=daAlink_getAlinkActorClass();
    if(!link || fopAcM_GetID(link)!=s_player || !a || !b)return HOOK_CONTINUE;
    auto* other=a==link?b:b==link?a:nullptr;
    return other && enemy_child_root(fopAcM_GetID(other)) ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}

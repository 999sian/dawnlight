HookAction before_key_draw(ModContext*, void* args, void*, void*) {
    auto* process = mods::arg<leafdraw_class*>(args, 0);
    auto* pickup = process ? tracked_pickup(fopAcM_GetID(process)) : nullptr;
    if (!pickup || pickup->retiring || pickup->item != dItemNo_KEY_OF_FILONE_e ||
        !in_arena() || transitioning()) return HOOK_CONTINUE;
    auto* actor = actor_by_id(pickup->id);
    if (!actor || fopAcM_GetName(actor) != fpcNm_ITEM_e) return HOOK_CONTINUE;
    auto* item = static_cast<daItem_c*>(actor);
    if (!item->mpModel || !item->chkDraw()) return HOOK_CONTINUE;
    // KEY_OF_FILONE uses the same T_g_key field model as the ordinary small
    // key. ITEM's generic matrix leaves it flat/clipped into the floor; mirror
    // daKey_c::setBaseMtx after native execution, immediately before drawing.
    // Keep the physical position/cylinder and native rotation/scale unchanged.
    mDoMtx_stack_c::transS(item->current.pos);
    mDoMtx_stack_c::transM(0.f, 5.f, 0.f);
    mDoMtx_stack_c::ZXYrotM(item->shape_angle);
    mDoMtx_stack_c::XrotM(-0x4000);
    item->mpModel->setBaseTRMtx(mDoMtx_stack_c::get());
    return HOOK_CONTINUE; // Native lighting, materials, draw and cleanup own it.
}

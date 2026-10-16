// Included inside fierce_deity_visual.cpp's private namespace.
// Keep native texture/alpha animation and batching. Zero source contribution
// gives black silhouettes even for additive fire or texture-only TEV colors:
// result = background * (1 - native flame alpha).
void after_flame_shape_setup(ModContext*, void* args, void*, void*) {
    auto* work = mods::arg<JPAEmitterWorkData*>(args, 1);
    auto* shape = mods::arg<JPABaseShape*>(args, 0);
    if (work == nullptr || shape == nullptr || !owns_dark_flame(work->mpEmtr)) return;
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVSRCALPHA, GX_LO_COPY);
    GXSetZMode(shape->getZEnable(), shape->getZCmp(), GX_FALSE);
}

void after_flame_resource_draw(ModContext*, void* args, void*, void*) {
    auto* emitter = mods::arg<JPABaseEmitter*>(args, 2);
    auto* resource = mods::arg<JPAResource*>(args, 0);
    if (resource == nullptr || !owns_dark_flame(emitter)) return;
    // Restore the native state this draw would have left behind. Never modify
    // shared JPA resources: ordinary FIRE02 emitters retain their original fire.
    auto* shape = resource->getBsp();
    if (shape == nullptr) return;
    GXSetBlendMode(shape->getBlendMode(), shape->getBlendSrc(),
                   shape->getBlendDst(), shape->getLogicOp());
    GXSetZMode(shape->getZEnable(), shape->getZCmp(), shape->getZUpd());
}

// Draw into the native minimap render pass. Group 4 (Tears of Light) uses
// texture 4 and palette entries 132..139 in drawTreasureAfterPlayer(). Do not
// insert fake treasure records or change compass/story/light-drop flags.
void after_elite_minimap(ModContext*, void* args, void*, void*) {
    auto* map=mods::arg<renderingAmap_c*>(args,0);
    if (!map || !in_arena() || transitioning() ||
        (s_run.phase!=Phase::Running && s_run.phase!=Phase::Upgrade &&
         s_run.phase!=Phase::TreasureChoice && s_run.phase!=Phase::Treasure)) return;
    auto* texture=dMpath_n::m_texObjAgg.getTexObjPointer(4);
    if (!texture || !map->isRendIcon() || map->mRoomNoSingle<0) return;
    std::array<cXyz,elite_capacity> positions{};
    unsigned count=0;
    for (const auto& enemy : s_enemies) {
        if (enemy.id==kNone || !enemy.elite || !enemy.initialized || enemy.credited || enemy.retiring) continue;
        auto* actor=actor_by_id(enemy.id);
        if (!actor || actor->health<=0 || fopAcM_GetRoomNo(actor)!=arena().room) continue;
        BE(Vec) position=actor->current.pos;
        // Match Link's minimap coordinates, including room rotation/offset.
        dMapInfo_n::correctionOriginPos(arena().room,&position);
        if (map->isDrawAreaCheck(position) && count<positions.size()) positions[count++]=position;
    }
    if (!count) return;
    static const u8 texcoords[8]{0,1,1,1,1,0,0,0};
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XY,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_U8,0);
    GXSETARRAY(GX_VA_TEX0,texcoords,sizeof(texcoords),2,true);
    map->setTevSettingIntensityTextureToCI();
    GXInvalidateTexAll();
    GXLoadTexObj(texture,GX_TEXMAP0);
    GXSetTevColor(GX_TEVREG1,GXColor{132,0,0,0});
    GXSetTevColor(GX_TEVREG2,GXColor{136,0,0,0});
    const float size=map->getIconSize(4)*map->getCmPerTexel();
    for (unsigned i=0;i<count;++i) map->drawIconSingle(positions[i],size,size);
    map->setTevSettingNonTextureDirectColor();
    return;
}

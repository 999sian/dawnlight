// Reuse the native Poe (1,4) and Letters (3,4) cells. Never grant story flags,
// change Link's native equipment or retain archive textures across menu closes.
dMenu_Collect2D_c* s_gearCollection=nullptr;
std::array<u8,2> s_gearOriginalAvailability{};
std::array<bool,2> s_gearOriginalVisibility{};
std::array<J2DPane*,2> s_gearNativePanes{};
constexpr std::array<unsigned,2> kGearColumns{1,3};
void restore_gear_collection() {
    if(s_gearCollection)for(unsigned i=0;i<2;++i) {
        s_gearCollection->field_0x22d[kGearColumns[i]][4]=s_gearOriginalAvailability[i];
        if(auto* pane=s_gearNativePanes[i]) {if(s_gearOriginalVisibility[i])pane->show();else pane->hide();}
    }
    s_gearCollection=nullptr;s_gearNativePanes={};
}
GearItem collection_gear(unsigned i) {return s_gear.equipped(i==0?GearSlot::Weapon:GearSlot::Tunic);}
void gear_collection_slots(dMenu_Collect2D_c* menu) {
    if(!in_arena() || !menu || !menu->mpScreen)return;
    if(s_gearCollection!=menu) {
        restore_gear_collection();s_gearCollection=menu;
        for(unsigned i=0;i<2;++i) {
            s_gearOriginalAvailability[i]=menu->field_0x22d[kGearColumns[i]][4];
            s_gearNativePanes[i]=menu->mpScreen->search(i==0?MULTI_CHAR('item_2_n'):MULTI_CHAR('lett_4_n'));
            s_gearOriginalVisibility[i]=s_gearNativePanes[i]&&s_gearNativePanes[i]->isVisible();
        }
    }
    for(unsigned i=0;i<2;++i) {
        menu->field_0x22d[kGearColumns[i]][4]=collection_gear(i).kind!=GearKind::Empty;
        if(s_gearNativePanes[i])s_gearNativePanes[i]->show(); // Native pointer hit-testing uses the authored cells.
    }
}
void gear_collection_text(dMenu_Collect2D_c* menu) {
    if(!menu || menu!=s_gearCollection || menu->mCursorY!=4 || (menu->mCursorX!=1&&menu->mCursorX!=3))return;
    const auto item=collection_gear(menu->mCursorX==1?0:1);
    const auto name=item.kind==GearKind::Empty?std::string("No equipment"):std::string(rarity_names[item.rarity()])+" "+gear_name(item);
    const auto detail=item.kind==GearKind::Empty?std::string("Equip an item at the large chest in the Survivors hub."):gear_description(item);
    const auto write=[&](u64 tag,const char* value){
        auto* pane=menu->mpScreen->search(tag);if(!pane||pane->getTypeID()!=19)return;
        const auto buffer=static_cast<J2DTextBox*>(pane)->getStringPtr();
        if(buffer.buffer&&buffer.size)std::snprintf(buffer.buffer,buffer.size,"%s",value);
    };
    for(u64 tag:{MULTI_CHAR('item_n00'),MULTI_CHAR('item_n01'),MULTI_CHAR('item_n02'),MULTI_CHAR('item_n03'),
        MULTI_CHAR('item_n04'),MULTI_CHAR('item_n05'),MULTI_CHAR('item_n06'),MULTI_CHAR('item_n07')})write(tag,name.c_str());
    for(u64 tag:{MULTI_CHAR('i_text0'),MULTI_CHAR('i_text1'),MULTI_CHAR('f_text0'),MULTI_CHAR('f_text1')})write(tag,detail.c_str());
    menu->mItemNameString=0;menu->setAButtonString(0);
}
void after_gear_collection_screen(ModContext*,void* args,void*,void*) {gear_collection_slots(mods::arg<dMenu_Collect2D_c*>(args,0));}
HookAction before_gear_collection_draw(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);gear_collection_slots(menu);
    if(menu!=s_gearCollection)return HOOK_CONTINUE;
    gear_collection_text(menu);
    for(auto* pane:s_gearNativePanes)if(pane)pane->hide();
    return HOOK_CONTINUE;
}
// Native ItemIcon archive indices from dItem_data / dMeter2Info.
unsigned gear_collection_texture(GearItem item) {
    switch(item.family()) {
    case GearFamily::Sword:return 0x70;
    case GearFamily::Bow:return 0x69;
    case GearFamily::Boomerang:return 0x62;
    case GearFamily::Bombs:return 0x0c;
    case GearFamily::Ball:return 0x20;
    default:return 0x6b;
    }
}
void after_gear_collection_draw(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);
    if(!in_arena() || menu!=s_gearCollection || !menu->mpLinkPm)return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf)return;
    graf->setup2D();
    for(unsigned i=0;i<2;++i) {
        auto* pane=s_gearNativePanes[i];if(!pane)continue;
        pane->show();
        if(collection_gear(i).kind==GearKind::Empty)continue;
        auto* archive=dComIfGp_getItemIconArchive();
        const auto* texture=archive?static_cast<ResTIMG*>(archive->getIdxResource(gear_collection_texture(collection_gear(i)))):nullptr;
        if(!texture)continue;
        Mtx matrix;const auto a=menu->mpLinkPm->getGlobalVtx(pane,&matrix,0,false,0);
        const auto b=menu->mpLinkPm->getGlobalVtx(pane,&matrix,3,false,0);
        const float size=std::min(std::abs(b.x-a.x),std::abs(b.y-a.y));
        J2DPicture icon(texture);icon.setAlpha(menu->mpLinkPm->getAlpha());
        icon.draw((a.x+b.x-size)*.5f,(a.y+b.y-size)*.5f,size,size,false,false,false);
    }
    graf->setup2D();
}
void after_gear_collection_action(ModContext*,void* args,void*,void*) {
    auto* menu=mods::arg<dMenu_Collect2D_c*>(args,0);gear_collection_slots(menu);
    if(menu!=s_gearCollection)return;
    if(menu->mCursorY==4&&(menu->mCursorX==1||menu->mCursorX==3)) {
        // Both controller A and pointerActivateCurrent may request Letters.
        if(menu->mSubWindowOpenCheck==3)menu->mSubWindowOpenCheck=0;
        gear_collection_text(menu);
    }
}
HookAction before_gear_collection_delete(ModContext*,void* args,void*,void*) {
    if(mods::arg<dMenu_Collect2D_c*>(args,0)==s_gearCollection)restore_gear_collection();
    return HOOK_CONTINUE;
}

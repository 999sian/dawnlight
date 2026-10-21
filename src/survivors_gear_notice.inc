// ItemIcon textures belong to the host. Resolve them per draw, never retain
// archive pointers in the notification queue or modify native inventory panes.
unsigned gear_collection_texture(GearItem item);
void draw_gear_notice(bool preview=false) {
    if(!preview&&!s_gearNoticeTicks)return;
    auto* graf=dComIfGp_getCurrentGrafPort();auto* font=mDoExt_getMesgFont();
    if(!graf || !font)return;
    const auto item=preview?GearItem{GearKind::Sword,4,4,4}:s_gearNoticeItem;
    const bool acquired=item.kind!=GearKind::Empty;
    const float screenWidth=mDoGph_gInf_c::getWidthF(),screenHeight=mDoGph_gInf_c::getHeightF();
    const auto position=hud_position(HudPart::ItemBanner,mDoGph_gInf_c::getMinXF()+screenWidth*.5f);
    const float scale=position.scale*std::min({1.f,screenWidth/608.f,screenHeight/448.f});
    const float width=280*scale,height=(acquired?106:62)*scale,margin=8*scale;
    const unsigned duration=8*ticks_per_second;
    const float enter=preview?1.f:std::clamp(float(duration-s_gearNoticeTicks)/12.f,0.f,1.f);
    const float leave=preview?1.f:std::clamp(float(s_gearNoticeTicks)/12.f,0.f,1.f);
    const auto ease=[](float t){return t*t*(3-2*t);};
    const float targetLeft=position.x-width*.5f;
    const float travel=std::max(width+margin,mDoGph_gInf_c::getMinXF()+screenWidth-targetLeft);
    const float left=targetLeft+travel*(1-ease(std::min(enter,leave)));
    const float top=position.y-height*.5f;
    const auto alpha=u8(255*std::min(enter,leave));
    // Use the same rarity palette as equipment cards; tint the entire banner.
    unsigned rgb=0xffad54;
    if(acquired)std::sscanf(rarity_colors[item.rarity()]+1,"%x",&rgb);
    const unsigned red=(rgb>>16)&255,green=(rgb>>8)&255,blue=rgb&255;
    const auto oldTL=graf->mColorTL,oldTR=graf->mColorTR,oldBR=graf->mColorBR,oldBL=graf->mColorBL;
    graf->setup2D();
    graf->setColor(JUtility::TColor(red,green,blue,alpha));
    graf->fillBox(JGeometry::TBox2<f32>(left,top,left+width,top+height));
    graf->setColor(JUtility::TColor(red/5,green/5,blue/5,u8(alpha*.94f)));
    graf->fillBox(JGeometry::TBox2<f32>(left+3*scale,top+2*scale,left+width-2*scale,top+height-2*scale));
    graf->setColor(oldTL,oldTR,oldBR,oldBL);
    if(acquired) {
        auto* archive=dComIfGp_getItemIconArchive();
        const auto* texture=archive?static_cast<ResTIMG*>(archive->getIdxResource(gear_collection_texture(item))):nullptr;
        if(texture) {
            J2DPicture icon(texture);icon.setAlpha(alpha);
            icon.draw(left+10*scale,top+(height-44*scale)*.5f,44*scale,44*scale,false,false,false);
        }
    }
    const float textX=left+(acquired?64:12)*scale,textWidth=width-(acquired?76:24)*scale;
    const auto text=[&](const char* value,float y,float size,JUtility::TColor color) {
        J2DPrint print(font,color,color);print.setFontSize(size*scale,size*scale);
        print.printReturn(value,textWidth,18*scale,HBIND_LEFT,VBIND_TOP,textX,top+y*scale,alpha);
    };
    const auto white=JUtility::TColor(255,255,255,255),color=JUtility::TColor(red,green,blue,255);
    text(preview?"Weapon acquired":s_gearNotice,9,10,color);
    if(acquired) {
        text(item.definition().name,25,13,white);
        float y=45;
        for(const auto& affix:item.definition().affixes)if(affix.stat!=GearStat::None) {
            const auto line="+"+number(item.bonus(affix.stat))+(affix.percent?"% ":" ")+affix.label;
            text(line.c_str(),y,10,white);y+=16;
        }
        if(preview||gear_notice_equip_available()) {
            J2DPrint prompt(font,white,white);prompt.setFontSize(11*scale,11*scale);
            prompt.printReturn("Call Midna to Equip",width,18*scale,HBIND_CENTER,VBIND_TOP,
                left,top+height+5*scale,alpha);
        }
    }
    graf->setup2D(); // Restore untextured GX state for the remaining HUD.
}

void draw_next_chest(dMeter2Draw_c* meter, float center, float offset) {
    const auto p = hud_position(HudPart::Info, center);
    const float size = 14*p.scale, y = p.y + offset*p.scale;
    char text[96];
    if (chest_keys()) {
        std::snprintf(text, sizeof(text), "Next chest: 1 key  (Keys: %u)", chest_keys());
        label(text, p.x, y, size, 550*p.scale);
        return;
    }
    std::snprintf(text, sizeof(text), "Next chest: %u", s_run.chest_cost());
    auto* font = mDoExt_getMesgFont();
    auto* graf = dComIfGp_getCurrentGrafPort();
    auto* pane = meter && meter->mpScreen ? meter->mpScreen->search(MULTI_CHAR('rupi')) : nullptr;
    auto* source = pane ? static_cast<J2DPicture*>(pane) : nullptr;
    auto* texture = source ? source->getTexture(0) : nullptr;
    if (!font || !graf || !texture || !texture->getTexInfo() || !texture->getHeight()) {
        label(text, p.x, y, size, 550*p.scale);
        return;
    }
    // Measure using the same native layout as label(), so text and icon remain
    // one centered group at every HUD-editor scale. Never modify the HUD pane
    // or keep its archive-backed texture across frames/menu transitions.
    J2DPrint measure(font, JUtility::TColor(255,255,255,255), JUtility::TColor(255,255,255,255));
    measure.setFontSize(size, size); measure.initchar();
    J2DPrint::TSize bounds{};
    measure.parse(reinterpret_cast<const u8*>(text), int(std::strlen(text)), 32767, nullptr, bounds, 255, false);
    const float height = 18*p.scale;
    const float width = height*float(texture->getWidth())/float(texture->getHeight());
    const float gap = 4*p.scale, left = p.x - (bounds.field_0x0 + gap + width)*.5f;
    label(text, left + bounds.field_0x0*.5f, y, size, 550*p.scale);
    graf->setup2D();
    J2DPicture icon(texture->getTexInfo());
    icon.setBlackWhite(source->getBlack(), source->getWhite());
    icon.draw(left + bounds.field_0x0 + gap, y - 2*p.scale, width, height, false, false, false);
    graf->setup2D();
}

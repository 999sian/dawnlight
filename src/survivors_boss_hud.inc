// A single stable encounter target: timed bosses take precedence over awakened
// chest guards. Pending, sleeping, retired and defeated actors never get a bar.
ActorId s_bossHudTarget = kNone;

void draw_run_rank_hud(float x) {
    auto* font=mDoExt_getRubyFont();auto* graf=dComIfGp_getCurrentGrafPort();
    if(!font || !graf)return;
    char text[96];
    if(s_run.endless)std::snprintf(text,sizeof(text),"%02u:%02u  Endless",s_run.ticks/1800,s_run.ticks/30%60);
    else std::snprintf(text,sizeof(text),"%02u:%02u / %02u:00",s_run.ticks/1800,s_run.ticks/30%60,s_run.run_duration()/1800);
    J2DPrint clock(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    clock.setFontSize(18,18);clock.printReturn(text,230,26,HBIND_CENTER,VBIND_TOP,x-150,40,255);
    // A small skull silhouette avoids relying on a missing Unicode glyph in RubyFont.
    graf->setup2D();
    const auto tl=graf->mColorTL,tr=graf->mColorTR,bl=graf->mColorBL,br=graf->mColorBR;
    graf->setColor(JUtility::TColor(255,240,210,255));
    graf->fillBox(JGeometry::TBox2<f32>(x+90,42,x+104,52));
    graf->fillBox(JGeometry::TBox2<f32>(x+93,52,x+101,57));
    graf->setColor(JUtility::TColor(12,25,32,255));
    graf->fillBox(JGeometry::TBox2<f32>(x+92,46,x+96,50));
    graf->fillBox(JGeometry::TBox2<f32>(x+98,46,x+102,50));
    graf->fillBox(JGeometry::TBox2<f32>(x+96,52,x+98,55));
    graf->setColor(tl,tr,br,bl);
    std::snprintf(text,sizeof(text),"%u",s_run.twilit_rank());
    J2DPrint rank(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    rank.setFontSize(18,18);rank.printReturn(text,45,26,HBIND_CENTER,VBIND_TOP,x+108,40,255);
}

const char* boss_hud_name(unsigned species) {
    constexpr const char* names[] = {
        "Bokoblin", "Keese", "Rat", "Bulblin", "Bulblin Archer", "Tektite",
        "Lizalfos", "Helmasaur", "Helmasaurus", "Guay", "Kargarok", "Bomskit",
        "Stalchild", "Gibdo", "Stalfos", "Puppet", "Bubble", "Fire Bubble",
        "Black Chu", "Dynalfos", "Aeralfos", "Darknut", "Dodongo", "Stalhound",
        "Shadow Bulblin", "Shadow Bulblin Archer", "Shadow Beast", "Shadow Keese",
        "Twilit Carrier Kargarok", "Stallord", "Zant's Hand", "King Bulblin",
        "Death Sword", "Twilit Bloat", "Phantom Zant", "Darkhammer", "Puppet Zelda", "Ganondorf",
    };
    static_assert(std::size(names) == species_count);
    return species < species_count ? names[species] : "Elite";
}

Enemy* boss_hud_target() {
    auto* link = daAlink_getAlinkActorClass();
    Enemy* best = nullptr;
    float nearest = 0.0f;
    if (link) for (auto& enemy : s_enemies) {
        if (enemy.id == kNone || !enemy.elite || !enemy.initialized ||
            enemy.dormant || enemy.retiring || enemy.credited) continue;
        auto* actor = actor_by_id(enemy.id);
        if (!actor || actor->health <= 0 || actor->field_0x560 <= 0) continue;
        const float distance = actor->current.pos.abs2(link->current.pos);
        const bool timed = enemy.eliteSlot >= 0;
        const bool bestTimed = best && best->eliteSlot >= 0;
        if (!best || (timed && !bestTimed) || (timed == bestTimed &&
            (enemy.id == s_bossHudTarget || (best->id != s_bossHudTarget && distance < nearest)))) {
            best = &enemy;
            nearest = distance;
        }
    }
    s_bossHudTarget = best ? best->id : kNone;
    return best;
}

bool draw_boss_hud(dMeter2Draw_c* meter, float x) {
    auto* enemy = boss_hud_target();
    if (!enemy) return false;
    auto* actor = actor_by_id(enemy->id);
    if (!actor) return false;
    const float percentage = 100.0f * float(actor->health) / float(actor->field_0x560);
    if (!draw_boss_health_meter(meter, percentage, x - 160, 120, 320, 24)) return false;
    // Native MESSAGE_BOX_BOSS_NAME (also used by Hero's Shade) uses RubyFont.
    if (auto* font = mDoExt_getRubyFont()) {
        const char* name = boss_hud_name(enemy->species);
        J2DPrint shadow(font, JUtility::TColor(0,0,0,230), JUtility::TColor(0,0,0,230));
        shadow.setFontSize(20,20);
        shadow.printReturn(name,550,28,HBIND_CENTER,VBIND_TOP,x-274,95,255);
        J2DPrint print(font, JUtility::TColor(255,240,210,255), JUtility::TColor(230,190,150,255));
        print.setFontSize(20,20);
        print.printReturn(name,550,28,HBIND_CENTER,VBIND_TOP,x-275,94,255);
    }
    return true;
}

void draw_victory_hud(float x) {
    auto* font=mDoExt_getRubyFont();
    if (!font) return;
    J2DPrint shadow(font,JUtility::TColor(0,0,0,230),JUtility::TColor(0,0,0,230));
    shadow.setFontSize(34,34);
    shadow.printReturn("Victory",550,44,HBIND_CENTER,VBIND_TOP,x-274,85,255);
    J2DPrint title(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    title.setFontSize(34,34);
    title.printReturn("Victory",550,44,HBIND_CENTER,VBIND_TOP,x-275,84,255);
    const unsigned remaining=s_victoryTicks>=10*ticks_per_second ? 0 :
        (10*ticks_per_second-s_victoryTicks+ticks_per_second-1)/ticks_per_second;
    char text[16]; std::snprintf(text,sizeof(text),"%u",remaining);
    J2DPrint timer(font,JUtility::TColor(255,65,55,255),JUtility::TColor(220,25,20,255));
    timer.setFontSize(25,25);
    timer.printReturn(text,550,34,HBIND_CENTER,VBIND_TOP,x-275,128,255);
}

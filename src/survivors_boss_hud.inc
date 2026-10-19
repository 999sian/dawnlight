// A single stable encounter target: timed bosses take precedence over awakened
// chest guards. Pending, sleeping, retired and defeated actors never get a bar.
ActorId s_bossHudTarget = kNone;

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

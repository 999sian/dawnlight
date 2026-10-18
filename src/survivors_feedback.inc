enum class CombatText { Block, Dodge, Crit };
struct FloatingCombatText {
    cXyz position{};
    unsigned remaining=0;
    CombatText kind=CombatText::Crit;
};
constexpr unsigned combat_text_lifetime=30;
std::array<FloatingCombatText,64> s_combatTexts{};
struct DamageFeedback {
    void* args;
    fopAc_ac_c* actor;
    dCcU_AtInfo* info;
    int health;
    bool critical=false;
};
std::vector<DamageFeedback> s_damageFeedback;
void clear_combat_feedback() { s_combatTexts={}; s_damageFeedback.clear(); }
void combat_feedback(fopAc_ac_c* actor,CombatText kind) {
    if (!actor || !in_arena() || s_run.phase!=Phase::Running) return;
    auto slot=std::min_element(s_combatTexts.begin(),s_combatTexts.end(),
        [](const auto& a,const auto& b){return a.remaining<b.remaining;});
    const float height=std::max(100.f,std::clamp(actor->eyePos.y-actor->current.pos.y,0.f,400.f))+35.f;
    *slot={actor->current.pos+cXyz(0,height,0),combat_text_lifetime,kind};
}
void update_combat_feedback() {
    for (auto& text:s_combatTexts) if (text.remaining) { --text.remaining; text.position.y+=1.5f; }
}
void begin_damage_feedback(void* args,fopAc_ac_c* actor,dCcU_AtInfo* info) {
    s_damageFeedback.push_back({args,actor,info,actor->health});
}
void mark_damage_critical(dCcU_AtInfo* info,bool critical) {
    if (!s_damageFeedback.empty() && s_damageFeedback.back().info==info)
        s_damageFeedback.back().critical=critical;
}
void end_damage_feedback(void* args) {
    if (s_damageFeedback.empty() || s_damageFeedback.back().args!=args) return;
    const auto hit=s_damageFeedback.back();s_damageFeedback.pop_back();
    if (hit.critical && hit.info->mAttackPower && hit.actor->health<hit.health)
        combat_feedback(hit.actor,CombatText::Crit);
}

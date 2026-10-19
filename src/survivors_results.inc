// Run-local results; the persisted records continue to store scores/progression.
constexpr const char* kResultsStyle=R"(
window { background-color: #0c1210f5; }
window content pane.survivors-results-main { flex: 1 1 69%; padding: 20dp; gap: 12dp; overflow: hidden auto; }
window content pane.survivors-results-side { flex: 0 0 31%; padding: 20dp; gap: 12dp; overflow: hidden auto; background-color: #19221aee; }
.sr-body { display: block; width: 100%; flex-shrink: 0; }
.sr-integrity { font-size: 13dp; color: #ff4545; margin-bottom: 6dp; }
.sr-heading { font-family: var(--font-family-heading); font-size: 32dp; color: #ffdc75; }
.sr-subtitle { font-size: 17dp; color: #c4cebd; margin-bottom: 14dp; }
.sr-metrics { display: flex; flex-wrap: wrap; gap: 8dp; margin-bottom: 18dp; }
.sr-metric { flex: 1 1 28%; padding: 10dp; background-color: #263128; border-radius: 6dp; }
.sr-value { display: block; font-size: 24dp; color: #fff2ca; }
.sr-label { display: block; font-size: 13dp; color: #c4cebd; }
.sr-section { font-size: 19dp; color: #ffdc75; margin: 12dp 0dp 8dp; }
.sr-row { display: flex; align-items: center; padding: 8dp 0dp; border-bottom: 1dp #ffffff16; font-size: 15dp; }
.sr-row > div { box-sizing: border-box; min-width: 0dp; }
.sr-skill { flex: 0 0 39%; padding-right: 8dp; }
.sr-level { flex: 0 0 8%; text-align: right; }
.sr-damage { flex: 0 0 22%; text-align: right; color: #fff2ca; }
.sr-dps { flex: 0 0 17%; text-align: right; }
.sr-share { flex: 0 0 14%; text-align: right; }
.sr-columns { color: #aebda8; font-size: 13dp; }
.sr-total { color: #ffdc75; border-top: 2dp #ffdc7566; font-size: 17dp; }
.sr-track { height: 3dp; margin-top: 5dp; background-color: #ffffff12; }
.sr-bar { height: 3dp; background-color: #d8bc66; }
.sr-note { font-size: 12dp; color: #aebda8; margin-top: 12dp; }
.sr-stat { display: flex; justify-content: space-between; gap: 8dp; margin-bottom: 7dp; font-size: 15dp; }
.sr-equipment { display: flex; align-items: center; gap: 10dp; padding: 7dp 0dp; }
.sr-equipment img { width: 36dp; height: 36dp; object-fit: contain; }
.sr-actions { margin-top: auto; gap: 12dp; }
.sr-actions button { flex: 1 1 0; padding: 14dp; }
@media (max-width: 800dp) {
 window content pane.survivors-results-main { padding: 10dp; flex-basis: 68%; }
 window content pane.survivors-results-side { padding: 10dp; flex-basis: 32%; }
 .sr-heading { font-size: 24dp; }
 .sr-row, .sr-stat { font-size: 12dp; }
 .sr-columns, .sr-label, .sr-note { font-size: 11dp; }
 .sr-value { font-size: 19dp; }
 .sr-equipment img { width: 26dp; height: 26dp; }
}
)";
std::string result_number(uint64_t n) {
    auto text=std::to_string(n);
    for(int i=int(text.size())-3;i>0;i-=3)text.insert(size_t(i),",");
    return text;
}
std::string result_decimal(double value,int places=1) {
    char text[48];std::snprintf(text,sizeof(text),"%.*f",places,value);return text;
}
std::string result_time(unsigned ticks) {
    char text[32];std::snprintf(text,sizeof(text),"%02u:%02u",ticks/1800,ticks/30%60);return text;
}
unsigned result_source_level(DamageSource source) {
    switch(source) {
    case DamageSource::DarkLink:return s_run.dark_link;
    case DamageSource::Sword:return s_run.master_sword? s_run.master_sword:1;
    case DamageSource::DualWield:return s_run.dual_wield;
    case DamageSource::FireArrows:return s_run.rank(Upgrade::FireArrows);
    case DamageSource::FireBomb:return s_run.rank(Upgrade::FireBomb);
    case DamageSource::FireEdge:return s_run.rank(Upgrade::FireEdge);
    case DamageSource::DarkAura:return s_run.rank(Upgrade::DarkAura);
    default:
        for(unsigned i=0;i<unsigned(Upgrade::Rupees);++i)
            if(weapon_damage_source(i)==source && source!=DamageSource::Other)return s_run.rank(Upgrade(i));
        return 0;
    }
}
std::string result_source_name(DamageSource source) {
    if(source==DamageSource::Spin)return s_run.upgrade_name(Upgrade::Spin);
    return damage_source_names[unsigned(source)];
}
DamageAmounts result_damage() {
    auto damage=s_run.damage.dealt;
    for(auto pair:std::array{std::pair{DamageSource::DualWield,DamageSource::Sword},
        std::pair{DamageSource::FireArrows,DamageSource::Bow},std::pair{DamageSource::FireBomb,DamageSource::Bombs},
        std::pair{DamageSource::BombArrow,DamageSource::Bow},std::pair{DamageSource::FireEdge,DamageSource::Sword},
        std::pair{DamageSource::DarkAura,DamageSource::DarkLink}}) {
        damage[unsigned(pair.second)]+=damage[unsigned(pair.first)];damage[unsigned(pair.first)]=0;
    }
    return damage;
}
std::string results_damage_rml() {
    const auto damageBySource=result_damage();
    const auto total=s_run.damage.total();
    const double seconds=std::max(1u,s_run.ticks)/double(ticks_per_second);
    std::string rml="<div class=\"sr-heading\">"+std::string(s_run.phase==Phase::Victory?"VICTORY":"DEFEAT")+"</div>";
    if(s_run.options.allow_cheats || s_run.modded) {
        const std::string labels=std::string(s_run.options.allow_cheats?"Cheated":"")+
            (s_run.options.allow_cheats && s_run.modded?" &bull; ":"")+(s_run.modded?"Modded":"");
        rml="<div class=\"sr-integrity\">"+labels+"</div>"+rml;
    }
    rml+="<div class=\"sr-subtitle\">"+treasure_rml_escape(arenas[s_map].name)+" &bull; "+result_time(s_run.ticks)+"</div>";
    rml+="<div class=\"sr-metrics\">";
    const auto metric=[&](const char* label,const std::string& value){
        rml+="<div class=\"sr-metric\"><span class=\"sr-value\">"+value+"</span><span class=\"sr-label\">"+label+"</span></div>";
    };
    metric("Total damage",result_number(total));metric("Run DPS",result_decimal(total/seconds));metric("Enemies defeated",result_number(s_run.kills));
    rml+="</div><div class=\"sr-section\">Damage breakdown</div>";
    rml+="<div class=\"sr-row sr-columns\"><div class=\"sr-skill\">Ability</div><div class=\"sr-level\">Lv.</div><div class=\"sr-damage\">Damage</div><div class=\"sr-dps\">DPS</div><div class=\"sr-share\">Share</div></div>";
    std::array<unsigned,damage_source_count> order{};std::iota(order.begin(),order.end(),0u);
    std::stable_sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return damageBySource[a]>damageBySource[b];});
    for(unsigned i:order) {
        const auto source=DamageSource(i);const auto damage=damageBySource[i];
        const unsigned rank=result_source_level(source);
        if(!damage)continue;
        const double share=total?100.0*damage/total:0;
        rml+="<div class=\"sr-row\"><div class=\"sr-skill\">"+treasure_rml_escape(result_source_name(source))+
            "<div class=\"sr-track\"><div class=\"sr-bar\" style=\"width: "+result_decimal(share)+"%;\"></div></div></div>";
        rml+="<div class=\"sr-level\">"+(rank?number(rank):"-")+"</div><div class=\"sr-damage\">"+result_number(damage)+
            "</div><div class=\"sr-dps\">"+result_decimal(damage/seconds)+"</div><div class=\"sr-share\">"+result_decimal(share)+"%</div></div>";
    }
    if(!total)rml+="<div class=\"sr-note\">No damage dealt this run.</div>";
    rml+="<div class=\"sr-row sr-total\"><div class=\"sr-skill\">Total</div><div class=\"sr-level\"></div><div class=\"sr-damage\">"+
        result_number(total)+"</div><div class=\"sr-dps\">"+result_decimal(total/seconds)+"</div><div class=\"sr-share\">"+(total?"100%":"0%")+"</div></div>";
    rml+="<div class=\"sr-note\">Damage is enemy HP actually removed; blocked hits and overkill are excluded. DPS uses the full run time, excluding pauses. Simultaneous hits share damage proportionally. Passive effects count toward their ability. Dark Link shows its extra multiplier damage and Dark Aura; this damage is excluded from the other rows.</div>";
    return rml;
}
std::string results_loadout_rml() {
    std::string rml="<div class=\"sr-section\">Run summary</div>";
    const auto stat=[&](const char* label,const std::string& value){rml+="<div class=\"sr-stat\"><span>"+std::string(label)+"</span><span>"+value+"</span></div>";};
    stat("Score",result_number(s_run.score()));stat("Map high score",result_number(s_records.high_scores[s_map]));
    stat("Difficulty","x"+result_decimal(s_run.options.score_percent()/100.0,2));
    stat("Level reached",number(s_run.level));stat("Timed elites",number(s_run.timed_elites)+" / 4");
    stat("Other elites",number(s_run.guard_elites));stat("Chests opened",number(s_run.chests_opened));
    stat("XP earned",result_number(s_run.score()));stat("Meta level",number(s_records.level()));
    stat("Unlock points",number(s_records.available_points()));
    rml+="<div class=\"sr-section\">Equipment</div>";
    for(unsigned i=0;i<unsigned(ChestReward::Count);++i) {
        const auto item=ChestReward(i);if(!s_run.owns(item))continue;
        char icon[48];std::snprintf(icon,sizeof(icon),"item://item/%02x",unsigned(treasure_item(item,s_run)));
        const auto name=item==ChestReward::OrdonSword?"Dual Wield":reward_name(item,s_run);
        rml+="<div class=\"sr-equipment\"><img src=\""+std::string(icon)+"\"/><span>"+treasure_rml_escape(name)+"</span></div>";
    }
    return rml;
}
ModResult build_results(UiElementHandle left,UiElementHandle right) {
    auto result=svc_ui->elem_set_class(mod_ctx,left,"survivors-results-main",true);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,right,"survivors-results-side",true);if(result!=MOD_OK)return result;
    UiElementHandle mainBody=0,sideBody=0;
    result=svc_ui->pane_add_rml(mod_ctx,left,results_damage_rml().c_str(),&mainBody);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,mainBody,"sr-body",true);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_rml(mod_ctx,right,results_loadout_rml().c_str(),&sideBody);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,sideBody,"sr-body",true);if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_text(mod_ctx,left,s_storeFailed?
        "Records could not be stored. Retry Save records before leaving.":
        "Save records before quitting. Return to hub keeps your progress and lets you prepare another run.",nullptr);
    if(result!=MOD_OK)return result;
    UiRowDesc rowDesc=UI_ROW_DESC_INIT;UiElementHandle row=0;
    result=svc_ui->pane_add_row(mod_ctx,left,&rowDesc,&row);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,row,"sr-actions",true);if(result!=MOD_OK)return result;
    result=button(row,"Save records",save_results);
    return result==MOD_OK?button(row,"Return to hub",replay):result;
}

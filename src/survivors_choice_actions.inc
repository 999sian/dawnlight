// Shared run-wide resources; rebuilding a choice window must not cancel a chest.
void refresh_choices() { s_refreshChoices=s_window; close_window(); }
void reroll_choices(ModContext*,void*) {
    if (!s_window) return;
    if (s_run.phase!=Phase::Upgrade && s_run.phase!=Phase::TreasureChoice) return;
    const bool chest=s_run.phase==Phase::TreasureChoice;
    if (chest && (s_selectedChest>=s_chests.size() || s_chosenTreasure!=ChestReward::Count)) return;
    unsigned rupees=dComIfGs_getRupee();
    const unsigned reserved=chest ? s_run.chest_price(s_chests[s_selectedChest].free,s_chests[s_selectedChest].fixed) : 0;
    if (!s_run.pay_reroll(rupees,reserved)) return;
    dComIfGs_setRupee(rupees);
    if (chest) {
        auto& loot=s_chests[s_selectedChest];
        loot.offers=s_run.roll_chest_offers(potion_slot()>=0);
        for(auto& rarity:loot.rarities)rarity=s_run.roll_rarity();
    } else s_run.roll_upgrades();
    refresh_choices();
}
void banish_choice(ModContext*,void* data) {
    if(!s_window)return;
    const unsigned choice=*static_cast<unsigned*>(data);
    if (choice>=3) return;
    if (s_run.phase==Phase::Upgrade) {
        const auto u=s_run.offers[choice];
        if (!s_run.banish(u)) return;
        for(auto& offer:s_run.offers)if(offer==u)offer=Upgrade::Count;
    } else if(s_run.phase==Phase::TreasureChoice && s_selectedChest<s_chests.size() && s_chosenTreasure==ChestReward::Count) {
        auto& offers=s_chests[s_selectedChest].offers;
        const auto reward=offers[choice];
        if (!s_run.banish(reward,potion_slot()>=0)) return;
        for(auto& offer:offers)if(offer==reward)offer=ChestReward::Count;
    } else return;
    refresh_choices();
}
void skip_empty_upgrade(ModContext*,void*) {
    if (s_run.phase!=Phase::Upgrade || std::any_of(s_run.offers.begin(),s_run.offers.end(),[](Upgrade u){return s_run.eligible(u);})) return;
    s_run.phase=Phase::Running;close_window();s_run.level_up();
}
bool empty_starting_choice() {
    return s_run.starting_item_pending&&s_run.phase==Phase::TreasureChoice&&s_selectedChest==0&&
        s_chosenTreasure==ChestReward::Count&&std::none_of(s_chests[0].offers.begin(),s_chests[0].offers.end(),
            [](ChestReward r){return s_run.chest_eligible(r,potion_slot()>=0);});
}
void skip_empty_starting_choice(ModContext*,void*) {
    if(!empty_starting_choice())return;
    finish_starting_item_choice();close_window();
}
ModResult add_choice_header(UiElementHandle pane) {
    UiRowDesc rowDesc=UI_ROW_DESC_INIT;
    UiElementHandle row=0,button=0,loadout=0;
    auto result=svc_ui->pane_add_row(mod_ctx,pane,&rowDesc,&row);
    if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,row,"survivors-choice-toolbar",true);
    if(result!=MOD_OK)return result;
    const std::string label=(s_run.infinite_rerolls() || s_run.free_rerolls) ? "Reroll: Free" : "Reroll: "+number(s_run.reroll_cost())+" rupees";
    UiControlDesc control=UI_CONTROL_DESC_INIT;control.label=label.c_str();control.on_pressed=reroll_choices;
    control.is_disabled=[](ModContext*,void*) {
        const bool chest=s_run.phase==Phase::TreasureChoice;
        const unsigned reserved=chest && s_selectedChest<s_chests.size() ? s_run.chest_price(s_chests[s_selectedChest].free,s_chests[s_selectedChest].fixed) : 0;
        return dComIfGs_getRupee()<reserved+s_run.reroll_cost();
    };
    result=svc_ui->pane_add_control(mod_ctx,row,&control,&button);if(result!=MOD_OK)return result;
    result=svc_ui->elem_set_class(mod_ctx,button,"survivors-reroll",true);if(result!=MOD_OK)return result;
    std::string equipment="<div class=\"survivors-equipment\">";
    unsigned slots=0;
    for(unsigned i=0;i<unsigned(ChestReward::Count) && slots<4;++i) {
        const auto reward=ChestReward(i);if(!s_run.owns(reward))continue;
        char icon[40];std::snprintf(icon,sizeof(icon),"item://item/%02x",unsigned(treasure_item(reward,s_run)));
        equipment+="<div class=\"survivors-equipment-slot\"><img src=\""+std::string(icon)+"\"/></div>";++slots;
    }
    for(;slots<4;++slots)equipment+="<div class=\"survivors-equipment-slot\"></div>";
    equipment+="</div>";
    // Material Symbols already ships with the host: refresh and block glyphs.
    equipment="<div class=\"survivors-choice-status\">"+equipment+
        "<div class=\"survivors-choice-counts\"><span><icon>&#xE5D5;</icon>x"+(s_run.infinite_rerolls()?std::string("∞"):number(s_run.free_rerolls))+
        "</span><span><icon class=\"survivors-banish-color\">&#xE14B;</icon>x"+(s_run.infinite_banishes()?std::string("∞"):number(s_run.banishes))+"</span></div></div>";
    result=svc_ui->pane_add_rml(mod_ctx,row,equipment.c_str(),&loadout);if(result!=MOD_OK)return result;
    return svc_ui->elem_set_class(mod_ctx,loadout,"survivors-choice-loadout",true);
}
ModResult add_choice_actions(UiElementHandle pane,bool chest) {
    if(chest&&empty_starting_choice()) {
        UiControlDesc control=UI_CONTROL_DESC_INIT;control.label="Continue";control.on_pressed=skip_empty_starting_choice;
        return svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);
    }
    if(chest || std::any_of(s_run.offers.begin(),s_run.offers.end(),[](Upgrade u){return s_run.eligible(u);}))return MOD_OK;
    UiControlDesc control=UI_CONTROL_DESC_INIT;control.label="Continue";control.on_pressed=skip_empty_upgrade;
    return svc_ui->pane_add_control(mod_ctx,pane,&control,nullptr);
}

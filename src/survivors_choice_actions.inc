// Shared run-wide resources; rebuilding a choice window must not cancel a chest.
void refresh_choices() { s_refreshChoices=s_window; close_window(); }
void reroll_choices(ModContext*,void*) {
    if (!s_window) return;
    if (s_run.phase!=Phase::Upgrade && s_run.phase!=Phase::TreasureChoice) return;
    const bool chest=s_run.phase==Phase::TreasureChoice;
    if (chest && (s_selectedChest>=s_chests.size() || s_chosenTreasure!=ChestReward::Count)) return;
    unsigned rupees=dComIfGs_getRupee();
    const unsigned reserved=chest && !s_chests[s_selectedChest].free ? s_run.chest_cost() : 0;
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
ModResult add_choice_actions(UiElementHandle pane,bool chest) {
    UiRowDesc rowDesc=UI_ROW_DESC_INIT;rowDesc.wrap=true;
    UiElementHandle row=0;auto result=svc_ui->pane_add_row(mod_ctx,pane,&rowDesc,&row);
    if(result!=MOD_OK)return result;
    const std::string label=s_run.free_rerolls ? "Reroll ("+number(s_run.free_rerolls)+" free)" : "Reroll ("+number(s_run.reroll_cost())+" rupees)";
    UiControlDesc control=UI_CONTROL_DESC_INIT;control.label=label.c_str();control.on_pressed=reroll_choices;
    control.is_disabled=[](ModContext*,void*) {
        const bool chest=s_run.phase==Phase::TreasureChoice;
        const unsigned reserved=chest && s_selectedChest<s_chests.size() && !s_chests[s_selectedChest].free ? s_run.chest_cost() : 0;
        return dComIfGs_getRupee()<reserved+s_run.reroll_cost();
    };
    result=svc_ui->pane_add_control(mod_ctx,row,&control,nullptr);if(result!=MOD_OK)return result;
    bool any=false;
    for(unsigned i=0;i<3;++i){
        const auto reward=chest ? s_chests[s_selectedChest].offers[i] : ChestReward::Count;
        const auto upgrade=chest ? Upgrade::Count : s_run.offers[i];
        const bool valid=chest ? s_run.chest_eligible(reward,potion_slot()>=0) : s_run.eligible(upgrade);
        any|=valid;if(!valid || !s_run.banishes)continue;
        const std::string name="Banish ("+number(s_run.banishes)+") "+std::string(chest ? reward_name(reward,s_run) : names[unsigned(upgrade)]);
        control=UI_CONTROL_DESC_INIT;control.label=name.c_str();control.on_pressed=banish_choice;control.user_data=&s_choices[i];
        result=svc_ui->pane_add_control(mod_ctx,row,&control,nullptr);if(result!=MOD_OK)return result;
    }
    if(!chest && !any) {
        control=UI_CONTROL_DESC_INIT;control.label="Continue";control.on_pressed=skip_empty_upgrade;
        result=svc_ui->pane_add_control(mod_ctx,row,&control,nullptr);
    }
    return result;
}

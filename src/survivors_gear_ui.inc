constexpr const char* kGearStyle=R"(
window { width: 96%; max-width: 1150dp; }
window content pane { padding: 12dp; font-size: 16dp; }
ui-row.gear-row { gap: 8dp; margin-bottom: 8dp; align-items: stretch; }
ui-row.gear-cell { display: block; position: relative; flex: 1 1 0; min-width: 0; height: 112dp; }
.gear-copy { pointer-events: none; text-align: center; padding: 6dp; font-size: 12dp; }
.gear-copy * { pointer-events: none; }
.gear-copy img { display: block; width: 44dp; height: 44dp; margin: 0 auto 4dp; }
button.gear-pick { position: absolute; left: 0; top: 0; width: 100%; height: 100%; padding: 0;
 color: transparent; font-size: 0; background-color: #14241a30; border: 2dp #748071; border-radius: 5dp; }
button.gear-pick:not(:disabled):hover, button.gear-pick:not(:disabled):focus-visible { border-color: #ffe28a; color: transparent; }
button.gear-pick:selected { border-color: #71ed9a; color: transparent; background-color: #50e08518; }
button.gear-banish { font-family: var(--font-family-icons); color: #ff7373; }
@media (max-width: 640dp) { window content pane { padding: 6dp; font-size: 13dp; }
ui-row.gear-row { gap: 4dp; } ui-row.gear-cell { height: 100dp; }
.gear-copy img { width: 32dp; height: 32dp; } .gear-copy { font-size: 10dp; padding: 3dp; } }
)";
std::array<unsigned,GearInventory::capacity> s_gearSlots{};
std::array<UiElementHandle,GearInventory::capacity> s_gearCards{},s_gearButtons{};
UiElementHandle s_gearDetails=0,s_gearHint=0;
bool s_gearBanish=false;
const char* gear_name(GearItem item) {return item.definition().name;}
const char* gear_icon(GearItem item) {
    switch(item.family()) {
    case GearFamily::Sword:return "28";
    case GearFamily::Bow:return "43";
    case GearFamily::Boomerang:return "40";
    case GearFamily::Bombs:return "70";
    case GearFamily::Ball:return "42";
    default:return "2f";
    }
}
std::string gear_description(GearItem item) {
    if(item.kind==GearKind::Empty)return "Empty slot";
    std::string text=std::string(rarity_names[item.rarity()])+" "+gear_name(item);
    for(const auto& affix:item.definition().affixes)if(affix.stat!=GearStat::None)
        text+="\n+"+number(item.bonus(affix.stat))+(affix.percent?"% ":" ")+affix.label;
    if(item.kind==GearKind::Tunic)text+="\nAdded on top of talent and level-up bonuses. Maximum hearts remain capped at 20.";
    if(item.is_weapon() && item.family()!=GearFamily::Sword)
        text+="\nActive after acquiring this weapon in the run. Equipment bonuses stack beyond level-up and item caps.";
    return text;
}
std::string gear_rml(GearItem item) {
    auto text=treasure_rml_escape(gear_description(item));size_t pos=0;
    while((pos=text.find('\n',pos))!=std::string::npos){text.replace(pos,1,"<br/>");pos+=5;}
    return text;
}
bool gear_editable() {
    return s_active && s_gameStarted && s_loaded && s_hub && s_run.phase==Phase::Ready &&
        !s_restart && !s_hubDeparture && !s_progressSaving && !save_modal_active() && !transitioning() && s_gearWritable;
}
void refresh_gear_ui() {
    for(unsigned i=0;i<s_gearCards.size();++i) {
        const auto item=s_gear.items[i];
        const bool equipped=s_gear.weapon==i||s_gear.tunic==i;
        std::string rml="<div class=\"gear-copy\">";
        if(item.kind!=GearKind::Empty)rml+="<img src=\"item://item/"+std::string(gear_icon(item))+"\"/><span style=\"color: "+rarity_colors[item.rarity()]+";\">"+gear_name(item)+"</span>";
        else rml+="<br/>Empty";
        rml+="<br/>"+std::string(equipped?"Equipped":"")+"</div>";
        if(s_gearCards[i])svc_ui->elem_set_rml(mod_ctx,s_gearCards[i],rml.c_str());
        if(s_gearButtons[i])svc_ui->control_set_tooltip(mod_ctx,s_gearButtons[i],gear_description(item).c_str());
    }
    if(s_gearHint)svc_ui->elem_set_rml(mod_ctx,s_gearHint,s_gearBanish?
        "<span style=\"color: #ff7373;\">Banish active: select an item to permanently delete it. Equipped items are also unequipped.</span>":
        "Select a slot and press A / Confirm to equip. One Weapon and one Tunic can be equipped. Banish deletes items. Changes save when you close this inventory.");
}
void select_gear(ModContext*,void* data) {
    if(!s_gearWindow || !gear_editable())return;
    const auto i=*static_cast<unsigned*>(data);if(i>=GearInventory::capacity)return;
    const auto item=s_gear.items[i];
    if(item.kind==GearKind::Empty){svc_ui->elem_set_rml(mod_ctx,s_gearDetails,"Empty slot");return;}
    auto next=s_gear;
    if(s_gearBanish)next.erase(i);else next.equip(i);
    if(!store_gear(next)) {
        svc_ui->elem_set_rml(mod_ctx,s_gearDetails,"Equipment could not be saved. Nothing changed. Select the slot to retry.");return;
    }
    const auto text=s_gearBanish?std::string("Item deleted."):gear_rml(item)+"<br/><br/>Equipped for your next run.";
    svc_ui->elem_set_rml(mod_ctx,s_gearDetails,text.c_str());
    refresh_gear_ui();
}
ModResult build_gear_window(ModContext*,UiWindowHandle,UiElementHandle left,UiElementHandle right,void*,ModError*) {
    s_gearCards={};s_gearButtons={};s_gearDetails=s_gearHint=0;
    auto result=svc_ui->pane_add_rml(mod_ctx,right,"Choose a slot to equip an item and view its stats.",&s_gearDetails);
    if(result!=MOD_OK)return result;
    result=svc_ui->pane_add_rml(mod_ctx,left,"",&s_gearHint);if(result!=MOD_OK)return result;
    for(unsigned row=0;row<4;++row) {
        UiRowDesc desc=UI_ROW_DESC_INIT;UiElementHandle group=0;
        result=svc_ui->pane_add_row(mod_ctx,left,&desc,&group);if(result!=MOD_OK)return result;
        svc_ui->elem_set_class(mod_ctx,group,"gear-row",true);
        for(unsigned col=0;col<5;++col) {
            const auto i=row*5+col;s_gearSlots[i]=i;UiElementHandle cell=0;
            result=svc_ui->pane_add_row(mod_ctx,group,&desc,&cell);if(result!=MOD_OK)return result;
            svc_ui->elem_set_class(mod_ctx,cell,"gear-cell",true);
            result=svc_ui->pane_add_rml(mod_ctx,cell,"",&s_gearCards[i]);if(result!=MOD_OK)return result;
            UiControlDesc control=UI_CONTROL_DESC_INIT;
            const auto name="Slot "+number(i+1);control.label=name.c_str();control.user_data=&s_gearSlots[i];control.on_pressed=select_gear;
            control.is_disabled=[](ModContext*,void*){return !gear_editable();};
            control.is_selected=[](ModContext*,void* data){const auto slot=*static_cast<unsigned*>(data);return s_gear.weapon==slot||s_gear.tunic==slot;};
            result=svc_ui->pane_add_control(mod_ctx,cell,&control,&s_gearButtons[i]);if(result!=MOD_OK)return result;
            svc_ui->elem_set_class(mod_ctx,s_gearButtons[i],"gear-pick",true);
        }
    }
    UiControlDesc banish=UI_CONTROL_DESC_INIT;banish.label="\uE14B";banish.tooltip="Banish: delete an item";
    banish.on_pressed=[](ModContext*,void*){if(gear_editable()){s_gearBanish=!s_gearBanish;refresh_gear_ui();}};
    banish.is_selected=[](ModContext*,void*){return s_gearBanish;};
    UiElementHandle handle=0;result=svc_ui->pane_add_control(mod_ctx,left,&banish,&handle);if(result!=MOD_OK)return result;
    svc_ui->elem_set_class(mod_ctx,handle,"gear-banish",true);
    refresh_gear_ui();return MOD_OK;
}
void close_gear_window() {
    const auto window=s_gearWindow;s_gearWindow=0;
    if(window)svc_ui->window_close(mod_ctx,window);
    s_gearCards={};s_gearButtons={};s_gearDetails=s_gearHint=0;s_gearBanish=false;
}
void open_gear_window() {
    if(s_gearWindow || !gear_editable() || ui_visible())return;
    s_gearBanish=false;
    UiTabDesc tab=UI_TAB_DESC_INIT;tab.title="Equipment - 20 slots";tab.build=build_gear_window;
    UiWindowDesc desc=UI_WINDOW_DESC_INIT;desc.tabs=&tab;desc.tab_count=1;desc.rcss=kGearStyle;
    desc.on_closed=[](ModContext*,UiWindowHandle handle,void*) {
        if(s_gearWindow!=handle)return;
        s_gearWindow=0;s_gearCards={};s_gearButtons={};s_gearDetails=s_gearHint=0;s_gearBanish=false;
        if(s_active&&s_gameStarted&&s_hub&&s_run.phase==Phase::Ready)s_hubSaveRequested=true;
    };
    if(svc_ui->window_push(mod_ctx,&desc,&s_gearWindow)!=MOD_OK)s_gearWindow=0;
}
bool hub_gear_interaction(daAlink_c* link) {
    if(!gear_editable() || !playable() ||
       !link || fopAcM_GetID(link)!=s_player || link->checkWolf())return false;
    cXyz chestAnchor;if(!survivors_equipment_chest_anchor(chestAnchor))return false;
    const cXyz delta=chestAnchor-link->current.pos;
    if(delta.absXZ()>210 || std::abs(delta.y)>110)return false;
    cXyz from=link->current.pos+cXyz(0,60,0),to=chestAnchor+cXyz(0,60,0);
    dBgS_LinChk wall;wall.Set(&from,&to,link);if(dComIfG_Bgsp().LineCross(&wall))return false;
    dComIfGp_setDoStatusForce(BUTTON_STATUS_OPEN,0);
    if(mDoCPd_c::getTrigA(PAD_1)) {
        open_gear_window();
        if(s_gearWindow)mDoCPd_c::getCpadInfo(PAD_1).mPressedButtonFlags&=~PAD_BUTTON_A;
    }
    return true;
}

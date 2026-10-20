// Intercept the native A/door request before an event starts. Once Start run is
// confirmed we replay that same event, and redirect only its final scene change.
HookAction before_hub_door(ModContext*,void* args,void* result,void*) {
    if(!in_arena())return HOOK_CONTINUE;
    auto* link=mods::arg<fopAc_ac_c*>(args,0);
    auto* door=mods::arg<fopAc_ac_c*>(args,1);
    if(s_allowHubDoor && s_hub && s_hubDeparture && door && fopAcM_GetID(door)==s_hubDoor)
        return HOOK_CONTINUE;
    if(s_hub && s_run.phase==Phase::Ready && !s_hubDeparture && !s_progressSaving &&
       link && door && fopAcM_GetID(link)==s_player && playable()) {
        s_hubDoor=fopAcM_GetID(door);
        s_hubDoorPriority=mods::arg<u16>(args,2);s_hubDoorFlags=mods::arg<u16>(args,3);
        s_hubMenu=HubMenu::Run;open_window();
    }
    *static_cast<s32*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
bool hub_chest_record(const stage_tresure_data_class& entry, bool largeOnly) {
    char name[9]{};std::memcpy(name,entry.name,8);
    const auto* info=dStage_searchName(name);
    if(!info || (info->procname!=fpcNm_TBOX_e && info->procname!=fpcNm_TBOX2_e))return false;
    const auto shape=(entry.base.parameters>>20)&0xf;
    if(largeOnly && shape!=1 && shape!=2)return false;
    s_gearChestPosition=entry.base.position;s_gearChestYaw=entry.base.angle.y;
    s_gearChestLocated=true;return true;
}
void locate_hub_gear_chest() {
    if(s_gearChestLocated || !s_hub || s_map!=0 || !in_arena())return;
    auto* room=dComIfGp_roomControl_getStatusRoomDt(arenas[0].room);
    auto* treasures=room?room->getTresure():nullptr;
    // getTresure() exposes ONLY common TRES. TRE0..TREe (and ACT layers) are
    // decoded separately; a chest on an inactive story layer never reaches
    // our actor-create hook either. Read their placement from the loaded room
    // archive and render our own chest, ignoring native switches/open flags.
    auto* file=static_cast<dStage_fileHeader*>(dComIfG_getStageRes(
        dComIfG_getRoomArcName(arenas[0].room),"room.dzr"));
    for(bool largeOnly : {true,false}) {
        if(treasures && treasures->m_entries)
            for(int i=0;i<treasures->num;++i)
                if(hub_chest_record(treasures->m_entries[i],largeOnly))return;
        if(!file)continue;
        for(int i=0;i<file->m_chunkCount;++i) {
            const auto& node=file->m_nodes[i];
            const auto* tag=reinterpret_cast<const char*>(&node.m_tag);
            const bool layer=(tag[3]>='0' && tag[3]<='9') || (tag[3]>='a' && tag[3]<='e');
            const bool treasure=std::memcmp(tag,"TRE",3)==0 && (tag[3]=='S' || layer);
            const bool actor=std::memcmp(tag,"ACT",3)==0 && (tag[3]=='R' || layer);
            if(!treasure && !actor)continue;
            // The native loader has relocated every chunk before the hub's
            // Ready phase. Keep OffsetPtr/BE conversion (Windows/ARM included).
            const auto* entries=static_cast<const stage_tresure_data_class*>(node.m_offset);
            if(!entries)continue;
            for(int j=0;j<node.m_entryNum;++j)
                if(hub_chest_record(entries[j],largeOnly))return;
        }
    }
}
void update_hub(daAlink_c* link) {
    locate_hub_gear_chest();
    if(s_gearChestLocated && s_run.phase==Phase::Ready && !s_hubDeparture && prepare_hub_loot()) {
        auto& chest=s_chests[kSmallChestCount];
        // Native treasure may start below/above the floor for an appearance
        // event. Our independent model never runs that event or its gravity.
        cXyz floor=s_gearChestPosition;floor.y=link->current.pos.y;
        if(dry_arena_floor(floor,1000,20000)) {
            if(!chest.active) {
                char message[128];std::snprintf(message,sizeof(message),
                    "Survivors: hub equipment chest at %.1f, %.1f, %.1f",
                    double(floor.x),double(floor.y),double(floor.z));
                svc_log->info(mod_ctx,message);
            }
            s_gearChestPosition=floor;
            chest.active=true;chest.free=true;chest.position=floor;chest.yaw=s_gearChestYaw;
        }
    }
    update_survivors_pedestal(s_run.phase==Phase::Ready && !s_hubDeparture);
    if(!s_hubDeparture || s_hubClosingWindow || ui_visible() || s_progressSaving || s_hubSaveRequested)return;
    if(s_hubDoorStarted) {
        // A rejected/aborted native event must never leave the hub locked. No
        // synthetic teleport: the next interaction can retry the real door.
        if(++s_hubDoorTicks>600 && !dComIfGp_event_runCheck()) {
            s_hubDeparture=s_hubDoorStarted=false;s_hubDoorTicks=0;
            svc_log->warn(mod_ctx,"Survivors: door event ended without a scene change; please retry");
        }
        return;
    }
    auto* door=actor_by_id(s_hubDoor);
    if(!door) {s_hubDeparture=false;s_hubDoorTicks=0;return;}
    // orderDoorEvent only enqueues a request. commonCheck can reject it later
    // if either actor has lost CANDOOR while the menu was open.
    if(link->eventInfo.checkCommandDoor() && door->eventInfo.checkCommandDoor()) {
        s_hubDoorStarted=true;s_hubDoorTicks=0;return;
    }
    if(!playable())return;
    if(++s_hubDoorTicks>90) {
        s_hubDeparture=false;s_hubDoorTicks=0;
        svc_log->warn(mod_ctx,"Survivors: door event was not accepted; please retry");
        return;
    }
    // Let native actor updates rebuild eligibility; never fabricate conditions
    // or mark an enqueued request as an accepted door animation.
    if(!link->eventInfo.chkCondition(dEvtCnd_CANDOOR_e) ||
       !door->eventInfo.chkCondition(dEvtCnd_CANDOOR_e))return;
    s_allowHubDoor=true;
    fopAcM_orderDoorEvent(link,door,s_hubDoorPriority,s_hubDoorFlags);
    s_allowHubDoor=false;
}
void* hub_exit_actor(void* ptr,void*) {
    if(!fopAcM_IsActor(ptr))return nullptr;
    auto* actor=static_cast<fopAc_ac_c*>(ptr);
    // Remembered interaction targets are authoritative, including modded doors.
    if(fopAcM_GetID(actor)==s_hubDoor)return ptr;
    const auto name=fopAcM_GetName(actor);
    if(name!=fpcNm_DOOR20_e && name!=fpcNm_KNOB20_e && name!=fpcNm_DBDOOR_e &&
       name!=fpcNm_BOSS_DOOR_e && name!=fpcNm_L1BOSS_DOOR_e &&
       name!=fpcNm_L1MBOSS_DOOR_e && name!=fpcNm_L5BOSS_DOOR_e && name!=fpcNm_SPIRAL_DOOR_e)return nullptr;
    const auto room=arenas[0].room;
    return fopAcM_GetRoomNo(actor)==room || fopAcM_GetRoomNo(actor)==-1 ||
        door_param2_c::getFRoomNo(actor)==room || door_param2_c::getBRoomNo(actor)==room ? ptr : nullptr;
}
void hub_label(const char* text,cXyz world) {
    auto* font=mDoExt_getRubyFont();
    if(!font || !dComIfGd_getView())return;
    const auto& matrix=*dComIfGd_getProjViewMtx();
    const float w=matrix[3][0]*world.x+matrix[3][1]*world.y+matrix[3][2]*world.z+matrix[3][3];
    if(w<=1)return;
    cXyz screen;mDoLib_project(&world,&screen);
    if(!std::isfinite(screen.x)||!std::isfinite(screen.y)||screen.x<mDoGph_gInf_c::getMinXF()||
       screen.x>mDoGph_gInf_c::getMaxXF()||screen.y<0||screen.y>mDoGph_gInf_c::getHeightF())return;
    const float size=std::clamp(24000.f/w,16.f,28.f);
    J2DPrint shadow(font,JUtility::TColor(0,0,0,230),JUtility::TColor(0,0,0,230));
    shadow.setFontSize(size,size);
    shadow.printReturn(text,550,size+8,HBIND_CENTER,VBIND_TOP,screen.x-274,screen.y+1,255);
    J2DPrint print(font,JUtility::TColor(255,240,210,255),JUtility::TColor(230,190,150,255));
    print.setFontSize(size,size);
    print.printReturn(text,550,size+8,HBIND_CENTER,VBIND_TOP,screen.x-275,screen.y,255);
}
void* draw_hub_exit_label(void* ptr,void*) {
    if(auto* door=static_cast<fopAc_ac_c*>(hub_exit_actor(ptr,nullptr)))
        hub_label("Start Run",door->current.pos+cXyz(0,400,0));
    return nullptr; // Visit every exit; the first actor may be off camera.
}
void draw_hub_labels() {
    if(!survivors_hub_active()||transitioning()||ui_visible()||dComIfGp_event_runCheck())return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf)return;
    graf->setup2D();
    cXyz anchor;
    if(survivors_pedestal_anchor(anchor))hub_label("Talents",anchor+cXyz(0,230,0));
    if(s_gearChestLocated)hub_label("Equipment",s_gearChestPosition+cXyz(0,200,0));
    fpcM_Search(draw_hub_exit_label,nullptr);
    graf->setup2D();
    if(s_storeFailed)label("Progress save failed. Retry Save progress at the sword.",
        mDoGph_gInf_c::getMinXF()+mDoGph_gInf_c::getWidthF()*.5f,40,16);
    graf->setup2D();
}

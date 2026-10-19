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
void update_hub(daAlink_c* link) {
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
    if(!playable())return;
    auto* door=actor_by_id(s_hubDoor);
    if(!door) {s_hubDeparture=false;return;}
    s_allowHubDoor=true;
    const auto ordered=fopAcM_orderDoorEvent(link,door,s_hubDoorPriority,s_hubDoorFlags);
    s_allowHubDoor=false;
    if(ordered) {s_hubDoorStarted=true;s_hubDoorTicks=0;}
    else if(++s_hubDoorTicks>60) {
        s_hubDeparture=false;s_hubDoorTicks=0;
        svc_log->warn(mod_ctx,"Survivors: door event could not start; please retry");
    }
}
void* hub_exit_actor(void* ptr,void*) {
    if(!fopAcM_IsActor(ptr))return nullptr;
    auto* actor=static_cast<fopAc_ac_c*>(ptr);
    if(fopAcM_GetRoomNo(actor)!=arenas[0].room && fopAcM_GetRoomNo(actor)!=-1)return nullptr;
    const auto name=fopAcM_GetName(actor);
    return name==fpcNm_DOOR20_e || name==fpcNm_DBDOOR_e || name==fpcNm_BOSS_DOOR_e ? ptr : nullptr;
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
void draw_hub_labels() {
    if(!survivors_hub_active()||transitioning()||ui_visible()||dComIfGp_event_runCheck())return;
    auto* graf=dComIfGp_getCurrentGrafPort();if(!graf)return;
    graf->setup2D();
    cXyz anchor;
    if(survivors_pedestal_anchor(anchor))hub_label("Talents",anchor+cXyz(0,230,0));
    if(auto* door=static_cast<fopAc_ac_c*>(fpcM_Search(hub_exit_actor,nullptr)))
        hub_label("Start Run",door->current.pos+cXyz(0,400,0));
    graf->setup2D();
    if(s_storeFailed)label("Progress save failed. Retry Save progress at the sword.",
        mDoGph_gInf_c::getMinXF()+mDoGph_gInf_c::getWidthF()*.5f,40,16);
    graf->setup2D();
}

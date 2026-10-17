// Arena-only stage edits. The user's story files and the native room archives
// are never patched. In particular, filtering happens before actor creation,
// not by deleting a boss after its opening event has already started.
std::vector<barriers::Wall> s_barriers;
std::vector<ActorId> s_arenaBackgrounds;
bool s_arenaMeshReady = false;
cXyz s_arenaSafePosition;
bool s_arenaSafe = false;
barriers::Point map_point(const cXyz& p) { return {p.x,p.y,p.z}; }
cXyz game_point(barriers::Point p) { return cXyz(p.x,p.y,p.z); }
void clear_arena_geometry() {
    s_barriers.clear(); s_arenaBackgrounds.clear(); s_arenaMeshReady = false; s_arenaSafe = false;
}
void add_exit_box(const fopAcM_prm_class& params, float width, float depth) {
    const cXyz center = params.base.position;
    const csXyz angle = params.base.angle;
    const float sine=cM_ssin(angle.y), cosine=cM_scos(angle.y);
    std::array<barriers::Point,4> points;
    constexpr float signs[4][2]{{-1,-1},{1,-1},{1,1},{-1,1}};
    for (unsigned i=0;i<4;++i) {
        const float x=signs[i][0]*width,z=signs[i][1]*depth;
        points[i]={center.x+x*cosine+z*sine,center.y,center.z+z*cosine-x*sine};
    }
    for (unsigned i=0;i<4;++i) s_barriers.push_back({points[i],points[(i+1)%4]});
}
bool arena_story_actor(int name) {
    const auto* base=fpcPf_Get(static_cast<s16>(name));
    const auto* profile=reinterpret_cast<const actor_process_profile_definition*>(base);
    if (profile && (profile->group==fopAc_ENEMY_e || profile->group==fopAc_NPC_e)) return true;
    switch (name) {
    case fpcNm_TBOX_e: case fpcNm_TBOX2_e: case fpcNm_Obj_Carry_e:
    case fpcNm_HORSE_e: case fpcNm_TAG_EVENT_e: case fpcNm_TAG_EVT_e:
    case fpcNm_TAG_MSG_e: case fpcNm_TAG_EVTAREA_e: case fpcNm_TAG_EVTMSG_e:
    case fpcNm_Tag_TWGate_e: case fpcNm_Tag_Lv8Gate_e: case fpcNm_Tag_Mist_e:
    case fpcNm_Tag_Restart_e: case fpcNm_Tag_ChgRestart_e: case fpcNm_Tag_RetRoom_e:
        return true;
    default: return false;
    }
}
HookAction before_arena_stage_actor(ModContext*, void* args, void*, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    const auto* record=mods::arg<stage_actor_data_class*>(args,0);
    auto* params=mods::arg<fopAcM_prm_class*>(args,1);
    if (!record || !params) return HOOK_CONTINUE;
    // DZS names are fixed-width, not necessarily null-terminated.
    char name[9]{}; std::memcpy(name,record->name,8);
    const auto* info=dStage_searchName(name);
    if (!info) return HOOK_CONTINUE; // native creator owns/frees its append
    if (info->procname==fpcNm_ALINK_e) {
        // The original arena already has a working authored entrance. Keep it
        // intact. Mode 0 is NOT an idle/no-demo spawn: setStartProcInit starts a
        // 35-frame entrance demo for it. Use native idle mode 16 on the new maps.
        if (s_map!=0) params->base.parameters=
            (params->base.parameters & 0x00fe0fffu) | 0xff010000u;
        return HOOK_CONTINUE;
    }
    const bool local=params->room_no==arena().room || params->room_no==-1;
    bool skip=arena_story_actor(info->procname);
    if (info->procname==fpcNm_SCENE_EXIT_e || info->procname==fpcNm_SCENE_EXIT2_e) {
        if (local) {
            // Native SCENE_EXIT scales its XZ half-extents by 75. Grotto exit
            // radius is scale.x*100; enclose that circle rather than cut it off.
            const float unit=info->procname==fpcNm_SCENE_EXIT_e ? 7.5f : 10.f;
            const float width=std::max(40.f,float(params->scale.x)*unit);
            const float depth=info->procname==fpcNm_SCENE_EXIT_e ?
                std::max(40.f,float(params->scale.z)*unit) : width;
            add_exit_box(*params,width,depth);
        }
        skip=true;
    } else if (local && (info->procname==fpcNm_DOOR20_e || info->procname==fpcNm_DBDOOR_e ||
                        info->procname==fpcNm_BOSS_DOOR_e)) {
        add_exit_box(*params,std::max(300.f,float(params->scale.x)*30.f),30.f);
    }
    if (!skip) return HOOK_CONTINUE;
    // The original creator normally owns this allocation. Skipping it must
    // release the append, including while streaming neighbouring field rooms.
    JKRFree(params);
    return HOOK_SKIP_ORIGINAL;
}
void collect_arena_mesh(fopAc_ac_c* actor) {
    if (fopAcM_GetName(actor)!=fpcNm_BG_e || fopAcM_GetRoomNo(actor)!=arena().room) return;
    const auto id=fopAcM_GetID(actor);
    if (std::find(s_arenaBackgrounds.begin(),s_arenaBackgrounds.end(),id)!=s_arenaBackgrounds.end()) return;
    auto* bg=static_cast<daBg_c*>(actor);
    dBgW_Base* mesh=bg->mpBgW ? static_cast<dBgW_Base*>(bg->mpBgW) : bg->mpKCol;
    if (!mesh || mesh->ChkNotReady()) return;
    int begin=0, count=0;
    if (bg->mpBgW) count=bg->mpBgW->GetBgd()->m_t_num;
    else {
        // KCL has a dummy first prism. DZB and KCL expose identical polygon
        // queries; keep both native collision formats supported.
        auto* header=bg->mpKCol->m_pkc_head;
        const auto* first=static_cast<KC_PrismData*>(header->m_prism_data);
        const auto* last=reinterpret_cast<const KC_PrismData*>(static_cast<BE(u32)*>(header->m_block_data));
        count=static_cast<int>(last-first); begin=1;
    }
    if (count<=begin || count>65535) return;
    barriers::Boundary boundary;
    cBgS_PolyInfo polygon;
    for (int i=begin;i<count;++i) {
        polygon.SetPolyIndex(i);
        cXyz a,b,c;
        if (!mesh->GetTriPnt(polygon,&a,&b,&c)) continue;
        const auto normal=mesh->GetTriPla(polygon);
        const int room=mesh->GetGrpRoomIndex(polygon);
        const bool outside=(room!=0xff && room>=0 && room!=arena().room) || mesh->GetExitId(polygon)!=0x3f;
        boundary.triangle({map_point(a),map_point(b),map_point(c)},normal.GetNP()->y>0.25f,outside);
    }
    auto walls=boundary.walls();
    s_barriers.insert(s_barriers.end(),walls.begin(),walls.end());
    s_arenaBackgrounds.push_back(id); s_arenaMeshReady=true;
    char message[160];
    std::snprintf(message,sizeof(message),"Hyrule Survivors: arena %s room %d, %u boundary segments",
        arena().stage,arena().room,unsigned(s_barriers.size()));
    svc_log->info(mod_ctx,message);
}
bool arena_path_clear(const cXyz& from, const cXyz& to, float radius=35) {
    const auto target=barriers::slide(map_point(from),map_point(to),radius,s_barriers);
    return std::abs(target.x-to.x)<1 && std::abs(target.z-to.z)<1;
}
void confine_arena_actor(fopAc_ac_c* actor, float radius) {
    actor->current.pos=game_point(barriers::slide(map_point(actor->old.pos),map_point(actor->current.pos),radius,s_barriers));
}
void after_arena_position(ModContext*, void* args, void*, void*) {
    // Authored entrances can begin behind a door/exit curtain. Let the native
    // entry and ground correction finish before constraining movement.
    if (!in_arena() || transitioning() || s_run.phase==Phase::Loading || !s_arenaSafe) return;
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!link || fopAcM_GetID(link)!=s_player) return;
    confine_arena_actor(link,40);
}
// Reject exits before the native code puts Link into an unfinishable scene-change
// action. setNextStage is additionally guarded against scripted transitions.
HookAction before_arena_exit(ModContext*, void*, void* result, void*) {
    if (!in_arena() || transitioning()) return HOOK_CONTINUE;
    *static_cast<int*>(result)=0;
    return HOOK_SKIP_ORIGINAL;
}
bool arena_grounded(daAlink_c* link) {
    const float floor=link->mLinkAcch.GetGroundH();
    return link->mLinkAcch.ChkGroundHit() && std::isfinite(floor) && floor>-1.0e8f &&
        std::abs(link->current.pos.y-floor)<20 && fopAcM_GetRoomNo(link)==arena().room;
}
void update_arena_safety(daAlink_c* link) {
    // Native ground contact is available before our derived barrier mesh. Keep
    // the first safe position during Loading as well, not just after warmup.
    if (fpcM_IsCreating(fopAcM_GetID(link))) return;
    const int room=fopAcM_GetRoomNo(link);
    if (s_arenaSafe && (room!=arena().room || link->current.pos.y<s_arenaSafePosition.y-500)) {
        // Last safe floor position, not a scene reload that would discard a run.
        link->current.pos=link->old.pos=s_arenaSafePosition;
        link->speed.set(0,0,0);
        fopAcM_SetRoomNo(link,arena().room);
    } else if (arena_grounded(link)) {
        s_arenaSafePosition=link->current.pos; s_arenaSafe=true;
    }
}
void draw_arena_barriers() {
    const auto* link=daAlink_getAlinkActorClass();
    if (!link) return;
    const auto& color=arena().barrier_color;
    const GXColor glow{color[0],color[1],color[2],185};
    const GXColor veil{color[0],color[1],color[2],55};
    // Near segments only, leaving ample space in the shared native packet queue
    // for XP, projectiles and other mods. Collision is never distance-culled.
    unsigned drawn=0;
    for (const auto& wall : s_barriers) {
        if (barriers::distance_squared(map_point(link->current.pos),wall)>3500.f*3500.f) continue;
        if (++drawn>160) break;
        cXyz a=game_point(wall.a),b=game_point(wall.b);
        a.y-=40; b.y-=40;
        cXyz quad[4]{a,b,b+cXyz(0,1500,0),a+cXyz(0,1500,0)};
        dDbVw_drawQuadXlu(quad,veil,1);
        dDbVw_drawLineXlu(quad[0],quad[1],glow,1,16);
        const float pulse=100.f+float(s_run.ticks%120)*10.f;
        a.y+=pulse; b.y+=pulse;
        dDbVw_drawLineXlu(a,b,glow,1,8);
    }
}

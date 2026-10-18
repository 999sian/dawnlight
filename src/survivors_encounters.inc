// These actors normally require a dungeon arena, Sol orb or scripted rider.
// Replace only owned action logic, retaining native resources, matrices and CC.
Enemy* encounter_owner(fopAc_ac_c* actor) {
    return roster_owned(actor)?tracked(fopAcM_GetID(actor)):nullptr;
}
void encounter_move(fopAc_ac_c* actor,const cXyz& target,float horizontal,float vertical) {
    const auto delta=target-actor->current.pos;
    const float distance=delta.absXZ();
    const float step=distance>0?std::min(1.f,horizontal/distance):0;
    actor->speed.set(delta.x*step,std::clamp(delta.y,-vertical,vertical),delta.z*step);
    actor->current.pos+=actor->speed;
    actor->speedF=0;actor->gravity=0;
    if(distance>1)actor->current.angle.y=actor->shape_angle.y=cM_atan2s(delta.x,delta.z);
}
void encounter_animation(mDoExt_McaMorfSO* morph,const char* archive,int animation,int loop=2) {
    morph->setAnm(static_cast<J3DAnmTransform*>(dComIfG_getObjectRes(archive,animation)),loop,4.f,1.f,0.f,-1.f);
}
bool encounter_running(Enemy& enemy) {
    return enemy.initialized && !enemy.retiring && !enemy.credited &&
        s_run.phase==Phase::Running && !transitioning();
}
HookAction before_roster_yc_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_yc_class*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return HOOK_CONTINUE;
    if(!enemy->encounterReady) {
        actor->mRiderID=kNone;actor->mAction=e_yc_class::ACT_F_FLY;actor->mMode=1;
        actor->mAnm=e_yc_class::ANM_FLY;
        encounter_animation(actor->mpMorf,"E_yc",actor->mAnm);
        enemy->encounterReady=true;
    }
    if(!encounter_running(*enemy))return HOOK_SKIP_ORIGINAL;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_SKIP_ORIGINAL;
    const unsigned phase=enemy->encounterTick++%180;
    if(phase==90)enemy->encounterTarget=link->current.pos+cXyz(0,60,0);
    if(phase==0||phase==90||phase==120) {
        actor->mAnm=phase!=90?e_yc_class::ANM_FLY:e_yc_class::ANM_CATCH_START;
        encounter_animation(actor->mpMorf,"E_yc",actor->mAnm,phase!=90?2:0);
    }
    const bool diving=phase>=90&&phase<120;
    const auto target=diving?enemy->encounterTarget:link->current.pos+cXyz(0,280,0);
    encounter_move(actor,target,diving?24.f:6.f,diving?24.f:8.f);
    actor->mAcch.CrrPos(dComIfG_Bgsp());
    actor->mAtSph.OffAtSetBit();
    if(diving) {
        actor->mAtSph.OnAtSetBit();actor->mAtSph.SetC(actor->current.pos);
        actor->mAtSph.SetR(65.f*actor->scale.x);dComIfG_Ccsp()->Set(&actor->mAtSph);
    }
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_head_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_DS_c*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return HOOK_CONTINUE;
    if(!enemy->encounterReady) {
        // Native Create relocates the head into Arbiter's Grounds before Execute.
        actor->current.pos=actor->old.pos=enemy->lootPosition;
        actor->shape_angle.x=actor->shape_angle.z=0;
        actor->mBossPhase=1;actor->mIsOpeningDemo=false;actor->mEyeColorAlpha=255;
        actor->mAction=daB_DS_c::ACT_B2_WAIT;actor->mMode=1;actor->mCreateFireBreath=false;
        actor->mDrawZant=false;actor->mNoDrawSword=false;actor->mWallR=200;
        actor->setBck(daB_DS_c::ANM_HEAD_FWAIT,2,3.f,1.f);
        enemy->encounterReady=true;
    }
    if(!encounter_running(*enemy))return HOOK_SKIP_ORIGINAL;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_SKIP_ORIGINAL;
    const unsigned phase=enemy->encounterTick++%240;
    const auto delta=link->current.pos-actor->current.pos;
    const float distance=delta.absXZ();
    cXyz target=actor->current.pos;
    if(distance>700) {target.x=link->current.pos.x;target.z=link->current.pos.z;}
    // Low recovery windows keep the boss beatable with the starting sword.
    target.y=link->current.pos.y+(phase>=135&&phase<210?80.f:320.f);
    encounter_move(actor,target,6.f,8.f);
    actor->shape_angle.y=actor->current.angle.y=cM_atan2s(delta.x,delta.z);
    actor->mAcchCir.SetWall(100.f,200.f);actor->mAcch.CrrPos(dComIfG_Bgsp());
    if(phase==60)actor->setBck(daB_DS_c::ANM_HEAD_FATTACK_A,0,3.f,1.f);
    if(phase==90)actor->setBck(daB_DS_c::ANM_HEAD_FATTACK_B,2,3.f,1.f);
    if(phase==135||phase==0)actor->setBck(daB_DS_c::ANM_HEAD_FWAIT,2,3.f,1.f);
    if(phase>=90&&phase<135&&phase%15==0) {
        const auto aim=link->current.pos+cXyz(0,60,0)-actor->mMouthPos;
        csXyz angle(-cM_atan2s(aim.y,aim.absXZ()),cM_atan2s(aim.x,aim.z),0);
        fopAcM_createChild(fpcNm_B_DS_e,enemy->id,daB_DS_c::TYPE_BULLET_B,&actor->mMouthPos,
            fopAcM_GetRoomNo(actor),&angle,nullptr,-1,nullptr);
    }
    actor->mSoundPos=actor->current.pos;
    actor->mpMorf->play(0,dComIfGp_getReverb(fopAcM_GetRoomNo(actor)));
    actor->mpSwordMorf->play(0,dComIfGp_getReverb(fopAcM_GetRoomNo(actor)));
    actor->mpSwordBrkAnm->play();
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_hand_action(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daE_ZH_c*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return HOOK_CONTINUE;
    if(!enemy->encounterReady) {
        actor->mS_Ball=nullptr;actor->mActionMode=4;actor->mMoveMode=1;
        actor->field_0x78c=0;actor->setBck(0x0d,2,3.f,1.f); // native search
        enemy->encounterReady=true;
    }
    if(!encounter_running(*enemy))return HOOK_SKIP_ORIGINAL;
    auto* link=daAlink_getAlinkActorClass();if(!link)return HOOK_SKIP_ORIGINAL;
    const unsigned phase=enemy->encounterTick++%180;
    if(phase==60) {
        enemy->encounterTarget=link->current.pos;
        actor->setBck(0x0b,0,3.f,1.f); // open hand telegraphs the locked landing point
    }
    if(phase==90)actor->setBck(0x06,0,3.f,1.f); // catch/slam
    if(phase==0)actor->setBck(0x0d,2,3.f,1.f);
    const auto target=phase<60?link->current.pos+cXyz(0,280,0):
        enemy->encounterTarget+cXyz(0,phase<90?300.f:phase<150?0.f:280.f,0);
    encounter_move(actor,target,phase<90?6.f:0.f,phase>=90&&phase<108?32.f:8.f);
    actor->mBgc.CrrPos(dComIfG_Bgsp());
    actor->mpBtkAnm->play();actor->mpModelMorf->play(0,dComIfGp_getReverb(fopAcM_GetRoomNo(actor)));
    return HOOK_SKIP_ORIGINAL;
}
void after_roster_hand_collision(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daE_ZH_c*>(args,0);auto* enemy=encounter_owner(actor);
    if(!enemy)return;
    actor->field_0xc7c.OffAtSetBit();
    const unsigned phase=(enemy->encounterTick+179)%180;
    if(encounter_running(*enemy)&&!enemy->stagger&&phase>=90&&phase<108) {
        actor->field_0xc7c.OnAtSetBit();actor->field_0xc7c.SetC(actor->current.pos);
        actor->field_0xc7c.SetR(150);actor->field_0xc7c.SetH(150);
        dComIfG_Ccsp()->Set(&actor->field_0xc7c);
    }
}

// Suppress only this owned head's creation-time dungeon writes/query. Nested
// unrelated processes continue to see the real room flags.
struct RosterCreateFrame {void* args;bool owned;};
std::vector<RosterCreateFrame> s_rosterHeadCreates;
HookAction before_roster_head_create(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<daB_DS_c*>(args,0);
    s_rosterHeadCreates.push_back({args,encounter_owner(actor)!=nullptr||
        (in_arena()&&active_enemy_arrow(fopAcM_GetID(actor)))});
    return HOOK_CONTINUE;
}
void after_roster_head_create(ModContext*,void* args,void*,void*) {
    if(!s_rosterHeadCreates.empty()&&s_rosterHeadCreates.back().args==args)s_rosterHeadCreates.pop_back();
}
HookAction before_roster_head_cleared(ModContext*,void*,void* result,void*) {
    if(s_rosterHeadCreates.empty()||!s_rosterHeadCreates.back().owned||!enemy_spawner_process_active())return HOOK_CONTINUE;
    *static_cast<s32*>(result)=0;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_head_zone(ModContext*,void*,void*,void*) {
    return !s_rosterHeadCreates.empty()&&s_rosterHeadCreates.back().owned&&enemy_spawner_process_active()?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;
}

// Stalhounds use native emergence, movement and effects even in daylight.
// Scope midnight to their Execute, restoring it for nested unrelated actors.
struct RosterTimeFrame {void* args;float daytime;};
std::vector<RosterTimeFrame> s_rosterTimes;
HookAction before_roster_time(ModContext*,void* args,void*,void*) {
    auto* process=mods::arg<void*>(args,1);bool owned=false;
    if(process&&fopAcM_IsActor(process)&&in_arena()) {
        auto* actor=static_cast<fopAc_ac_c*>(process);
        const auto* enemy=tracked(fopAcM_GetID(actor));
        const auto* methods=reinterpret_cast<const process_method_class*>(actor->sub_method);
        owned=enemy&&enemy->profile==fpcNm_E_SH_e&&methods&&
            mods::arg<process_method_func>(args,0)==methods->execute_method;
    }
    if(owned||!s_rosterTimes.empty()) {
        auto& time=dKy_getEnvlight()->daytime;
        const float value=owned?0.f:s_rosterTimes.front().daytime;
        s_rosterTimes.push_back({args,time});time=value;
    }
    return HOOK_CONTINUE;
}
void after_roster_time(ModContext*,void* args,void*,void*) {
    if(!s_rosterTimes.empty()&&s_rosterTimes.back().args==args) {
        dKy_getEnvlight()->daytime=s_rosterTimes.back().daytime;s_rosterTimes.pop_back();
    }
}
HookAction before_roster_dd_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<fopAc_ac_c*>(args,0);
    // The full-angle calls are acquisition/leash; keep fire-cone checks native.
    if(!arena_aggro(actor,fpcNm_E_DD_e)||mods::arg<s16>(args,2)!=0x7fff)return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_rdy_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<e_rdy_class*>(args,0);auto* enemy=encounter_owner(&actor->actor);
    if(!enemy)return HOOK_CONTINUE;
    if(enemy->dormant){*static_cast<int*>(result)=0;return HOOK_SKIP_ORIGINAL;}
    const float range=mods::arg<float>(args,1);
    if(!arena_aggro(&actor->actor,fpcNm_E_RDY_e)||(range!=actor->mPlayerCheckDist&&range!=actor->mPlayerCheckDist+50.f))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_yk_search(ModContext*,void* args,void* result,void*) {
    auto* actor=mods::arg<e_yk_class*>(args,0);const float range=mods::arg<float>(args,1);
    if(!arena_aggro(actor,fpcNm_E_YK_e)||(range!=actor->mPlayerTrigger&&range!=actor->mPlayerTrigger+50.f))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_s1_search(ModContext*,void* args,void* result,void*) {
    if(!arena_aggro(mods::arg<e_s1_class*>(args,0),fpcNm_E_S1_e))return HOOK_CONTINUE;
    *static_cast<int*>(result)=1;return HOOK_SKIP_ORIGINAL;
}
HookAction before_roster_rdy_idle(ModContext*,void* args,void*,void*) {
    auto* actor=mods::arg<e_rdy_class*>(args,0);auto* enemy=encounter_owner(&actor->actor);
    if(!enemy||!enemy->dormant)return HOOK_CONTINUE;
    if(!enemy->idleAnimation) {
        encounter_animation(actor->mpMorf,"E_rdy",0x4c);enemy->idleAnimation=true;
    }
    actor->actor.speedF=0;actor->actor.speed.x=actor->actor.speed.z=0;
    return HOOK_SKIP_ORIGINAL;
}

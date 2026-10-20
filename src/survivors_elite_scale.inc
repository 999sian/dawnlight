// Native combat spacing is expressed in unscaled model units. Limit the
// conversion to an owned elite's Execute and its melee decision states; all
// world positions, navigation probes, projectiles and unrelated actors stay in
// world units. Nested process calls suspend the outer actor's scope.
struct EliteScaleFrame {void* args;fopAc_ac_c* actor;float root=0,startY=0;bool bokAttack=false,moved=false;};
std::vector<EliteScaleFrame> s_eliteScaleFrames;
bool scaled_elite(const Enemy* enemy,const fopAc_ac_c* actor) {
    return enemy && actor && enemy->elite && enemy->initialized &&
        !enemy->retiring && !enemy->credited && !enemy->dormant && !enemy->stagger &&
        actor->health>0 && actor->scale.x>1.f && in_arena() &&
        s_run.phase==Phase::Running && !transitioning();
}
HookAction before_elite_scale_scope(ModContext*,void* args,void*,void*) {
    auto* process=mods::arg<void*>(args,1);fopAc_ac_c* owned=nullptr;
    if(process && in_arena() && is_actor_process(process)) {
        auto* actor=static_cast<fopAc_ac_c*>(process);
        const auto* enemy=tracked(fopAcM_GetID(actor));
        const auto* methods=reinterpret_cast<const process_method_class*>(actor->sub_method);
        if(scaled_elite(enemy,actor) && methods &&
           mods::arg<process_method_func>(args,0)==methods->execute_method) {
            switch(enemy->profile) {
            case fpcNm_E_OC_e:case fpcNm_E_RD_e:case fpcNm_E_RDY_e:
            case fpcNm_E_S1_e:case fpcNm_E_SH_e:owned=actor;break;
            default:break;
            }
        }
    }
    if(owned || !s_eliteScaleFrames.empty()) {
        EliteScaleFrame frame{args,owned};
        if(owned)frame.startY=owned->current.pos.y;
        if(owned && tracked(fopAcM_GetID(owned))->profile==fpcNm_E_OC_e) {
            auto* bok=static_cast<daE_OC_c*>(owned);
            frame.root=bok->field_0x6a0;
            frame.bokAttack=bok->getActionMode()==4 && bok->mOcState>=1 && bok->mOcState<=3;
        }
        s_eliteScaleFrames.push_back(frame);
    }
    return HOOK_CONTINUE;
}
void after_elite_scale_scope(ModContext*,void* args,void*,void*) {
    if(!s_eliteScaleFrames.empty() && s_eliteScaleFrames.back().args==args)s_eliteScaleFrames.pop_back();
}
void after_elite_melee_distance(ModContext*,void* args,void* result,void*) {
    if(s_eliteScaleFrames.empty())return;
    auto* actor=s_eliteScaleFrames.back().actor;
    const auto* link=daAlink_getAlinkActorClass();
    if(!actor || mods::arg<const fopAc_ac_c*>(args,0)!=actor ||
       !link || mods::arg<const fopAc_ac_c*>(args,1)!=link)return;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if(!scaled_elite(enemy,actor))return;
    bool melee=false;
    switch(enemy->profile) {
    case fpcNm_E_OC_e: {
        const int action=static_cast<daE_OC_c*>(actor)->getActionMode();
        melee=action==3 || action==15 || action==16;break; // Find / FindStay / MoveOut
    }
    case fpcNm_E_RD_e:melee=reinterpret_cast<e_rd_class*>(actor)->action==3;break; // FightRun, not BowRun
    case fpcNm_E_RDY_e:melee=reinterpret_cast<e_rdy_class*>(actor)->mAction==3;break;
    case fpcNm_E_S1_e: {
        const int action=reinterpret_cast<e_s1_class*>(actor)->mAction;
        melee=action==2 || action==3;break; // Approach and attack recovery range
    }
    case fpcNm_E_SH_e: {
        const int action=reinterpret_cast<e_sh_class*>(actor)->field_0x676;
        melee=action==2 || action==3;break; // Preserve the native random distance factor
    }
    default:break;
    }
    if(melee)*static_cast<float*>(result)/=actor->scale.x;
}
HookAction before_elite_attack_motion(ModContext*,void* args,void*,void*) {
    if(s_eliteScaleFrames.empty())return HOOK_CONTINUE;
    auto& frame=s_eliteScaleFrames.back();
    auto* acch=mods::arg<dBgS_Acch*>(args,0);
    auto* actor=acch?acch->getMyAc():nullptr;
    if(!actor || actor!=frame.actor || frame.moved)return HOOK_CONTINUE;
    const auto* enemy=tracked(fopAcM_GetID(actor));
    if(!scaled_elite(enemy,actor))return HOOK_CONTINUE;
    if(enemy->profile==fpcNm_E_OC_e && frame.bokAttack) {
        auto* bok=static_cast<daE_OC_c*>(actor);
        if(acch!=&bok->mAcch)return HOOK_CONTINUE;
        const int action=bok->getActionMode();
        if(action!=4 && action!=3 && action!=15 && action!=16)return HOOK_CONTINUE;
        // Only the animation's root delta, including its final/reverse frame.
        // Collision pushes and ordinary velocity are not animation translation.
        const float extra=(bok->field_0x6a0-frame.root)*(actor->scale.x-1.f);
        actor->current.pos.x+=extra*cM_ssin(actor->shape_angle.y);
        actor->current.pos.z+=extra*cM_scos(actor->shape_angle.y);
    } else if(enemy->profile==fpcNm_E_SH_e) {
        auto* hound=reinterpret_cast<e_sh_class*>(actor);
        if(acch!=&hound->mObjAcch || hound->field_0x676!=3 || hound->field_0x678<3 ||
           actor->speedF<=0 || hound->field_0x6a4>1.f)return HOOK_CONTINUE;
        // The doubled head already raises the bite sphere. Keep the pounce
        // low enough to meet a grounded Link by compressing only this frame's
        // vertical displacement, before native floor/wall correction. Keep
        // native vertical velocity/gravity and the committed horizontal pass.
        if(hound->field_0x678==3)
            actor->current.pos.y=frame.startY+(actor->current.pos.y-frame.startY)/actor->scale.x;
        // Match the wider leap range; the existing elite speed bonus is applied.
        const float extra=(actor->scale.x-1.f)*(faster_boss(*enemy)?1.5f:1.f);
        actor->current.pos.x+=actor->speed.x*extra;
        actor->current.pos.z+=actor->speed.z*extra;
    } else return HOOK_CONTINUE;
    frame.moved=true;acch->OnLineCheck();
    return HOOK_CONTINUE;
}
HookAction before_elite_attack_scale(ModContext*,void* args,void*,void*) {
    auto* object=mods::arg<cCcD_Obj*>(args,1);
    auto* actor=object?object->GetAc():nullptr;
    const auto* enemy=actor?tracked(fopAcM_GetID(actor)):nullptr;
    if(!scaled_elite(enemy,actor))return HOOK_CONTINUE;
    const float scale=actor->scale.x;
    if(enemy->profile==fpcNm_E_DN_e) {
        auto* e=reinterpret_cast<e_dn_class*>(actor);
        // Native model_size is 1.1. Keep the three distinct weapon/tail radii
        // and the native inactive center; never multiply a cached radius.
        if(object==&e->at_sph)e->at_sph.SetR(1.1f*(e->tail_at_sph_flag==2?70.f:e->tail_at_sph_flag==1?80.f:50.f)*scale);
    } else if(enemy->profile==fpcNm_E_KR_e) {
        auto* e=reinterpret_cast<e_kr_class*>(actor);
        if(object==&e->mSphere1)e->mSphere1.SetR(25.f*scale);
        if(object==&e->mSphere4)e->mSphere4.SetR(80.f*scale);
    } else if(enemy->profile==fpcNm_E_RD_e) {
        auto* e=reinterpret_cast<e_rd_class*>(actor);
        if(object==&e->at_sph && e->at_sph.GetR()>0) {
            const auto* link=daAlink_getAlinkActorClass();
            const bool mounted=link && (link->checkHorseRide() || link->checkBoarRide());
            // Native model_size is 1.2. A kick always uses 50; mounted club
            // swings use 60. Preserve the negative inactive-radius sentinel.
            e->at_sph.SetR(1.2f*(e->field_0x9ab!=2 && mounted?60.f:50.f)*scale);
        }
    }
    return HOOK_CONTINUE;
}

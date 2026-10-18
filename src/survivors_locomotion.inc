// Borrow only the pelvis subtree's matrix calculator while Link's body is
// evaluated. Native root/upper-body animations, attack clocks, movement and
// Dual Wield's animation packs retain their original ownership.
J3DTransformInfo blend_stride_pose(const J3DTransformInfo& a,const J3DTransformInfo& b,float weight) {
    J3DTransformInfo result=a;
    result.mTranslate.x+=(b.mTranslate.x-a.mTranslate.x)*weight;
    result.mTranslate.y+=(b.mTranslate.y-a.mTranslate.y)*weight;
    result.mTranslate.z+=(b.mTranslate.z-a.mTranslate.z)*weight;
    result.mScale.x+=(b.mScale.x-a.mScale.x)*weight;
    result.mScale.y+=(b.mScale.y-a.mScale.y)*weight;
    result.mScale.z+=(b.mScale.z-a.mScale.z)*weight;
    set_joint_rotation(result,dual::blend(joint_rotation(a),joint_rotation(b),weight));
    return result;
}
struct StridePose final : J3DAnmTransformKey {
    std::array<J3DTransformInfo,35> joints{};
    daAlink_c* link=nullptr;
    float weight=0;
    void getTransform(u16 joint,J3DTransformInfo* out) const override {
        *out=joints[joint];
        if(joint==16) {
            // The attack still owns the root. Compensate its rotation at the
            // pelvis so sword twists do not turn the running legs with it.
            const auto& root=*link->field_0x2060->getOldFrameQuaternion(0);
            const auto correction=dual::multiply(dual::conjugate({root.x,root.y,root.z,root.w}),joint_rotation(joints[0]));
            set_joint_rotation(*out,dual::multiply(correction,joint_rotation(*out)));
            const auto p=dual::rotate(correction,{out->mTranslate.x,out->mTranslate.y,out->mTranslate.z});
            out->mTranslate.x=p.x;out->mTranslate.y=p.y;out->mTranslate.z=p.z;
        }
        if(weight<1) {
            auto* native=link->field_0x1f20;
            J3DTransformInfo original,next;
            native->getAnm(0)->getTransform(joint,&original);
            for(int i=1;i<3;++i) if(auto* clip=native->getAnm(i)) {
                clip->getTransform(joint,&next);
                original=blend_stride_pose(original,next,native->getRatio(i));
            }
            *out=blend_stride_pose(original,*out,weight);
        }
    }
};
struct Locomotion {
    JKRSolidHeap* heap=nullptr;
    bool failed=false,moving=false;
    ActorId owner=kNone;
    float speed=0,phase=0,weight=0;
    s16 angle=0;
    unsigned poseTick=~0u;
    std::array<J3DAnmTransform*,8> clips{};
    StridePose pose;
    mDoExt_AnmRatioPack pack;
    std::optional<mDoExt_MtxCalcAnmBlendTblOld> calculator;
    J3DJoint* joint=nullptr;
    J3DMtxCalc* original=nullptr;
    void* call=nullptr;
} s_locomotion;
void detach_locomotion() {
    auto& s=s_locomotion;
    if(s.joint && s.calculator && s.joint->getMtxCalc()==&*s.calculator)
        s.joint->setMtxCalc(s.original);
    s.joint=nullptr;s.original=nullptr;s.call=nullptr;s.calculator.reset();
}
void clear_locomotion() {
    detach_locomotion();
    if(s_locomotion.heap) mDoExt_destroySolidHeap(s_locomotion.heap);
    s_locomotion={};
}
bool locomotion_allowed(daAlink_c* link) {
    return auto_movement(link) && !link->checkWolf() && !link->checkHorseRide() &&
        (sword_action_available(link) ||
         (s_swordAction==SwordAction::Normal && normal_sword_proc(link)) || shield_pose(link));
}
bool prepare_locomotion(daAlink_c* link) {
    auto& s=s_locomotion;
    if(s.heap) return true;
    if(s.failed || !dComIfGp_getAnmArchive()) return false;
    s.heap=mDoExt_createSolidHeapFromGame(0x30000,0x20);
    if(!s.heap) {s.failed=true;return false;}
    auto* previous=mDoExt_setCurrentHeap(s.heap);
    constexpr std::array<daAlink_c::daAlink_ANM,8> animations{
        daAlink_c::ANM_ATN_RETURN_FROM_WALK,daAlink_c::ANM_ATN_WALK_LEFT,
        daAlink_c::ANM_ATN_BACK_WALK,daAlink_c::ANM_ATN_WALK_RIGHT,
        daAlink_c::ANM_ATN_RETURN_FROM_RUN,daAlink_c::ANM_ATN_RUN_LEFT,
        daAlink_c::ANM_ATN_BACK_RUN,daAlink_c::ANM_ATN_RUN_RIGHT};
    bool valid=true;
    for(unsigned i=0;i<animations.size();++i) {
        const auto id=link->getMainBckData(animations[i])->m_underID;
        s.clips[i]=static_cast<J3DAnmTransform*>(link->loadAram(id,0x4000));
        valid=valid && s.clips[i] && s.clips[i]->field_0x1e==35 && s.clips[i]->getFrameMax()>0;
    }
    mDoExt_setCurrentHeap(previous);
    if(!valid) {clear_locomotion();s_locomotion.failed=true;return false;}
    mDoExt_adjustSolidHeap(s.heap);
    return true;
}
void after_locomotion_position(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!link || fopAcM_GetID(link)!=s_player) return;
    auto& s=s_locomotion;
    if(s.owner!=s_player) {s.owner=s_player;s.phase=s.weight=0;s.poseTick=~0u;}
    const cXyz delta=link->current.pos-link->old.pos;
    s.speed=delta.absXZ();
    s.moving=locomotion_allowed(link) && link->mMoveValue>.05f &&
        s.speed>.1f && s.speed<std::max(1.f,link->mMaxSpeed)*3;
    if(s.moving) s.angle=cM_atan2s(delta.x,delta.z);
    else s.weight=0;
}
HookAction before_locomotion_model(ModContext*,void* args,void*,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    auto* model=mods::arg<J3DModel*>(args,1);
    auto& s=s_locomotion;
    if(s.joint||!link||fopAcM_GetID(link)!=s_player)return HOOK_CONTINUE;
    const float directional=locomotion_overlay_weight(s16(s.angle-link->shape_angle.y));
    if(directional==0) {s.weight=0;return HOOK_CONTINUE;}
    if(s.joint || !locomotion_allowed(link) || !s.moving || s.owner!=s_player ||
       !model || model!=link->mpLinkModel || model->getModelData()->getJointNum()!=35 ||
       !link->field_0x2060->getOldFrameFlg() || !prepare_locomotion(link)) return HOOK_CONTINUE;
    if(s.poseTick!=s_run.ticks) {
        const auto& move=link->mpHIO->mMove.m;
        const float run=locomotion_run_blend(s.speed,move.mMaxSpeed,move.mWalkChangeRate,move.mRunChangeRate);
        const auto weights=locomotion_weights(s16(s.angle-link->shape_angle.y));
        const float walkStride=move.mMaxSpeed*move.mWalkChangeRate*s.clips[0]->getFrameMax()/std::max(.1f,move.mWalkAnmSpeed);
        const float runStride=move.mMaxSpeed*s.clips[4]->getFrameMax()/std::max(.1f,move.mRunAnmSpeed);
        s.phase=std::fmod(s.phase+s.speed/std::max(1.f,walkStride+(runStride-walkStride)*run),1.f);
        // Native attention clips mark the same foot at frames 11/1/0/9.
        // Align these contacts before mixing opposite side-step animations.
        constexpr std::array<float,4> footFrame{11,1,0,9};
        for(unsigned i=0;i<s.clips.size();++i) {
            auto* clip=s.clips[i];
            clip->setFrame(std::fmod(s.phase*clip->getFrameMax()+footFrame[i%4],float(clip->getFrameMax())));
        }
        for(unsigned joint=0;joint<35;++joint) {
            float total=0;
            for(unsigned direction=0;direction<4;++direction) if(weights[direction]>0) {
                J3DTransformInfo walk,fast;
                s.clips[direction]->getTransform(joint,&walk);s.clips[direction+4]->getTransform(joint,&fast);
                const auto pose=blend_stride_pose(walk,fast,run);
                total+=weights[direction];
                s.pose.joints[joint]=total==weights[direction] ? pose :
                    blend_stride_pose(s.pose.joints[joint],pose,weights[direction]/total);
            }
        }
        s.weight=std::min(1.f,s.weight+.25f);s.poseTick=s_run.ticks;
    }
    s.pose.link=link;s.pose.weight=s.weight*directional;
    s.pack.setAnmTransform(&s.pose);s.pack.setRatio(1);
    s.calculator.emplace(link->field_0x2060,1,&s.pack);
    s.joint=model->getModelData()->getJointNodePointer(16);
    s.original=s.joint->getMtxCalc();s.call=args;
    s.joint->setMtxCalc(&*s.calculator);
    return HOOK_CONTINUE;
}
void after_locomotion_model(ModContext*,void* args,void*,void*) {
    if(s_locomotion.call==args) detach_locomotion();
}

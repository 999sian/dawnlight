// Remap accepted game input after physical/controller/touch mapping. Restore
// the physical L history before read so analog threshold edges remain native.
bool s_swappedPad=false;
u32 s_physicalHoldL=0;
HookAction before_survivors_pad(ModContext*,void*,void*,void*) {
    if(s_swappedPad)mDoCPd_c::getCpadInfo(PAD_1).mHoldLockL=s_physicalHoldL;
    s_swappedPad=false;
    return HOOK_CONTINUE;
}
void after_survivors_pad(ModContext*,void*,void*,void*) {
    if(!survivors_switch_attack_lock())return;
    auto& pad=mDoCPd_c::getCpadInfo(PAD_1);
    s_physicalHoldL=pad.mHoldLockL;s_swappedPad=true;
    const bool attack=(pad.mButtonFlags&PAD_BUTTON_B)!=0;
    const bool attackPressed=(pad.mPressedButtonFlags&PAD_BUTTON_B)!=0;
    const bool lock=pad.mHoldLockL||(pad.mButtonFlags&PAD_TRIGGER_L)!=0;
    const bool lockPressed=pad.mTrigLockL||(pad.mPressedButtonFlags&PAD_TRIGGER_L)!=0;
    pad.mButtonFlags=(pad.mButtonFlags&~(PAD_BUTTON_B|PAD_TRIGGER_L))|
        (lock?PAD_BUTTON_B:0)|(attack?PAD_TRIGGER_L:0);
    pad.mPressedButtonFlags=(pad.mPressedButtonFlags&~(PAD_BUTTON_B|PAD_TRIGGER_L))|
        (lockPressed?PAD_BUTTON_B:0)|(attackPressed?PAD_TRIGGER_L:0);
    pad.mTriggerLeft=attack?1.f:0.f;
    pad.mHoldLockL=attack;pad.mTrigLockL=attackPressed;
}
// Normalize authored Palace gameplay cameras at type selection, before native
// orbit smoothing, collision and our distance multiplier. Event/subject/tag
// cameras retain their own types and the stage's cached records stay untouched.
void after_survivors_camera_type(ModContext*,void* args,void* result,void*) {
    auto* camera=mods::arg<dCamera_c*>(args,0);
    auto* data=mods::arg<stage_camera2_data_class*>(args,1);
    if(!result||!camera||camera->mPadID!=0||!in_arena()||
       std::string_view(arena().stage)!="D_MN08"||dComIfGp_event_runCheck())return;
    if(data!=&camera->mStageCamTool.mCamData&&data!=&camera->mRoomMapTool.mCamData&&
       data!=&camera->mDefRoomCamTool.mCamData)return;
    const int type=camera->GetCameraTypeFromCameraName("DungeonS");
    if(type>=0&&type<camera->mCamTypeNum)*static_cast<int*>(result)=type;
}

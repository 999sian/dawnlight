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
// Normalize only the camera's local gameplay tool copies, before nextType()
// selects its style. GetCameraTypeFromToolData's ARM64 entry cannot be relocated
// by Funchook on Linux v2.0.3; the enclosing setMapToolData has a stable entry.
// Authored stage records, tag cameras and event/subject selection stay native.
void after_survivors_camera_tools(ModContext*,void* args,void*,void*) {
    auto* camera=mods::arg<dCamera_c*>(args,0);
    if(!camera||camera->mPadID!=0||!in_arena()||
       std::string_view(arena().stage)!="D_MN08"||dComIfGp_event_runCheck())return;
    const int type=camera->GetCameraTypeFromCameraName("DungeonS");
    if(type<0||type>=camera->mCamTypeNum)return;
    for(auto* tool:{&camera->mStageCamTool,&camera->mRoomMapTool,&camera->mDefRoomCamTool}) {
        if(tool->mCameraIndex!=0xff && tool->mCameraIndex!=0x1ff && !tool->mpActor)
            tool->mCamData.field_0x16=type;
    }
    if(camera->mStageCamTool.mCameraIndex!=0xff && camera->mStageCamTool.mCameraIndex!=0x1ff &&
       !camera->mStageCamTool.mpActor)
        camera->mMapToolType=type;
}

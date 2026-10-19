// Hold the mapped sword button for the shared Dawnlight shoulder view. Keep
// native orbit input and movement; no item/subject procedure owns Link's body.
struct SwordAimView {
    dCamera_c* camera=nullptr;
    ActorId player=kNone;
    bool active=false, targetValid=false;
    float distance=300, height=130, fovy=45, nativeFovy=45;
    s16 gyroYaw=0;int gyroPitch=0;
    cXyz nativeEye{},nativeCenter{},eye{},center{},target{};
} s_swordAim;
void restore_sword_aim_camera(dCamera_c* camera) {
    if(camera!=s_swordAim.camera)return;
    if(camera && camera==s_swordAim.camera && camera->mEye.abs2(s_swordAim.eye)<.001f &&
       camera->mCenter.abs2(s_swordAim.center)<.001f) {
        camera->mEye=s_swordAim.nativeEye;camera->mCenter=s_swordAim.nativeCenter;
        camera->mFovy=s_swordAim.nativeFovy;camera->mDirection.Val(camera->mEye-camera->mCenter);
    }
    s_swordAim.camera=nullptr;
}
void clear_sword_aim() {
    float yaw=0,pitch=0;survivors_gyro_deltas(false,yaw,pitch);
    if(auto* camera=dComIfGp_getCamera(0))restore_sword_aim_camera(&camera->mCamera);
    auto* link=daAlink_getAlinkActorClass();
    if(link && s_swordAim.active && fopAcM_GetID(link)==s_swordAim.player) {
        link->mSight.offDrawFlg();link->mSight.offLockFlg();
    }
    s_swordAim={};
}
bool sword_aim_target(daAlink_c* link,cXyz& target,cXyz& forward) {
    auto* actor=dComIfGp_getCamera(link->field_0x317c);
    if(!actor)return false;
    cXyz eye;
    const int offset=s_swordAim.active?s_run.options.reticle_y:0;
    if(!third_person_reticle_ray(&actor->mCamera,offset,actor->mCamera.mFovy,eye,forward))return false;
    target=eye+forward*10000.f;
    dBgS_LinChk wall;wall.Set(&eye,&target,link);
    if(dComIfG_Bgsp().LineCross(&wall))target=wall.GetCross();
    // Converge from Link's launch point onto the camera ray, including nearby
    // native enemy hit shapes. Never snap to an actor's center off the reticle.
    auto* server=dComIfG_Ccsp();
    float nearest=(target-eye).abs();
    for(unsigned i=0;server && i<server->mObjTgCount;++i) {
        auto* object=server->mpObjTg[i];
        auto* actor=object?static_cast<fopAc_ac_c*>(object->GetAc()):nullptr;
        auto* enemy=actor?tracked(fopAcM_GetID(actor)):nullptr;
        if(!enemy||!enemy->initialized||enemy->retiring||enemy->credited||actor->health<=0)continue;
        auto* shape=object->GetShapeAttr();if(!shape)continue;
        cCcD_CpsAttr ray;cXyz contact;
        ray.Set(eye,eye+forward*nearest,.01f);
        if(!shape->CrossAtTg(ray,&contact))continue;
        float lo=0,hi=nearest;
        for(unsigned step=0;step<16;++step) {
            const float mid=(lo+hi)*.5f;ray.Set(eye,eye+forward*mid,.01f);
            if(shape->CrossAtTg(ray,&contact))hi=mid;else lo=mid;
        }
        nearest=hi;
    }
    target=eye+forward*nearest;
    return true;
}
bool update_sword_aim_camera(dCamera_c* camera) {
    if(!camera||camera->mPadID!=0)return false;
    auto* link=daAlink_getAlinkActorClass();
    if(!manual_attack(link)||!link->swordButton()||
       link->checkWolf()||jump_abilities_busy_for(link)||dCamera_c::isAimActive()) {
        clear_sword_aim();return false;
    }
    const cXyz nativeDirection=camera->mCenter-camera->mEye;
    if(nativeDirection.abs()<.001f){clear_sword_aim();return false;}
    if(!s_swordAim.active||s_swordAim.player!=fopAcM_GetID(link)) {
        s_swordAim.gyroYaw=0;s_swordAim.gyroPitch=0;
        s_swordAim.distance=std::clamp(nativeDirection.abs(),200.f,600.f);
        s_swordAim.height=std::clamp(camera->mCenter.y-link->current.pos.y,100.f,180.f);
        s_swordAim.fovy=camera->mFovy;
    }
    s_swordAim.active=true;s_swordAim.player=fopAcM_GetID(link);
    s_swordAim.nativeEye=camera->mEye;s_swordAim.nativeCenter=camera->mCenter;s_swordAim.nativeFovy=camera->mFovy;
    float gyroYaw=0,gyroPitch=0;
    survivors_gyro_deltas(s_run.options.gyro_aim,gyroYaw,gyroPitch);
    const int nativePitch=cM_atan2s(-nativeDirection.y,nativeDirection.absXZ());
    if(std::isfinite(gyroYaw)&&std::isfinite(gyroPitch)) {
        s_swordAim.gyroYaw=s16(s_swordAim.gyroYaw+cM_rad2s(gyroYaw));
        const int pitch=std::clamp(nativePitch+s_swordAim.gyroPitch+int(cM_rad2s(gyroPitch)),-14500,14500);
        s_swordAim.gyroPitch=pitch-nativePitch;
    }
    third_person_aim_frame(link,camera,s16(cM_atan2s(nativeDirection.x,nativeDirection.z)+s_swordAim.gyroYaw),
        s16(std::clamp(nativePitch+s_swordAim.gyroPitch,-14500,14500)),s_swordAim.distance,s_swordAim.height,
        s_swordAim.center,s_swordAim.eye);
    camera->mEye=s_swordAim.eye;camera->mCenter=s_swordAim.center;camera->mFovy=s_swordAim.fovy;
    camera->mDirection.Val(camera->mEye-camera->mCenter);s_swordAim.camera=camera;
    cXyz forward;
    s_swordAim.targetValid=sword_aim_target(link,s_swordAim.target,forward);
    if(s_swordAim.targetValid) {
        link->mSight.setPos(&s_swordAim.target);link->mSight.offLockFlg();link->mSight.onDrawFlg();
    }
    return true;
}

// Player Execute precedes the camera's final view. Correct only fresh manual
// projectiles before their first movement/collision frame, using that rendered reticle.
void resolve_manual_projectiles(dCamera_c* camera) {
    auto* link=daAlink_getAlinkActorClass();
    if(!link||!camera||camera->mPadID!=0||!playable()||s_run.phase!=Phase::Running||fopAcM_GetID(link)!=s_player)return;
    if(std::none_of(s_attacks.begin(),s_attacks.end(),[](const auto& a){return a.active&&a.aimPending;}))return;
    cXyz target,forward;if(!sword_aim_target(link,target,forward))return;
    for(auto& a:s_attacks)if(a.active&&a.aimPending) {
        a.aimPending=false;
        cXyz direction=target-a.position;
        if(direction.abs()<=.001f||direction.inprod(forward)<=0)direction=forward;
        else direction.normalize();
        const auto weapon=static_cast<Upgrade>(a.weapon);
        if(weapon==Upgrade::Bombs) {
            const float distance=std::min((target-a.position).abs(),s_run.item_range(weapon));
            a.velocity=direction*(distance/30.f);a.velocity.y+=1.4f*29.f/2.f;
            a.limit=30;continue;
        }
        if(weapon==Upgrade::Boomerang||weapon==Upgrade::Ball) {
            const float range=weapon==Upgrade::Ball?std::min(s_run.item_range(weapon),s_run.radius(900.f)):s_run.item_range(weapon);
            a.limit=std::min((target-a.position).abs(),range);
        }
        const float speed=weapon==Upgrade::Bow?70.f:weapon==Upgrade::Ball?40.f:weapon==Upgrade::Shockwave?45.f:42.f;
        const float sine=std::sin(a.aimSpread),cosine=std::cos(a.aimSpread);
        a.velocity=cXyz(direction.x*cosine+direction.z*sine,direction.y,
            direction.z*cosine-direction.x*sine)*speed;
    }
}

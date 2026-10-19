// Shared by item aiming and Survivors sword aiming. Offset uses the same
// virtual 448px screen coordinates as Dawnlight's existing reticle setting.
bool third_person_reticle_ray(dCamera_c* camera, int offsetY, float fovy, cXyz& eye, cXyz& forward) {
    if (!camera) return false;
    // Run's post-hook sees the new camera before camera_class::view is updated.
    eye = camera->mEye;
    forward = camera->mCenter - eye;
    if (forward.abs() <= 0.001f) return false;
    forward.normalize();
    {
        const float offset = static_cast<float>(offsetY);
        if (std::fabs(offset) > 0.001f) {
            // Convert the virtual 448px screen offset into the corresponding
            // angle for the current third-person camera FOV.
            const float halfFov = fovy * static_cast<float>(M_PI) / 360.0f;
            const float screenOffset = (2.0f * offset / 448.0f) * std::tan(halfFov);
            const float pitchOffset = std::atan(screenOffset);
            // Follow the rendered camera's up vector on magnetic walls/ceilings.
            const cXyz cameraUp = camera->Up();
            cXyz right(
                cameraUp.y * forward.z - cameraUp.z * forward.y,
                cameraUp.z * forward.x - cameraUp.x * forward.z,
                cameraUp.x * forward.y - cameraUp.y * forward.x);
            if (right.abs() > 0.001f) {
                right.normalize();
                cXyz up(
                    forward.y * right.z - forward.z * right.y,
                    forward.z * right.x - forward.x * right.z,
                    forward.x * right.y - forward.y * right.x);
                up.normalize();
                forward = forward * std::cos(pitchOffset) + up * std::sin(pitchOffset);
                forward.normalize();
            }
        }
    }
    return true;
}

void third_person_aim_frame(daAlink_c* link, dCamera_c* camera, short yaw, short pitch,
    float distance, float height, cXyz& center, cXyz& eye) {
    const float horizontal = distance * cM_scos(pitch);
    center = link->current.pos +
        aim_vector_to_world(link, cXyz(0.0f, height, 0.0f));
    // Translate eye and center together so framing does not rotate the aim direction.
    constexpr float shoulderLeft = 60.0f;
    constexpr float shoulderUp = 40.0f;
    const cXyz offset = aim_vector_to_world(link,
        cXyz(shoulderLeft * cM_scos(yaw) + shoulderUp * cM_ssin(yaw) * cM_ssin(pitch),
            shoulderUp * cM_scos(pitch),
            -shoulderLeft * cM_ssin(yaw) + shoulderUp * cM_scos(yaw) * cM_ssin(pitch)));
    cXyz shiftedCenter = center + offset;
    dBgS_CamLinChk shoulderCheck;
    if (camera->lineBGCheck(&center, &shiftedCenter, &shoulderCheck, 0x40b7)) {
        const float distance = std::max(0.0f, (shoulderCheck.GetCross() - center).abs() - 5.0f);
        shiftedCenter = center + offset * (distance / offset.abs());
    }
    center = shiftedCenter;
    eye = center + aim_vector_to_world(link, cXyz(-horizontal * cM_ssin(yaw),
        distance * cM_ssin(pitch), -horizontal * cM_scos(yaw)));

    // Native subject mode disables the chase-camera bump check.
    dBgS_CamLinChk check;
    if (camera->lineBGCheck(&center, &eye, &check, 0x40b7)) {
        cXyz inward = center - eye;
        inward.normalize();
        eye = check.GetCross() + inward * 5.0f;
    }
}

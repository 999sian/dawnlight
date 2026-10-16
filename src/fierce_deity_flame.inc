// Included inside fierce_deity_visual.cpp's private namespace.
// Use the resident FIRE02 flame, without its separate glow/spark emitters. Only
// emitter instances are modified; the shared particle resource stays native.
constexpr u16 kDarkFlameEffect = ID_ZF_J_FIRE02_FIRE;
// Human skeleton: spine/neck, left shoulder/elbow, right shoulder/elbow,
// left thigh/knee and right thigh/knee. Limb emitters follow their own bones
// during attacks, running and jumping instead of clustering at the torso.
constexpr std::array<u16, 11> kDarkFlameJoints{1, 2, 3, 7, 8, 12, 13, 18, 19, 23, 24};
struct DarkFlameState {
    std::array<u32, kDarkFlameJoints.size()> keys{};
    daAlink_c* owner = nullptr;
    fpc_ProcID ownerId = fpcM_ERROR_PROCESS_ID_e;
    JPAEmitterManager* manager = nullptr;
};
DarkFlameState s_darkFlame;

bool owns_dark_flame(JPABaseEmitter* emitter) {
    if (emitter == nullptr || s_darkFlame.owner == nullptr ||
        g_dComIfG_gameInfo.play.getParticle() == nullptr ||
        dPa_control_c::getEmitterManager() == nullptr ||
        dPa_control_c::getEmitterManager() != s_darkFlame.manager) return false;
    for (u32 key : s_darkFlame.keys) {
        if (key != 0 && dComIfGp_particle_getEmitter(key) == emitter) return true;
    }
    return false;
}

void stop_dark_flame() {
    auto* manager = dPa_control_c::getEmitterManager();
    if (g_dComIfG_gameInfo.play.getParticle() != nullptr && manager != nullptr &&
        manager == s_darkFlame.manager) {
        for (u32 key : s_darkFlame.keys) {
            if (key == 0) continue;
            if (auto* emitter = dComIfGp_particle_getEmitter(key)) {
                emitter->stopCreateParticle();
                emitter->stopDrawParticle();
                emitter->deleteAllParticle();
                // The native level-emitter table releases its own entries.
                // Do not free an emitter behind that table's back.
            }
        }
    }
    s_darkFlame = {};
}

void tick_dark_flame(daAlink_c* link, bool enabled) {
    auto* manager = dPa_control_c::getEmitterManager();
    if (!enabled || link == nullptr || link->checkPlayerNoDraw() ||
        link->checkWolf() || link->checkDeadHP() || link->checkEventRun() ||
        link->checkSceneChangeAreaStart() || link->mClothesChangeWaitTimer != 0 ||
        link->mpLinkModel == nullptr || link->mpLinkModel->getModelData() == nullptr ||
        link->mpLinkModel->getModelData()->getJointNum() <= kDarkFlameJoints.back() ||
        g_dComIfG_gameInfo.play.getParticle() == nullptr || manager == nullptr) {
        stop_dark_flame();
        return;
    }
    if (s_darkFlame.owner != link || s_darkFlame.ownerId != fopAcM_GetID(link) ||
        s_darkFlame.manager != manager) {
        stop_dark_flame();
        s_darkFlame.owner = link;
        s_darkFlame.ownerId = fopAcM_GetID(link);
        s_darkFlame.manager = manager;
    }
    // The keyed particle_set overload dereferences the resource manager before
    // creating anything. Scene teardown / missing common resources must be safe.
    auto* resources = manager->getResourceManager(u8{0});
    if (resources == nullptr || resources->getResource(kDarkFlameEffect) == nullptr) {
        stop_dark_flame();
        return;
    }

    const csXyz upright(0, 0, 0);
    const JGeometry::TVec3<s16> localRotation(0, 0, 0);
    const cXyz up(0.0f, 1.0f, 0.0f);
    const cXyz origin(0.0f, 0.0f, 0.0f);
    const cXyz unit(1.0f, 1.0f, 1.0f);
    for (size_t i = 0; i < kDarkFlameJoints.size(); ++i) {
        // Read the current pose after player execution, never retain model or
        // joint pointers across clothes changes (including TE's custom outfits).
        MtxP joint = link->mpLinkModel->getAnmMtx(kDarkFlameJoints[i]);
        if (joint == nullptr) continue;
        const cXyz position(joint[0][3], joint[1][3], joint[2][3]);
        auto* emitter = link->setEmitter(&s_darkFlame.keys[i], kDarkFlameEffect,
                                        &position, &upright);
        if (emitter == nullptr) continue;
        emitter->becomeContinuousParticle();
        emitter->setGlobalPrmColor(0, 0, 0);
        emitter->setGlobalEnvColor(0, 0, 0);
        emitter->setGlobalAlpha(150);
        emitter->setLocalTranslation(origin);
        emitter->setLocalRotation(localRotation);
        emitter->setLocalScale(unit);
        emitter->setDirection(up);
        emitter->setDirectionalSpeed(1.1f);
        emitter->setAwayFromCenterSpeed(0.0f);
        emitter->setAwayFromAxisSpeed(0.0f);
        emitter->setRandomDirectionSpeed(0.25f);
        emitter->setSpread(0.12f);
        emitter->setVolumeSize(12);
        // Spread the flames across the body without multiplying their density by
        // the number of new attachment points (about 60 live particles total).
        emitter->setRate(0.12f);
        emitter->setRateStep(0);
        emitter->setLifeTime(45);
        emitter->setGlobalParticleScale(0.25f, 0.4f);
    }
}

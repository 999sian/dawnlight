#include "config.hpp"
#include "save_compat.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "SSystem/SComponent/c_phase.h"
#include "m_Do/m_Do_ext.h"
#include "d/actor/d_a_b_bq.h"
#include "d/d_s_play.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace dawnlight {
namespace {

DEFINE_HOOK_SYMBOL("fopAc_Create", int(void*), ActorCreate);
#if defined(__APPLE__)
DEFINE_HOOK_SYMBOL("_ZL11b_bq_damageP10b_bq_class", void(b_bq_class*), DiababaHealthHook);
#else
DEFINE_HOOK_SYMBOL("b_bq_damage", void(b_bq_class*), DiababaHealthHook);
#endif

s16 scale_health(s16 health, int percent) {
    if (health <= 0 || percent == 100) {
        return health;
    }
    const int scaled = (static_cast<int>(health) * percent + 99) / 100;
    return static_cast<s16>(std::min(scaled, 0x7fff));
}

int effective_health_percent() {
    int percent = health_scale_percent();
    if (automatic_ngplus_health_scaling()) {
        const unsigned count = std::min(new_game_plus_count(), 9u);
        if (count != 0) {
            percent = std::max(percent, 200 + static_cast<int>(count) * 10);
        }
    }
    return percent;
}

void on_actor_create_post(ModContext*, void* args, void* retval, void*) {
    if (args == nullptr || retval == nullptr || *static_cast<int*>(retval) != cPhs_COMPLEATE_e) {
        return;
    }

    auto* actor = static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    if (actor == nullptr || actor->group != fopAc_ENEMY_e) {
        return;
    }

    // Diababa's side-head HP is a scripted bomb-hit/phase counter. Keep its
    // native values for both manual and NG+ scaling so phase one stays native.
    if (fopAcM_GetName(actor) == fpcNm_B_BH_e) {
        return;
    }

    const int percent = effective_health_percent();
    actor->health = scale_health(actor->health, percent);
    actor->field_0x560 = scale_health(actor->field_0x560, percent);
}

// Diababa overwrites its core HP inside this action, long after actor creation.
// Keep the two native reset sites, but scale each reset once before the next hit.
struct DiababaHealthFrame {
    void* args;
    b_bq_class* boss;
    int percent;
    bool resetsHealth;
};
std::vector<DiababaHealthFrame> s_diababaHealthFrames;

HookAction before_diababa_health(ModContext*, void* args, void*, void*) {
    if (args == nullptr) return HOOK_CONTINUE;
    auto* boss = mods::arg<b_bq_class*>(args, 0);
    if (boss == nullptr) return HOOK_CONTINUE;
    const int percent = effective_health_percent();
    const bool resetsHealth = boss->mMode == 0 ||
        (boss->mMode == 1 && boss->mDamageBackCount >= 2 && boss->mpMorf != nullptr &&
         boss->mpMorf->checkFrame(YREG_F(8) + 113.0f));
    s_diababaHealthFrames.push_back({args, boss, percent, resetsHealth});

    // damage_check's four-cut shortcut enters mode 20 even with HP left; its
    // fast-sword branch can then kill the boss outright. Treat that shortcut as
    // an ordinary core hit when extra HP are configured. Zero HP still takes the
    // native retreat/victory path, and the vulnerability timeout stays native.
    if (percent > 100 && boss->mMode == 20 && boss->health > 0) {
        boss->mMode = 10;
        boss->field_0x6de = 6;
        boss->mTimers[0] = std::max<s16>(boss->mTimers[0], 30);
    }
    return HOOK_CONTINUE;
}

void after_diababa_health(ModContext*, void* args, void*, void*) {
    if (s_diababaHealthFrames.empty() || s_diababaHealthFrames.back().args != args) return;
    const auto frame = s_diababaHealthFrames.back();
    s_diababaHealthFrames.pop_back();
    // Do not replace a different value written by another hook/native variant.
    if (frame.resetsHealth && frame.boss->health == 50) {
        frame.boss->health = scale_health(50, frame.percent);
        frame.boss->field_0x560 = frame.boss->health;
    }
}

}  // namespace

ModResult install_enemy_scaling_hooks(ModError* error) {
    ModResult result = mods::hook_add_post<ActorCreate>(svc_hook, on_actor_create_post);
    if (result == MOD_OK) result = mods::hook_add_pre<DiababaHealthHook>(svc_hook, before_diababa_health);
    if (result == MOD_OK) result = mods::hook_add_post<DiababaHealthHook>(svc_hook, after_diababa_health);
    if (result != MOD_OK) {
        mods::hook_uninstall<DiababaHealthHook>(svc_hook);
        mods::hook_uninstall<ActorCreate>(svc_hook);
        return mods::set_error(error, result, "failed to install Dawnlight enemy scaling hook");
    }
    return MOD_OK;
}

}  // namespace dawnlight

#include "survivors.hpp"
#include "process_actor.hpp"
#include "survivors_rules.hpp"
#include "survivors_roster.hpp"
#include "survivors_items.hpp"
#include "survivors_maps.hpp"
#include "survivors_barriers.hpp"
#include "enemy_spawner.hpp"
#include "bow_fire_resource.hpp"
#include "save_state.hpp"
#include "gale_counter.hpp"
#include "jump_abilities.hpp"
#include "m_Do/m_Do_MemCard.h"
#include "service_imports.hpp"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/flow.hpp"
#include "dusk/ui/nav_types.hpp"
#include "d/d_menu_ring.h"
#include "d/d_menu_collect.h"
#include "d/d_menu_item_explain.h"
#include "d/d_map.h"
#include "d/d_camera.h"
#include "d/d_msg_object.h"
#include "d/d_msg_class.h"
#include "JSystem/JMessage/control.h"
#include "SSystem/SComponent/c_math.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_midna.h"
#include "d/actor/d_a_demo_item.h"
#include "d/actor/d_a_e_ba.h"
#include "d/actor/d_a_e_oc.h"
#include "d/actor/d_a_obj_carry.h"
#include "d/actor/d_a_obj_item.h"
#include "d/actor/d_a_obj_life_container.h"
#include "d/d_bg_s_wtr_chk.h"
#include "m_Do/m_Do_lib.h"
#include "d/actor/d_a_e_ms.h"
#include "d/actor/d_a_e_rd.h"
#include "d/actor/d_a_e_dd.h"
#include "d/actor/d_a_e_sh.h"
#include "d/actor/d_a_e_rdy.h"
#include "d/actor/d_a_e_s1.h"
#include "d/actor/d_a_e_yk.h"
#include "d/actor/d_a_e_yc.h"
#include "d/actor/d_a_b_ds.h"
#include "d/actor/d_a_e_zh.h"
#include "d/actor/d_a_e_tt.h"
#include "d/actor/d_a_e_dn.h"
#include "d/actor/d_a_e_mm.h"
#include "d/actor/d_a_e_mm_mt.h"
#include "d/actor/d_a_e_ge.h"
#include "d/actor/d_a_e_kr.h"
#include "d/actor/d_a_e_cr.h"
#include "d/actor/d_a_e_bs.h"
#include "d/actor/d_a_e_gi.h"
#include "d/actor/d_a_e_sf.h"
#include "d/actor/d_a_e_fs.h"
#include "d/actor/d_a_e_bu.h"
#include "d/actor/d_a_e_sm2.h"
#include "d/actor/d_a_e_mf.h"
#include "d/actor/d_a_b_gg.h"
#include "d/actor/d_a_b_tn.h"

#include "d/d_com_inf_game.h"
#include "d/d_debug_viewer.h"
#include "d/d_gameover.h"
#include "d/d_item.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_s_play.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_overlap_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_create_req.h"
#include "f_pc/f_pc_create_tag.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_leaf.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_Reset.h"
#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DPrint.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JKernel/JKRSolidHeap.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "JSystem/J3DGraphAnimator/J3DJoint.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "res/Object/AlAnm.h"
#include "d/d_particle_name.h"
#include "d/d_bg_w.h"
#include "f_pc/f_pc_profile.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "d/actor/d_a_bg.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <memory>

namespace dawnlight {
namespace {
using namespace survivors;
#include "survivors_roster_native.inc"
constexpr char kMode[] = "hyrule_survivors";
unsigned s_map = 0, s_requestedMap = 0;
const Arena& arena() { return arenas[s_map]; }
// Keep the original key so the versioned payload can migrate existing saves.
constexpr char kBlob[] = "hyrule-survivors-records-v1";
constexpr ActorId kNone = fpcM_ERROR_PROCESS_ID_e;
DEFINE_HOOK(&daAlink_c::execute, SurvivorsPlayer);
// Symbol form avoids MSVC constinit failure for this multiply-inherited class.
DEFINE_HOOK_SYMBOL("dBgS_Acch::CrrPos", void(dBgS_Acch*, dBgS&), SurvivorsEnemySpeed);
DEFINE_HOOK(&daAlink_c::setDamagePoint, SurvivorsEnemyDamage);
DEFINE_HOOK(&daAlink_c::commonBottleDrink, SurvivorsPotionDrink);
DEFINE_HOOK(&dComIfGp_setItemLifeCount, SurvivorsPotionHeal);
DEFINE_HOOK(&dCamera_c::Run, SurvivorsCamera);
DEFINE_HOOK(&daAlink_c::orderZTalk, SurvivorsMidnaOrder);
DEFINE_HOOK(&dMsgFlow_c::getInitNodeIndex, SurvivorsMidnaFlow);
DEFINE_HOOK(&daAlink_c::checkSceneChange, SurvivorsArenaExit);
DEFINE_HOOK(&daAlink_c::checkRestartRoom, SurvivorsArenaRestart);
DEFINE_HOOK_SYMBOL("src/d/d_stage.cpp#dStage_actorCreate", void(stage_actor_data_class*, fopAcM_prm_class*, size_t), SurvivorsArenaActor);
DEFINE_HOOK(&daAlink_c::checkDamageAction, SurvivorsBlock);
DEFINE_HOOK(&daAlink_c::setShieldGuard, SurvivorsGuard);
DEFINE_HOOK(&daE_OC_c::damage_check, SurvivorsBokoblinHit);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rd.cpp#damage_check", void(e_rd_class*), SurvivorsBulblinHit);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_rd.cpp#pl_check", BOOL(e_rd_class*, f32, s16), SurvivorsBulblinAggro);
DEFINE_HOOK(&fopAcM_createChild, SurvivorsEnemyArrow);
DEFINE_HOOK(&daE_OC_c::cc_set, SurvivorsBokoblinTargets);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ms.cpp#e_ms_damage", void(e_ms_class*), SurvivorsRatRecovery);
DEFINE_HOOK(&daE_OC_c::searchPlayer, SurvivorsBokoblinAggro);
DEFINE_HOOK(&daE_OC_c::executeWait, SurvivorsBokoblinWait);
DEFINE_HOOK(&daObjCarry_c::obj_break, SurvivorsPotBreak);
DEFINE_HOOK(&renderingAmap_c::afterDrawPath, SurvivorsEliteMinimap);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ba.cpp#pl_check", BOOL(e_ba_class*, f32, s16), SurvivorsKeeseAggro);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ms.cpp#pl_check", int(e_ms_class*, f32), SurvivorsRatAggro);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ba.cpp#damage_check", void(e_ba_class*), SurvivorsKeeseHit);
DEFINE_HOOK_SYMBOL("src/d/actor/d_a_e_ms.cpp#damage_check", void(e_ms_class*), SurvivorsRatHit);
DEFINE_HOOK(&daAlink_c::setMatrix, SurvivorsRangedAim);
DEFINE_HOOK(&daAlink_c::checkUpperItemActionBow, SurvivorsBowAction);
DEFINE_HOOK(&daAlink_c::checkNextActionBow, SurvivorsBowNext);
DEFINE_HOOK(&daAlink_c::checkUpperItemActionIronBall, SurvivorsBallAction);
DEFINE_HOOK(&daAlink_c::checkNextActionIronBall, SurvivorsBallNext);
DEFINE_HOOK(&dMenu_Collect2D_c::isSkillIconVisible, SurvivorsScrollVisible);
DEFINE_HOOK(&dMenu_Collect2D_c::wait_proc, SurvivorsCollectionWait);
DEFINE_HOOK(&dMenu_Collect2D_c::pointerActivateCurrent, SurvivorsCollectionClick);
DEFINE_HOOK(&dMenu_Collect2D_c::_delete, SurvivorsCollectionDelete);
DEFINE_HOOK(&dMeter2Info_c::readItemTexture, SurvivorsScrollTexture);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SurvivorsRingCursor);
DEFINE_HOOK(&dMenu_Ring_c::setItem, SurvivorsRingAssign);
DEFINE_HOOK(&dMenu_Ring_c::setMixItem, SurvivorsRingMix);
DEFINE_HOOK(&dSv_player_status_a_c::setSelectItemIndex, SurvivorsBind);
DEFINE_HOOK(&dSv_player_status_a_c::setMixItemIndex, SurvivorsMixBind);
DEFINE_HOOK(&dMenu_ItemExplain_c::openExplain, SurvivorsExplain);
DEFINE_HOOK(&JMessage::TControl::update, SurvivorsTreasureText);
DEFINE_HOOK(&daAlink_c::checkUpperGrabItemThrow, SurvivorsBombThrow);
DEFINE_HOOK(&daAlink_c::cancelUpperItemReadyAnime, SurvivorsRangedCancel);
DEFINE_HOOK(&daAlink_c::procCoGetItemInit, SurvivorsGetItem);
DEFINE_HOOK(&fpcLf_Draw, SurvivorsTreasureDraw);
DEFINE_HOOK(&daAlink_c::draw, SurvivorsDraw);
DEFINE_HOOK(&daAlink_c::procCoDeadInit, SurvivorsDeath);
DEFINE_HOOK(&daAlink_c::checkDoCutAction, SurvivorsManualAttack);
DEFINE_HOOK(&daAlink_c::checkCutAction, SurvivorsSwordAttack);
DEFINE_HOOK(&daAlink_c::transAnimeProc, SurvivorsMovement);
DEFINE_HOOK(&daAlink_c::posMove, SurvivorsPosition);
DEFINE_HOOK(&daAlink_c::checkAutoJumpAction, SurvivorsLedgeJump);
DEFINE_HOOK(&daAlink_c::procCutTurn, SurvivorsSpinMove);
DEFINE_HOOK(&daAlink_c::procCutLargeJump, SurvivorsJumpStrike);
DEFINE_HOOK(&daAlink_c::procCutLargeJumpLand, SurvivorsJumpStrikeLand);
DEFINE_HOOK(&daAlink_c::setCutLargeJumpLandEffect, SurvivorsJumpStrikeEffect);
DEFINE_HOOK(&daAlink_c::procCutNormal, SurvivorsNormalMove);
DEFINE_HOOK(&daAlink_c::procCutFinish, SurvivorsFinishMove);
DEFINE_HOOK(&daAlink_c::checkCutLargeTurnState, SurvivorsLargeSpin);
DEFINE_HOOK(&fopAcM_orderDoorEvent, SurvivorsDoor);
DEFINE_HOOK(&at_power_check, SurvivorsPower);
DEFINE_HOOK(&cc_at_check, SurvivorsHitMaterial);
DEFINE_HOOK(&daAlink_c::checkLightSwordMtrl, SurvivorsLightSword);
DEFINE_HOOK(&dMeter2Draw_c::draw, SurvivorsHud);
DEFINE_HOOK(&dComIfGs_gameStart, SurvivorsGameStart);
DEFINE_HOOK(&dComIfG_changeOpeningScene, SurvivorsOpening);
DEFINE_HOOK(&fpcMtd_Method, SurvivorsProcess);
DEFINE_HOOK(&fopAcM_createItemFromTable, SurvivorsDrops);
DEFINE_HOOK(&fopAcM_createItem, SurvivorsDirectDrops);
DEFINE_HOOK(static_cast<void (*)(const char*, s16, s8, s8, f32, u32, int, s8, s16, int, int)>(
    &dComIfGp_setNextStage), SurvivorsStage);

bool s_active = false, s_registered = false, s_loaded = false, s_restart = false;
// Selecting a mode starts the title flow, not a game. A slot callback prepares
// the save; only gameStart commits it after names and optional TV settings.
bool s_saveReady = false, s_gameStarted = false;
bool s_resultsStored = false, s_storeFailed = false, s_saveRequested = false;
Run s_run;
RunOptions s_options;
Records s_records;
bool s_progressSaving=false, s_progressSaved=false;
UiWindowHandle s_refreshChoices=0;
// Opaque host documents: no private RmlUi/Window layouts are copied.
DEFINE_HOOK_SYMBOL("dusk::ui::Window::handle_nav_command",
                   bool(void*, void*, dusk::ui::NavCommand), SurvivorsMenuWindow);
DEFINE_HOOK_SYMBOL("dusk::ui::Modal::handle_nav_command",
                   bool(void*, void*, dusk::ui::NavCommand), SurvivorsMenuModal);
void* s_menuDocument = nullptr;
void* s_talentDocument = nullptr;
void remember_menu_document(bool talent);
UiWindowHandle s_window = 0, s_powerupsWindow = 0;
UiDialogHandle s_talentDialog=0;
bool s_powerupsRequested = false;
dMenu_Collect2D_c* s_powerupsMenu = nullptr;
void close_powerup_window();
ActorId s_player = kNone;
fpc_ProcID s_savePrompt = kNone;
unsigned s_spawnCooldown = 0, s_spawnFailures = 0, s_warmup = 0;
unsigned s_midnaRequestFrames = 0;
bool s_midnaTalking = false, s_midnaEndPending = false;
bool midna_dialog_busy() { return s_midnaRequestFrames || s_midnaTalking || s_midnaEndPending; }
void reset_midna_dialog() { s_midnaRequestFrames=0; s_midnaTalking=s_midnaEndPending=false; }
enum class SwordAction { None, Normal, Spin, Jump };
SwordAction s_swordAction = SwordAction::None;
unsigned s_meleeCooldown = 0;
unsigned s_comboStep = 0, s_comboTimeout = 0;
bool s_autoSwordCall = false, s_trackMelee = false;
ActorId s_meleeTarget = kNone;
bool s_wavePending = false, s_allowDrop = false;
std::array<unsigned, unsigned(Upgrade::Rupees)> s_cooldowns{};
void clear_loot();
void clear_attacks();
void interrupt_auto_actions(daAlink_c*);
void finish(bool won);
void clear_enemy_effects();
bool effect_collider(const cCcD_Obj*);
bool native_sword_collider(const cCcD_Obj*);
void clear_jump_effects();
void new_melee_swing();
void cancel_treasure_demo();
int potion_slot();
unsigned potion_capacity();
void cancel_ranged();
bool prepare_treasure_message(ChestReward, const Run&, const Run&, Rarity);
unsigned treasure_message_id();
void sync_survivors_inventory();
bool queue_ranged(daAlink_c*, Upgrade);
struct Enemy;
void initialize_extended_enemy(Enemy&,fopAc_ac_c*);
void drop_elite_chest(Enemy&, fopAc_ac_c*);
std::array<unsigned, 3> s_choices{0, 1, 2};
struct ProcessFrame { void* args; ActorId id; };
std::vector<ProcessFrame> s_processFrames;

struct Enemy {
    ActorId id = kNone;
    ProfileName profile = 0;
    bool initialized = false, credited = false, retiring = false, elite = false;
    unsigned age = 0;
    int eliteSlot = -1;
    bool dormant = false, idleAnimation = false;
    cXyz lootPosition{};
    unsigned species=0, hitCooldown=0, stagger=0;
    cCcD_Obj* childHit=nullptr;
    bool encounterReady=false;
    unsigned encounterTick=0;
    cXyz encounterTarget{};
};
std::array<Enemy, enemy_capacity> s_enemies;
struct Orb { cXyz position{}; unsigned value = 0; bool summoned = false, merged = false; };
std::array<Orb, orb_capacity> s_orbs;
struct Attack {
    bool active = false;
    unsigned life = 0, weapon = 0, age = 0;
    bool returning = false, exploding = false;
    ActorId target = kNone;
    unsigned hops = 0, visitedCount = 0;
    std::array<ActorId, 11> visited{};
    std::array<ActorId, enemy_capacity> ballVictims{};
    unsigned ballVictimCount = 0;
    float travel = 0, limit = 0;
    cXyz position{}, velocity{};
    float radius = 0;
    dCcD_Stts status{};
    dCcD_Sph collider{};
    std::array<u32, 9> emitters{};
};
// Stable addresses: collision server retains pointers until its next frame.
std::array<Attack, projectile_capacity> s_attacks;
const dCcD_SrcSph kAttackSource = {
    {{0, {{AT_TYPE_NORMAL_SWORD, 3, 0x1a}, {0, 0}, 0}},
     {dCcD_SE_SWORD, 3, dCcG_At_Spl_UNK_1, dCcD_MTRL_NONE, {1}},
     {dCcD_SE_NONE, 0, 0, dCcD_MTRL_NONE, {0}}, {0}},
    {{{0, 0, 0}, 50}},
};

bool in_arena() {
    return s_active && s_gameStarted && arena_matches(s_map, dComIfGp_getStartStageName(), dComIfGp_getStartStageRoomNo());
}
bool ui_visible() {
    bool value = false;
    svc_ui->is_any_document_visible(mod_ctx, &value);
    return value;
}
bool transitioning() { return dComIfGp_isEnableNextStage() || fopOvlpM_IsPeek(); }
bool playable() {
    return in_arena() && !midna_dialog_busy() && !transitioning() && !dComIfGp_isPauseFlag() &&
        !dScnPly_c::isPause() && !dComIfGp_event_runCheck() && !ui_visible() &&
        dMeter2Info_getGameOverType() == 0;
}
fopAc_ac_c* actor_by_id(ActorId id) {
    fopAc_ac_c* actor = nullptr;
    if (id != kNone && !fpcM_IsCreating(id)) fopAcM_SearchByID(id, &actor);
    return actor;
}
Enemy* tracked(ActorId id) {
    for (auto& e : s_enemies) if (e.id != kNone && e.id == id) return &e;
    return nullptr;
}
#include "survivors_enemy_types.inc"
#include "survivors_enemy_arrows.inc"
#include "survivors_elite_positions.inc"
#include "survivors_maps.inc"
#include "survivors_extended_ai.inc"
#include "survivors_aggro.inc"
#include "survivors_enemy_facing.inc"
#include "survivors_feedback.inc"
#include "survivors_ground.inc"
#include "survivors_pickups.inc"
#include "survivors_player_damage.inc"
#include "survivors_options.inc"
#include "survivors_potions.inc"
void stop_attack(Attack& a) {
    for (auto& id : a.emitters) {
        if (id) {
            if (auto* emitter = dComIfGp_particle_getEmitter(id)) {
                emitter->stopCreateParticle();
                emitter->quitImmortalEmitter();
                emitter->becomeInvalidEmitter();
            }
            id = 0;
        }
    }
    a.active = false; a.life = 0;
    a.collider.OffAtSetBit(); a.collider.ResetAtHit();
}
void clear_attacks() {
    clear_jump_effects();
    s_meleeTarget = kNone; s_trackMelee = false;
    cancel_ranged();
    for (auto& a : s_attacks) stop_attack(a);
}
void close_window() {
    s_menuDocument = s_talentDocument = nullptr;
    const auto dialog=s_talentDialog;s_talentDialog=0;
    if(dialog)svc_ui->dialog_close(mod_ctx,dialog);
    const auto handle = s_window; s_window = 0;
    if (handle) svc_ui->window_close(mod_ctx, handle);
}
#include "survivors_menu_handoff.inc"
void retire_pots();
void cleanup_pots();
void detach_pot_attacks();
void retire_enemies() {
    retire_pots(); retire_pickups(); retire_enemy_arrows();
    clear_enemy_effects();
    for (auto& e : s_enemies) if (e.id != kNone) e.retiring = true;
}
void cleanup_enemies() {
    cleanup_enemy_arrows();
    for (auto& e : s_enemies) {
        if (e.id == kNone) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor && !fpcM_IsCreating(e.id)) {
            e = {}; continue;
        }
        // Pending creation is not completion or a kill. Keep its ID until the scene
        // destroys it or the actor is ready to accept the deferred delete request.
        if (e.retiring && actor) svc_actor->delete_actor(mod_ctx, e.id);
    }
}
bool own_collider(const cCcD_Obj* collider) {
    if (!collider) return false;
    if (effect_collider(collider)) return true;
    for (const auto& a : s_attacks) if (collider == &a.collider) return true;
    return false;
}
void detach_attack_references() {
    detach_pot_attacks();
    // The collision server also keeps last-frame pointers for debug drawing.
    // Null only our entries; never reset another actor/mod's collision state.
    if (auto* server = dComIfG_Ccsp()) {
        for (auto*& obj : server->mpObjAt) if (own_collider(obj)) obj = nullptr;
        for (auto*& obj : server->mpObjTg) if (own_collider(obj)) obj = nullptr;
        for (auto*& obj : server->mpObjCo) if (own_collider(obj)) obj = nullptr;
        for (auto*& obj : server->mpObj) if (own_collider(obj)) obj = nullptr;
    }
    const auto clear = [](dCcD_Sph& collider) {
        if (own_collider(collider.GetTgHitObj())) collider.ResetTgHit();
    };
    for (const auto& e : s_enemies) {
        auto* actor = actor_by_id(e.id);
        if (!actor || !e.initialized) continue;
        dCcU_AtInfo* info = nullptr;
        if (e.profile == fpcNm_E_OC_e) {
            auto* native = static_cast<daE_OC_c*>(actor);
            for (auto& collider : native->mSphs_cc) clear(collider);
            info = &native->mAtInfo;
        } else if (e.profile == fpcNm_E_BA_e) {
            auto* native = reinterpret_cast<e_ba_class*>(actor);
            clear(native->mSph); info = &native->mAtInfo;
        } else if (e.profile == fpcNm_E_MS_e) {
            auto* native = static_cast<e_ms_class*>(actor);
            clear(native->mSph1); clear(native->mSph2); info = &native->mAtInfo;
        } else if (e.profile == fpcNm_E_RD_e) {
            auto* native = reinterpret_cast<e_rd_class*>(actor);
            for (auto& collider : native->cc_sph) clear(collider);
            info = &native->at_info;
        }
        if (info && own_collider(info->mpCollider)) info->mpCollider = nullptr;
    }
}
void reset_runtime() {
    reset_midna_dialog();
    s_saveReady = false; s_gameStarted = false;
    s_map = s_requestedMap = 0; clear_arena_geometry();
    close_powerup_window(); close_window(); clear_attacks(); clear_loot(); detach_attack_references(); retire_enemies();
    if (s_savePrompt != kNone) {
        d_GameOver_Delete(s_savePrompt); s_savePrompt = kNone;
        dComIfGp_setGameoverStatus(0);
    }
    clear_combat_feedback();
    s_run = {}; s_orbs = {}; s_cooldowns = {};
    s_player = kNone; s_loaded = false; s_resultsStored = false;
    s_storeFailed = false; s_restart = false; s_saveRequested = false;
    s_progressSaving=s_progressSaved=false;
    s_spawnCooldown = s_spawnFailures = s_warmup = 0;
    s_swordAction = SwordAction::None; s_meleeCooldown = 0; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
}
void prepare_save() {
    save_state_set_boss_rush_active(false);
    save_state_set_intro_skipped(false);
    auto* save = dComIfGs_getSaveData();
    save->getPlayer().getPlayerReturnPlace().set(arena().stage, arena().room, arena().point);
    dComIfGs_onEventBit(dSv_event_flag_c::F_0250);
    dComIfGs_onEventBit(dSv_event_flag_c::F_0302);
    dComIfGs_onEventBit(dSv_event_flag_c::M_010);
    dComIfGs_onEventBit(dSv_event_flag_c::M_067); // Native Midna riding/call and touch icon.
    for (int province=0; province<3; ++province) dComIfGs_onDarkClearLV(province);
    dComIfGs_setTransformStatus(TF_STATUS_HUMAN);
    dComIfGs_onItemFirstBit(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_setCollectClothes(KOKIRI_CLOTHES_FLAG);
    dComIfGs_setSelectEquipClothes(dItemNo_WEAR_KOKIRI_e);
    dComIfGs_offCollectSword(COLLECT_LIGHT_SWORD);
    dComIfGs_offItemFirstBit(dItemNo_LIGHT_SWORD_e);
    for (int shield = COLLECT_WOODEN_SHIELD; shield <= COLLECT_HYLIAN_SHIELD; ++shield) dComIfGs_offCollectShield(shield);
    dComIfGs_offCollectSword(COLLECT_MASTER_SWORD);
    dComIfGs_offItemFirstBit(dItemNo_MASTER_SWORD_e);
    dComIfGs_onItemFirstBit(dItemNo_SWORD_e);
    dComIfGs_setCollectSword(COLLECT_ORDON_SWORD);
    dComIfGs_setSelectEquipSword(dItemNo_SWORD_e);
    dComIfGs_setSelectEquipShield(dItemNo_NONE_e);
    dComIfGs_setBButtonItemKey(dItemNo_SWORD_e);
    // Save callbacks run inside file select, before Link exists. The live
    // dComIfGs_setSelectItemIndex hook in HD HUD checks Link's wolf state for
    // slot 2. Initialize the data directly, including the play cache, instead.
    // Clear assignments first so setItem cannot refresh an assigned slot.
    auto& status = save->getPlayer().getPlayerStatusA();
    for (int i = 0; i < MAX_SELECT_ITEM; ++i) {
        status.setSelectItemIndex(i, dItemNo_NONE_e);
        status.setMixItemIndex(i, dItemNo_NONE_e);
        g_dComIfG_gameInfo.play.setSelectItem(i, dItemNo_NONE_e);
    }
    for (int i = 0; i < MAX_ITEM_SLOTS; ++i) dComIfGs_setItem(i, dItemNo_NONE_e);
    dComIfGs_setWalletSize(2); // Giant wallet for escalating chest prices.
    dComIfGs_setMaxLife(15); // Three hearts; max life uses fifths, gauge uses quarters.
    dComIfGs_setLife(12);
    dComIfGs_setRupee(0);
}
void load_records() {
    if (s_loaded) return;
    s_records = {};
    auto bytes = s_records.encode(); size_t size = bytes.size();
    const auto result = svc_save->get_blob(mod_ctx, kBlob, bytes.data(), &size);
    if (result == MOD_OK) s_records.decode(std::span(bytes.data(), std::min(size, bytes.size())));
    s_loaded = true;
}
#include "survivors_progress_save.inc"
void finish(bool won) {
    if (s_run.phase != Phase::Running && s_run.phase != Phase::Upgrade) return;
    s_run.end(won); clear_attacks(); retire_enemies();
    // Keep native Game Over from stealing the result screen. A replay reloads the
    // scene, so no dead Link action or temporary combat state survives it.
    dComIfGs_setLife(dComIfGs_getMaxLifeGauge());
    if (!s_resultsStored) {
        load_records(); s_records.finish(s_run,s_map); s_progressSaved=false;
        const auto bytes = s_records.encode();
        s_storeFailed = svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK;
        s_resultsStored = true;
    }
}
#include "survivors_midna.inc"
void add_orb(cXyz position, unsigned value) {
    position.y -= 20;
    cXyz probe = position; probe.y += 500;
    dBgS_ObjGndChk ground; ground.SetPos(&probe);
    const float floor = dComIfG_Bgsp().GroundCross(&ground);
    if (std::isfinite(floor) && floor > -1.0e8f) position.y = floor + 25;
    Orb* nearest = &s_orbs[0]; float distance = 1.0e20f;
    for (auto& orb : s_orbs) {
        if (!orb.value) { orb = {position, value}; return; }
        const float d = orb.position.abs2(position);
        if (d < distance) { distance = d; nearest = &orb; }
    }
    // Preserve XP at the cap without spawning more game actors or particles.
    nearest->value += value;
    nearest->merged = true;
}
void drop_reward(fopAc_ac_c* actor) {
    const auto drop = s_run.roll_drop();
    if (drop == Run::Drop::None) return;
    constexpr std::array<int, unsigned(Run::Drop::Count)> items{-1, dItemNo_HEART_e,
        dItemNo_GREEN_RUPEE_e, dItemNo_BLUE_RUPEE_e, dItemNo_YELLOW_RUPEE_e,
        dItemNo_RED_RUPEE_e, dItemNo_PURPLE_RUPEE_e, dItemNo_ORANGE_RUPEE_e, dItemNo_SILVER_RUPEE_e};
    const int item = items[unsigned(drop)];
    // Native item physics, pickup effects and currency/health handling. Ordinary
    // enemy drop hooks remain blocked, so a death cannot generate two rewards.
    create_run_pickup(actor->current.pos, item);
}
void credit_death(Enemy& e, fopAc_ac_c* actor) {
    if (e.credited || e.retiring || !e.initialized || s_run.phase != Phase::Running ||
        !in_arena() || transitioning()) return;
    bool dead = actor->health <= 0;
    if (e.profile == fpcNm_E_BA_e)
        dead = dead || reinterpret_cast<e_ba_class*>(actor)->mIsDying;
    if (e.profile == fpcNm_E_OC_e)
        dead = dead || static_cast<daE_OC_c*>(actor)->getActionMode() == 9;
    if (!dead) return;
    e.credited = true; ++s_run.kills;
    if(e.elite) {
        if(e.eliteSlot>=0 && unsigned(e.eliteSlot)<timed_elite_count)++s_run.timed_elites;
        else ++s_run.guard_elites;
    }
    if (e.elite || s_run.normal_kill_drops()) {
        if (!e.elite) drop_reward(actor);
        add_orb(actor->current.pos, e.elite ? 30 : (e.profile == fpcNm_E_OC_e || e.profile == fpcNm_E_RD_e) ? 3 : 1);
    }
    if (e.elite) { drop_elite_chest(e, actor); queue_exp_magnet(e, actor); }
}
void update_orbs(daAlink_c* link) {
    const float range = magnet_range();
    for (auto& orb : s_orbs) {
        if (!orb.value) continue;
        const auto target = link->current.pos + cXyz(0, 30, 0);
        cXyz delta = target - orb.position;
        const float distance = delta.abs();
        if (distance < 60) { s_run.xp += orb.value; orb = {}; }
        else if (orb.summoned || distance < range) {
            const float speed = orb.summoned ? std::max(35.f, distance * 0.08f) : 35.f;
            orb.position += delta * (std::min(speed, distance) / distance);
        }
    }
}
bool create_enemy_at(cXyz pos, s16 yaw, unsigned type, bool elite, int eliteSlot = -1, bool dormant = false) {
    auto slot = std::find_if(s_enemies.begin(), s_enemies.end(), [](const auto& e) { return e.id == kNone; });
    if (slot == s_enemies.end()) return false;
    if (type >= native_roster.size()) return false;
    const auto& native=native_roster[type];
    ActorId id = kNone;
    const float scale = elite ? elite_scale(Species(type)) : 1.0f;
    const ActorSpawnParams params{
        .parameters = native.parameters, .argument = native.argument, .room_num = arena().room,
        .position = {pos.x, pos.y, pos.z},
        .angle = {static_cast<s16>(native.profile==fpcNm_E_RDY_e ? 0xff : 0), static_cast<s16>(yaw + 0x8000), static_cast<s16>(native.profile==fpcNm_E_KR_e ? 0xff : 0)},
        .scale = {scale, scale, scale}, .create_function = nullptr,
    };
    if (create_standalone_actor(native.profile, params, id) != MOD_OK) return false;
    *slot = {id, native.profile, false, false, false, elite, 0};
    slot->species=type;
    slot->eliteSlot = eliteSlot; slot->dormant = dormant; slot->lootPosition = pos;
    return true;
}
bool spawn_enemy(daAlink_c* link, bool elite, int eliteSlot = -1) {
    auto slot = std::find_if(s_enemies.begin(), s_enemies.end(), [](const auto& e) { return e.id == kNone; });
    if (slot == s_enemies.end()) return false;
    std::array<unsigned,species_count> counts{};
    for (const auto& e:s_enemies) if (e.id!=kNone && !e.elite && !e.retiring && !e.credited && e.species<species_count)
        ++counts[e.species];
    const unsigned groupRoll=elite?0:s_run.next_random(),pickRoll=elite?0:s_run.next_random();
    const unsigned type=elite ? unsigned(eliteSlot>=0 && unsigned(eliteSlot)<timed_elite_count ?
        map_roster(s_map).elites[eliteSlot] : Species::Bokoblin) :
        unsigned(choose_species(s_map,s_run.ticks/ticks_per_second,s_run.horde_remaining!=0,
            s_run.options.horde,counts,groupRoll,pickRoll));
    if (type>=native_roster.size()) return false;
    for (unsigned attempt = 0; attempt < 12; ++attempt) {
        const auto yaw = static_cast<s16>(s_run.next_random());
        const float radius = 600.0f + float(s_run.next_random() % 450);
        cXyz pos = link->current.pos + cXyz(cM_ssin(yaw) * radius, 0, cM_scos(yaw) * radius);
        if (!walkable_spawn_path(link->current.pos, pos, elite ? 100.f : 40.f)) continue;
        pos.y += native_roster[type].height;
        return create_enemy_at(pos, yaw, type, elite, eliteSlot);
    }
    return false;
}
#include "survivors_elites.inc"
#include "survivors_pots.inc"
#include "survivors_minimap.inc"
void update_spawns(daAlink_c* link) {
    unsigned population = 0;
    for (auto& e : s_enemies) {
        if (e.id == kNone) continue;
        if (!e.elite) ++population;
        auto* actor = actor_by_id(e.id);
        if (!actor || !e.initialized) continue;
        ++e.age;
        // Guards and scheduled elites persist until killed; cleanup grants no loot.
        if (!e.elite && !e.retiring && e.age > 5400) e.retiring = true;
    }
    if (s_run.ticks <= duration && s_run.ticks % elite_interval == 0) s_spawnCooldown = 0;
    for (const auto& encounter : s_eliteEncounters)
        if (encounter.defeated && encounter.respawnAt == s_run.ticks) s_spawnCooldown = 0;
    if (s_spawnCooldown) { --s_spawnCooldown; return; }
    s_spawnCooldown = s_run.spawn_interval();
    update_elite_spawns(link);
    if (s_run.ticks >= duration || population>=s_run.target_population()) { s_spawnFailures=0; return; }
    for (unsigned i=0; i<s_run.spawn_batch() && population<s_run.target_population(); ++i) {
        if (spawn_enemy(link,false)) { ++population; s_spawnFailures=0; }
        else { ++s_spawnFailures; break; }
    }
}
cXyz target_direction(daAlink_c* link) {
    float nearest = 1600 * 1600;
    cXyz result(cM_ssin(link->shape_angle.y), 0, cM_scos(link->shape_angle.y));
    const cXyz origin = link->current.pos + cXyz(0, 70, 0);
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.retiring || e.credited) continue;
        auto* actor = actor_by_id(e.id);
        if (!actor) continue;
        cXyz target = actor->eyePos;
        cXyz delta = target - origin;
        const float distance = delta.abs2();
        if (distance < 1 || distance >= nearest) continue;
        dBgS_LinChk wall; wall.Set(&origin, &target, link);
        if (dComIfG_Bgsp().LineCross(&wall)) continue;
        nearest = distance; result = delta * (1.0f / std::sqrt(distance));
    }
    return result;
}
void launch(daAlink_c* link, unsigned weapon, cXyz direction) {
    auto slot = std::find_if(s_attacks.begin(), s_attacks.begin() + 8, [](const auto& a) { return !a.active; });
    if (slot == s_attacks.begin() + 8) return;
    auto& a = *slot;
    a.weapon = weapon; a.life = 35;
    a.position = link->current.pos + cXyz(0, 65, 0);
    a.velocity = direction * 42.0f;
    a.radius = s_run.radius(65.0f);
    a.status.Init(120, 0xff, link);
    a.collider.Set(kAttackSource); a.collider.SetStts(&a.status);
    a.collider.SetAtType(AT_TYPE_NORMAL_SWORD);
    a.collider.SetAtAtp(static_cast<u8>(s_run.attack_power(weapon)));
    // Keep the native material neutral so linear Survivors damage applies.
    a.collider.SetAtMtrl(dCcD_MTRL_NONE);
    a.collider.OnAtNoConHit(); a.collider.ResetAtHit(); a.collider.OnAtSetBit();
    a.active = true;
}
void update_wave_effects(Attack& a, daAlink_c* link) {
    // Same moving-emitter approach as Great Spin Projectile, using only the
    // two small normal-spin effects, never the six large-spin emitters.
    constexpr std::array<u16, 2> effects{ID_ZI_J_KAITENGIRI_A, ID_ZI_J_KAITENGIRI_B};
    const csXyz rotation(-cM_atan2s(a.velocity.y, a.velocity.absXZ()),
                         cM_atan2s(a.velocity.x, a.velocity.z), 0);
    for (unsigned i = 0; i < effects.size(); ++i) {
        auto& id = a.emitters[i];
        auto* emitter = id ? dComIfGp_particle_getEmitter(id) : nullptr;
        if (id && (!emitter || (emitter->mMaxFrame > 0 && emitter->getAge() >= emitter->mMaxFrame))) {
            if (emitter) {
                emitter->stopCreateParticle(); emitter->quitImmortalEmitter();
                emitter->becomeInvalidEmitter();
            }
            id = 0;
        }
        id = dComIfGp_particle_set(id, effects[i], &a.position, &link->tevStr,
                                  &rotation, nullptr, 0xff, nullptr, -1, nullptr, nullptr, nullptr);
        if (id) {
            dComIfGp_particle_levelEmitterOnEventMove(id);
            if (auto* current = dComIfGp_particle_getEmitter(id)) current->setGlobalTranslation(a.position);
        }
    }
}
bool sword_action_available(daAlink_c* link) {
    if (!link->mLinkAcch.ChkGroundHit() || link->checkWolf() || !link->mSwordModel) return false;
    switch (link->mProcID) {
    case daAlink_c::PROC_WAIT: case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE: case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE: case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN: return true;
    default: return false; // Never interrupt damage, dodge, jumping or item actions.
    }
}
bool enemy_in_range(daAlink_c* link, float range) {
    for (const auto& e : s_enemies) {
        if (!e.initialized || e.retiring || e.credited) continue;
        if (auto* actor = actor_by_id(e.id); actor && actor->current.pos.abs(link->current.pos) < range)
            return true;
    }
    return false;
}
void release_sword_wave(daAlink_c* link) {
    const auto direction = target_direction(link);
    launch(link, 0, direction);
    if (s_run.ranks[0] >= 3) {
        launch(link, 0, cXyz(direction.x * 0.94f + direction.z * 0.34f,
            direction.y, direction.z * 0.94f - direction.x * 0.34f));
        if (s_run.ranks[0] >= 5) launch(link, 0, cXyz(direction.x * 0.94f - direction.z * 0.34f,
            direction.y, direction.z * 0.94f + direction.x * 0.34f));
    }
}
bool normal_sword_proc(daAlink_c* link) {
    return link->mProcID == daAlink_c::PROC_CUT_NORMAL || link->mProcID == daAlink_c::PROC_CUT_FINISH;
}
#include "survivors_targeting.inc"
bool begin_sword_combo(daAlink_c* link) {
    // Native recovery can clear its counter before our cooldown expires. Keep
    // the automatic sequence across that gap, but let native code pick each
    // directional/lock-on/hit-dependent swing and the fourth-hit finisher.
    link->mComboCutCount = s_comboStep;
    struct Scope { Scope(){s_autoSwordCall=true;} ~Scope(){s_autoSwordCall=false;} } scope;
    if (!link->checkCutAction() || !normal_sword_proc(link)) return false;
    s_comboStep = link->mComboCutCount % 4;
    s_comboTimeout = 2 * ticks_per_second;
    return true;
}
#include "survivors_jump_strike.inc"
void update_sword(daAlink_c* link) {
    if (s_meleeCooldown) --s_meleeCooldown;
    if (s_comboTimeout && !--s_comboTimeout) s_comboStep = 0;
    bool continuingSwing = false;
    if (s_swordAction == SwordAction::Jump) return; // Owned traversal finishes before the scheduler resumes.
    if (s_swordAction == SwordAction::Normal) {
        if (normal_sword_proc(link)) {
            if (s_wavePending && link->mUnderFrameCtrl[0].getFrame() >= link->field_0x3478) {
                release_sword_wave(link); s_wavePending = false;
            }
            // Never cut off the damage window, and let the final hit recover.
            if (!s_comboStep || s_meleeCooldown || link->mUnderFrameCtrl[0].getFrame() <=
                std::max(link->field_0x347c, link->field_0x3484)) return;
            continuingSwing = true;
        } else {
            s_wavePending = false; // Interrupted attacks cannot fire late from a damage/dodge action.
            s_swordAction = SwordAction::None; s_trackMelee = false; s_meleeTarget = kNone;
        }
    } else if (s_swordAction == SwordAction::Spin) {
        if (link->mProcID == daAlink_c::PROC_CUT_TURN) return;
        s_swordAction = SwordAction::None;
    }
    if ((!continuingSwing && !sword_action_available(link)) ||
        !link->mLinkAcch.ChkGroundHit() || link->checkWolf() || !link->mSwordModel) {
        s_comboStep = s_comboTimeout = 0;
        return;
    }
    const bool spin = !continuingSwing && s_run.ranks[1] && !s_cooldowns[1] && enemy_in_range(link, s_run.radius(450));
    const bool wave = s_run.ranks[0] && !s_cooldowns[0] && enemy_in_range(link, 1600);
    if (!spin && !wave && (s_meleeCooldown || !melee_target(link))) return;
    if (!link->checkItemSwordEquip()) {
        if (link->mEquipItem != dItemNo_NONE_e) return;
        // The pending item ID can survive an interrupted upper animation.
        // Only an actually running equip animation means the draw is pending.
        if (!link->checkEquipAnime()) link->swordEquip(0);
        return; // Let native equip animation finish before replacing the body action.
    }
    // Roll only at a real melee opportunity, never once per idle frame.
    if (!spin && !wave && try_jump_strike(link)) return;
    auto direction = target_direction(link);
    if (!spin && !wave) {
        auto* target = melee_target(link);
        if (!target) return;
        direction = target->current.pos - link->current.pos;
    } else { s_meleeTarget = kNone; s_trackMelee = false; }
    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->current.angle.y = link->shape_angle.y;
    if (spin) {
        if (link->procCutTurnInit(1, 1)) {
            s_swordAction = SwordAction::Spin;
            s_comboStep = s_comboTimeout = 0;
            link->mAtSph.SetR(s_run.radius(link->field_0x347c));
            s_cooldowns[1] = s_run.cooldown(100);
        }
    } else if (wave ? link->procCutNormalInit(1) : begin_sword_combo(link)) {
        s_swordAction = SwordAction::Normal; s_wavePending = wave; s_trackMelee = !wave;
        new_melee_swing();
        if (wave) { s_comboStep = s_comboTimeout = 0; link->mComboCutCount = 0; }
        s_meleeCooldown = s_run.cooldown(30);
        if (wave) s_cooldowns[0] = s_run.cooldown(40);
    }
}
void after_sword_move(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || !playable() || s_run.phase != Phase::Running || fopAcM_GetID(link) != s_player) return;
    const bool spin = s_swordAction == SwordAction::Spin && link->mProcID == daAlink_c::PROC_CUT_TURN;
    const bool normal = s_swordAction == SwordAction::Normal && normal_sword_proc(link);
    // Scale the actual collision radius each frame, not only its target. The
    // native expansion window is too short to reach a scaled target at high Reach.
    if (spin) link->mAtSph.SetR(s_run.radius(link->field_0x347c));
    if ((!spin && !normal) || !link->mLinkAcch.ChkGroundHit()) return;
    // Runs before native posMove/ground/wall resolution, retaining collision.
    // Normal swings and finishers decelerate/lunge too, not only spin attacks.
    link->current.angle.y = link->mMoveAngle;
    link->mNormalSpeed = link->mpHIO->mMove.m.mMaxSpeed * link->mStickValue;
}
HookAction before_large_spin(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
#include "survivors_loot.inc"
#include "survivors_ranged.inc"
#include "survivors_treasure.inc"
#include "survivors_inventory.inc"
#include "survivors_status.inc"
void new_melee_swing() { ++s_meleeSerial; }
#include "survivors_combat_pause.inc"
#include "survivors_defense.inc"
#include "survivors_bulblin.inc"
#include "survivors_movement.inc"
#include "survivors_ledge_jump.inc"

void update_weapons(daAlink_c* link) {
    for (auto& a : s_attacks) {
        if (!a.active) continue;
        a.status.Move();
        if (a.weapon >= unsigned(Upgrade::Bow)) { update_item_attack(a, link); continue; }
        if (!a.life-- || a.collider.ChkAtShieldHit()) { stop_attack(a); continue; }
        const cXyz next = a.position + a.velocity;
        dBgS_LinChk wall; wall.Set(&a.position, &next, link);
        if (a.weapon == 0 && dComIfG_Bgsp().LineCross(&wall)) { stop_attack(a); continue; }
        a.position = next;
        if (a.weapon == 0) update_wave_effects(a, link);
        a.collider.SetC(a.position); a.collider.SetR(a.radius);
        a.collider.SetAtVec(a.velocity);
        dComIfG_Ccsp()->Set(&a.collider);
    }
    if (update_jump_strike(link)) return;
    if(auto_attacks_suspended(link)){suspend_auto_attacks(link);return;}
    for (auto& cooldown : s_cooldowns) if (cooldown) --cooldown;
    update_ranged(link);
    const bool shieldAttack = update_shield_attack(link);
    // A finished sword draw gets one melee opportunity before another due
    // item animation sheathes it again. Active combos keep the ranged handoff.
    const bool resumeSword=!s_ranged.active && !shieldAttack &&
        s_swordAction==SwordAction::None && link->checkItemSwordEquip();
    if (resumeSword) update_sword(link);
    if (!shieldAttack) fire_items(link);
    if (!s_ranged.active && !shieldAttack && !resumeSword) update_sword(link);

}
bool native_sword_collider(const cCcD_Obj* collider) {
    auto* link = daAlink_getAlinkActorClass();
    if (!in_arena() || !link || fopAcM_GetID(link) != s_player) return false;
    if (s_swordAction == SwordAction::Spin && collider == &link->mAtSph) return true;
    if (s_swordAction == SwordAction::Jump && jump_strike_pose(link) && collider == &link->mAtSph) return true;
    if (s_swordAction == SwordAction::Normal || (s_swordAction == SwordAction::Jump && jump_strike_pose(link)))
        for (const auto& capsule : link->mAtCps) if (collider == &capsule) return true;
    return false;
}
void after_power(ModContext*, void* args, void*, void*) {
    if (!s_active) return;
    auto* info = mods::arg<dCcU_AtInfo*>(args, 0);
    if (!info || !info->mpCollider) return;
    if (reuse_bulblin_power(info)) { mark_damage_critical(info,bulblin_power_hit(info)->critical); return; }
    const auto critical_power = [&](unsigned base) {
        bool critical=false; const auto power=s_run.critical_power(base,&critical);
        if (auto* hit=bulblin_power_hit(info)) hit->critical=critical;
        mark_damage_critical(info,critical);
        return power;
    };
    const BulblinPowerCache cache{info};
    for (const auto& effect : s_effects) if (info->mpCollider == &effect.collider) {
        info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK;
        if (auto* hit=bulblin_power_hit(info)) hit->critical=effect.critical;
        mark_damage_critical(info,effect.critical);
        info->mAttackPower = effect.damage * s_run.damage_multiplier(); return;
    }
    auto* link = daAlink_getAlinkActorClass();
    if (in_arena() && link && info->mpCollider == &link->mGuardAtCps) {
        info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK; // Rat stun must not reset elite HP to one.
        info->mAttackPower = critical_power(s_run.attack_power(unsigned(Upgrade::ShieldAttack))) * s_run.damage_multiplier(); return;
    }
    if (native_sword_collider(info->mpCollider)) {
        info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK; // Keep cc_at_check from doubling the linear value for Master Sword.
        const auto base = s_swordAction == SwordAction::Jump && info->mpCollider == &link->mAtSph ?
            s_run.jump_splash_power() : s_swordAction == SwordAction::Spin ? s_run.attack_power(1) : s_run.melee_power();
        info->mAttackPower = critical_power(base) * s_run.damage_multiplier();
        return;
    }
    for (const auto& attack : s_attacks) {
        if (info->mpCollider == &attack.collider) {
            // The native table maps 3 to 30 and >=4 to up to 200 damage.
            // Keep native collision, defense, damage reactions and death handling,
            // but use a linear power value for our own weapons only.
            info->mHitType = HIT_TYPE_LINK_HEAVY_ATTACK;
            info->mAttackPower = critical_power(info->mpCollider->GetAtAtp()) * s_run.damage_multiplier();
            return;
        }
    }
}
void after_player(ModContext*, void* args, void*, void*) {
    if (!s_active || s_run.phase != Phase::Running || !playable()) return;
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link || fopAcM_GetID(link) != s_player) return;
    if (dComIfGs_getLife() == 0) { finish(false); return; }
    update_combat_feedback();
    s_run.tick(true);
    if (s_run.ticks >= duration && s_run.timed_elites == timed_elite_count) { finish(true); return; }
    update_enemy_effects();
    update_spawns(link); update_pots(link); update_exp_magnets(link); update_weapons(link); update_orbs(link);
    s_run.maximum_hearts = dComIfGs_getMaxLife() / 5;
    if (s_run.level_up()) clear_attacks();
    else update_chests(link);
}
HookAction before_death(ModContext*, void*, void* result, void*) {
    if (!in_arena() || (s_run.phase != Phase::Running && s_run.phase != Phase::Upgrade)) return HOOK_CONTINUE;
    finish(false);
    *static_cast<int*>(result) = 1;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_cut(ModContext*, void*, void* result, void*) {
    if (!in_arena()) return HOOK_CONTINUE;
    *static_cast<BOOL*>(result) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_sword_attack(ModContext* ctx, void* args, void* result, void* user) {
    if (s_autoSwordCall) return HOOK_CONTINUE;
    return before_cut(ctx, args, result, user);
}
#include "survivors_extended_damage.inc"
#include "survivors_encounters.inc"
bool pause_run_actor(bool enemy) {
    if (!in_arena() || s_run.phase == Phase::Loading) return false;
    if (enemy && (midna_dialog_busy() || dComIfGp_event_runCheck())) return true;
    if (ui_visible() || dComIfGp_isPauseFlag()) return true;
    // Native Get Item needs Link's Execute; enemies and the run clock stay paused.
    return s_run.phase != Phase::Running && (enemy || s_run.phase != Phase::Treasure);
}
HookAction before_process(ModContext*, void* args, void* result, void*) {
    auto* process = mods::arg<void*>(args, 1);
    if (!process || !is_actor_process(process)) return HOOK_CONTINUE;
    auto* actor = static_cast<fopAc_ac_c*>(process);
    const auto id = fopAcM_GetID(actor);
    auto* enemy = tracked(id);
    auto* pot = tracked_pot(id);
    auto* pickup = tracked_pickup(id);
    auto* arrow = tracked_enemy_arrow(id);
    retain_treasure_actor(actor, args);
    if (in_arena()) {
        const auto* nativeMethods = reinterpret_cast<const process_method_class*>(actor->sub_method);
        if (nativeMethods && mods::arg<process_method_func>(args, 0) == nativeMethods->execute_method)
            collect_arena_mesh(actor);
    }
    if (!enemy && !pot && !pickup && !arrow && (!in_arena() || id != s_player)) return HOOK_CONTINUE;
    const auto* methods = reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (!methods) return HOOK_CONTINUE;
    const auto method = mods::arg<process_method_func>(args, 0);
    if (method == methods->delete_method) {
        if (pickup) pickup->id=kNone;
        if (arrow) arrow->id=kNone;
        // Never award XP here: cancellation, unloading and despawning also delete.
        if (enemy) {
            *enemy = {};
        }
        if (pot) {
            pot->id=kNone; pot->initialized=false;
            if (pot->retiring) *pot={};
        }
        return HOOK_CONTINUE;
    }
    if (method != methods->execute_method) return HOOK_CONTINUE;
    if (arrow && (arrow->retiring || pause_run_actor(true))) {
        *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
    }
    if (pickup) {
        if (pickup->retiring || pause_run_actor(pickup_waiting(*pickup,actor))) {
            *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
        }
        move_pickup(*pickup,actor);
        if (pickup->retiring) { *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL; }
    }
    if (pot) pot->initialized=true;
    if (enemy) s_processFrames.push_back({args, id});
    if (enemy) initialize_enemy(enemy,actor);
    if (enemy && enemy->dormant && enemy->profile == fpcNm_E_OC_e) {
        auto* guard = static_cast<daE_OC_c*>(actor);
        guard->mMoveRange = 0; guard->field_0x6e3 = 1; // No social alert/idle search.
        if (guard->getActionMode() <= 3) guard->setActionMode(0, 0);
    }
    if (enemy) { engage_enemy(actor, *enemy); track_enemy_facing(actor); }
    if ((enemy && enemy->retiring) || (pot && pot->retiring) || pause_run_actor(enemy != nullptr || pot != nullptr)) {
        *static_cast<int*>(result) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    if (enemy && extended_enemy(enemy->profile) && !update_extended_enemy(actor,*enemy)) {
        *static_cast<int*>(result)=1; return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
void after_process(ModContext*, void* args, void*, void*) {
    // Only matched Execute callbacks may inspect an actor. Delete callbacks can
    // return after freeing it, and nested methods must not consume another frame.
    if (s_processFrames.empty() || s_processFrames.back().args != args) return;
    const auto frame = s_processFrames.back(); s_processFrames.pop_back();
    if (!in_arena() || s_run.phase != Phase::Running || transitioning()) return;
    auto* enemy = tracked(frame.id);
    auto* actor = actor_by_id(frame.id);
    scale_elite_colliders(enemy,actor);
    finish_extended_targets(enemy,actor);
    if (enemy && actor) {
        confine_arena_actor(actor, enemy->elite ? 80.f : 35.f);
        if (enemy->elite && actor->health > 0 && enemy->age % 15 == 0) {
            cXyz floor = actor->current.pos;
            if (elite_floor_position(floor)) enemy->lootPosition = floor;
        }
        credit_death(*enemy, actor);
    }
}
HookAction before_drop(ModContext*, void*, void* result, void*) {
    if (!in_arena() || s_allowDrop) return HOOK_CONTINUE;
    *static_cast<fpc_ProcID*>(result) = kNone;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_game_start(ModContext*, void*, void*, void*) {
    if (s_active && s_saveReady && !mDoRst::isReset() && !mDoRst::isReturnToMenu())
        s_gameStarted = true;
    return HOOK_CONTINUE;
}
HookAction before_opening(ModContext*, void*, void*, void*) {
    if (s_active) reset_runtime();
    return HOOK_CONTINUE;
}
HookAction before_stage(ModContext*, void* args, void*, void*) {
    if (!s_active || !s_gameStarted || mDoRst::isReset() || mDoRst::isReturnToMenu()) return HOOK_CONTINUE;
    // gameStart and the subsequent new-file F_SP108 override must both target
    // the arena. The earlier opening/title request must remain untouched.
    if (in_arena() && !s_restart && s_run.phase != Phase::Loading) return HOOK_SKIP_ORIGINAL;
    dComIfGs_setRestartRoomParam(0);
    mods::arg_ref<const char*>(args, 0) = arena().stage;
    mods::arg_ref<s16>(args, 1) = arena().point;
    mods::arg_ref<s8>(args, 2) = arena().room;
    mods::arg_ref<s8>(args, 3) = 0;
    return HOOK_CONTINUE;
}
void after_draw(ModContext*, void*, void*, void*) {
    if (!in_arena() || transitioning()) return;
    for (auto& orb : s_orbs) {
        if (!orb.value) continue;
        cXyz size(10, 16, 10); csXyz angle(0, static_cast<s16>(s_run.ticks * 300), 0);
        dDbVw_drawCubeOpa(orb.position, size, angle, orb.merged ? GXColor{255,70,70,255} : GXColor{90,235,255,255});
    }
    for (const auto& drop:s_expMagnets) {
        if (!drop.active) continue;
        cXyz size(50,80,50); csXyz angle(0,static_cast<s16>(s_run.ticks*300),0);
        cXyz pos=drop.position+cXyz(0,8*cM_ssin(s16(s_run.ticks*700)),0);
        dDbVw_drawCubeOpa(pos,size,angle,GXColor{255,210,45,255});
    }
    draw_loot();
    draw_ranged();
    draw_arena_barriers();

}
void label(const char* text, float x, float y, float size) {
    auto* font = mDoExt_getMesgFont(); if (!font) return;
    J2DPrint shadow(font, JUtility::TColor(0,0,0,230), JUtility::TColor(0,0,0,230));
    shadow.setFontSize(size, size);
    shadow.printReturn(text, 550, size + 8, HBIND_CENTER, VBIND_TOP, x - 274, y + 1, 255);
    J2DPrint print(font, JUtility::TColor(180,245,255,255), JUtility::TColor(255,240,195,255));
    print.setFontSize(size, size);
    print.printReturn(text, 550, size + 8, HBIND_CENTER, VBIND_TOP, x - 275, y, 255);
}
#include "survivors_prices.inc"
#include "survivors_feedback_draw.inc"
void draw_hud(ModContext*, void*, void*, void*) {
    if (!in_arena() || transitioning() || ui_visible() || s_run.phase == Phase::Loading || s_run.phase == Phase::Ready) return;
    auto* graf = dComIfGp_getCurrentGrafPort(); if (!graf) return;
    graf->setup2D();
    draw_chest_prices();
    draw_combat_feedback();
    const float x = mDoGph_gInf_c::getMinXF() + mDoGph_gInf_c::getWidthF() * 0.5f;
    char text[180];
    std::snprintf(text, sizeof(text), "%02u:%02u / 20:00   Level %u   Score %u",
        s_run.ticks / 1800, s_run.ticks / 30 % 60, s_run.level, s_run.score());
    label(text, x, 40, 17);
    std::snprintf(text, sizeof(text), "XP %u / %u", s_run.xp, s_run.required_xp());
    label(text, x, 62, 14);
    // J2DPrint enables direct TEX0 attributes. fillBox emits only position and
    // color, so reset the vertex layout before its quads or the GX FIFO reads
    // subsequent command bytes as missing texture coordinates.
    graf->setup2D();
    const float fraction = std::min(1.0f, float(s_run.xp) / float(s_run.required_xp()));
    const auto oldTL = graf->mColorTL, oldTR = graf->mColorTR;
    const auto oldBR = graf->mColorBR, oldBL = graf->mColorBL;
    graf->setColor(JUtility::TColor(12, 25, 32, 210));
    graf->fillBox(JGeometry::TBox2<f32>(x - 160, 82, x + 160, 88));
    graf->setColor(JUtility::TColor(80, 220, 245, 240));
    graf->fillBox(JGeometry::TBox2<f32>(x - 160, 82, x - 160 + 320 * fraction, 88));
    graf->setColor(oldTL, oldTR, oldBR, oldBL);
    if (s_run.chest_rewards_left(potion_slot() >= 0))
        std::snprintf(text, sizeof(text), "Next chest: %u rupees", s_run.chest_cost());
    else std::snprintf(text, sizeof(text), "No chest rewards available");
    label(text, x, 96, 14);
    if (s_run.horde_remaining) {
        std::snprintf(text,sizeof(text),"HORDE!  3x enemy spawns  %us",(s_run.horde_remaining+29)/30);
        label(text,x,118,17);
    }
    if (s_spawnFailures >= 10)
        label("Waiting for a clear enemy spawn position", x, s_run.horde_remaining ? 140 : 118, 13);
    graf->setup2D();
}
void select_upgrade(ModContext*, void* data) {
    const unsigned choice = *static_cast<unsigned*>(data);
    if (s_run.phase != Phase::Upgrade || choice >= 3) return;
    const auto selected = s_run.offers[choice];
    s_run.maximum_hearts = dComIfGs_getMaxLife() / 5;
    const unsigned oldHearts=s_run.rank(Upgrade::Heart);
    const unsigned bonus = s_run.upgrade_rupees(selected, s_run.offer_rarities[choice]);
    if (!s_run.choose(choice)) return;
    if (selected == Upgrade::Heart)
        dComIfGs_setMaxLife(std::min(100u,unsigned(dComIfGs_getMaxLife())+5*(s_run.rank(Upgrade::Heart)-oldHearts)));
    if (bonus) dComIfGs_setRupee(std::min<unsigned>(dComIfGs_getRupeeMax(), dComIfGs_getRupee() + bonus));
    close_window();
    s_run.level_up(); // Preserve queued levels; do not resume combat between choices.
}
void start_run(ModContext*, void*) {
    if (s_run.phase != Phase::Ready || !s_arenaMeshReady || s_restart || s_progressSaving) return;
    close_window(); clear_attacks(); clear_loot(); prepare_save();
    dMeter2Info_setSword(dItemNo_SWORD_e, false); // Refresh live equipment after a previous run.
    const auto seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    reset_elite_encounters();
    clear_combat_feedback();
    s_run.start(seed, s_options, s_records.talents); reset_gale_charges();
    dComIfGs_setMaxLife(s_run.maximum_hearts*5); dComIfGs_setLife(s_run.maximum_hearts*4);
    s_chestDelay += s_run.next_random() % (10 * ticks_per_second + 1);
    s_resultsStored = false; s_cooldowns = {}; s_spawnCooldown = 30;
    s_swordAction = SwordAction::None; s_meleeCooldown = 0; s_wavePending = false;
    s_comboStep = s_comboTimeout = 0;
    begin_starting_item_choice();
}
void replay(ModContext*, void*) { close_window(); s_requestedMap = s_map; s_restart = true; }
void save_results(ModContext*, void*) { close_window(); s_saveRequested = true; }
ModResult button(UiElementHandle pane, const char* text, UiPressedFn callback, void* data = nullptr,
                 const char* help = nullptr) {
    UiControlDesc desc = UI_CONTROL_DESC_INIT;
    desc.label = text; desc.on_pressed = callback; desc.user_data = data; desc.help_rml = help;
    return svc_ui->pane_add_control(mod_ctx, pane, &desc, nullptr);
}
#include "survivors_choice_ui.inc"
#include "survivors_choice_actions.inc"
#include "survivors_chest_ui.inc"
#include "survivors_powerups.inc"
#include "survivors_talents.inc"
#include "survivors_maps_ui.inc"

ModResult build_window(ModContext*, UiWindowHandle, UiElementHandle left, UiElementHandle right,
                       void*, ModError*) {
    if (s_run.phase == Phase::TreasureChoice) return build_treasure_choices(left, right);
    if (s_run.phase == Phase::Upgrade) return build_upgrade_choices(left, right);
    char text[240];
    if (s_run.phase == Phase::Ready) return build_arena_choices(left, right);
    std::snprintf(text, sizeof(text), "%s - %02u:%02u | %u kills | Level %u",
        s_run.phase == Phase::Victory ? "Victory" : "Defeat", s_run.ticks / 1800,
        s_run.ticks / 30 % 60, s_run.kills, s_run.level);
    svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
    std::snprintf(text,sizeof(text),"Score %u | Map high score %u | Difficulty x%.2f",
        s_run.score(),s_records.high_scores[s_map],s_run.options.score_percent()/100.f);
    svc_ui->pane_add_text(mod_ctx,left,text,nullptr);
    std::snprintf(text,sizeof(text),"Survival %us | Normal kills %u | Timed elites %u | Other elites %u",
        std::min(s_run.ticks,duration)/ticks_per_second,s_run.normal_kills(),s_run.timed_elites,s_run.guard_elites);
    svc_ui->pane_add_text(mod_ctx,left,text,nullptr);
    std::snprintf(text,sizeof(text),"Meta level %u | XP %llu / %u | Unlock Points %u (+%u this run)",
        s_records.level(),static_cast<unsigned long long>(s_records.level_xp()),s_records.next_level_xp(),
        s_records.available_points(),meta_levels(s_records.total_xp)-meta_levels(s_records.total_xp-std::min<uint64_t>(s_records.total_xp,s_run.score())));
    svc_ui->pane_add_text(mod_ctx,left,text,nullptr);
    std::snprintf(text, sizeof(text), "Sword Wave %u | Whirling Blade %u | Shield Attack %u",
        s_run.ranks[0], s_run.ranks[1], s_run.ranks[2]);
    svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
    std::snprintf(text, sizeof(text), "Bow %u | Boomerang %u | Bombs %u | Ball and Chain %u | Chests %u",
        s_run.rank(Upgrade::Bow), s_run.rank(Upgrade::Boomerang), s_run.rank(Upgrade::Bombs),
        s_run.rank(Upgrade::Ball), s_run.chests_opened);
    svc_ui->pane_add_text(mod_ctx, left, text, nullptr);
    svc_ui->pane_add_text(mod_ctx, right, s_storeFailed ?
        "Records could not be stored. Retry saving before leaving this mode." :
        "Save records before quitting. The native save screen writes your Survivors records to this mode's separate save file. "
        "Runs are not resumed mid-combat; loading returns to the start screen.", nullptr);
    auto result = button(left, "Save records", save_results);
    if (result == MOD_OK) result = button(left, "New run", replay);
    return result;
}
void open_window() {
    if (s_window || ui_visible()) return;
    UiTabDesc tabs[4] = {UI_TAB_DESC_INIT, UI_TAB_DESC_INIT, UI_TAB_DESC_INIT, UI_TAB_DESC_INIT};
    auto& tab=tabs[0];
    tab.title = s_run.phase == Phase::TreasureChoice ? (s_run.starting_item_pending ? "Choose a starting item" : "Choose a treasure") :
        s_run.phase == Phase::Upgrade ? "Choose an upgrade" : "Hyrule Survivors";
    tab.build = build_window;
    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs; desc.tab_count = 1;
    if (s_run.phase==Phase::Ready) {
        tab.title="Maps";
        tabs[1].title="Difficulty"; tabs[1].build=build_difficulty;
        tabs[2].title="Talents"; tabs[2].build=build_talents; tabs[2].update=update_talent_tab;
        tabs[3].title="Settings"; tabs[3].build=build_run_settings;
        desc.tab_count=4;
    }
    if (s_run.phase == Phase::TreasureChoice || s_run.phase == Phase::Upgrade) desc.rcss = kTreasureStyle;
    if (s_run.phase == Phase::Ready) desc.rcss = kArenaStyle;
    desc.on_closed = [](ModContext*, UiWindowHandle handle, void*) {
        if (s_window == handle) { s_window = 0; s_menuDocument = nullptr; }
        if (s_refreshChoices==handle) { s_refreshChoices=0; return; }
        if (s_run.phase == Phase::TreasureChoice && s_chosenTreasure == ChestReward::Count)
            dismiss_treasure_choice();
    };
    if (svc_ui->window_push(mod_ctx, &desc, &s_window) != MOD_OK)
        svc_log->warn(mod_ctx, "Hyrule Survivors: could not open run window");
    else if (s_run.phase == Phase::Ready) remember_menu_document(false);
}
ModResult activated(void*, ModError*) { s_active = true; reset_runtime(); return MOD_OK; }
ModResult deactivated(void*, ModError*) { s_active = false; reset_runtime(); return MOD_OK; }
ModResult new_save(void*, ModError*) {
    reset_runtime(); s_records = {}; prepare_save();
    s_saveReady = true;
    const auto bytes = s_records.encode();
    svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size());
    return MOD_OK;
}
ModResult loaded(void*, ModError*) {
    reset_runtime(); prepare_save(); s_saveReady = true; return MOD_OK;
}
ModResult reset(void*, ModError*) { reset_runtime(); return MOD_OK; }
} // namespace

bool survivors_active() { return s_active; }
bool survivors_mobility_override(ModeSetting setting,int64_t& value) {
    return talent_mode_override(s_run.talents,setting,value);
}
float survivors_sprint_multiplier() { return s_run.talents.sprint_multiplier(); }
bool survivors_dual_wield() {
    return in_arena() && s_run.owns(ChestReward::OrdonSword) && !shield_pose(daAlink_getAlinkActorClass());
}
bool survivors_arena_active() { return in_arena(); }
bool survivors_dark_link_active() {
    return in_arena() && s_run.dark_active() && (s_run.phase == Phase::Running ||
        s_run.phase == Phase::Upgrade || s_run.phase == Phase::TreasureChoice || s_run.phase == Phase::Treasure);
}
void update_survivors() {
    cleanup_enemies();
    cleanup_pots(); cleanup_pickups();
    if (s_treasure.retiring) retire_treasure_actor();
    if (!s_active || !s_gameStarted) return;
    if (s_progressSaving) { update_progress_save(); return; }
    if (s_savePrompt != kNone) {
        if (d_GameOver_CheckDelete(s_savePrompt) && dComIfGp_getGameoverStatus() == 1) {
            d_GameOver_Delete(s_savePrompt); dComIfGp_setGameoverStatus(0);
            s_savePrompt = kNone;
        }
        return;
    }
    if (!in_arena() || transitioning()) { reset_midna_dialog(); close_powerup_window(); clear_attacks(); clear_loot(); clear_enemy_effects(); s_player = kNone; s_warmup = 0; return; }
    auto* link = daAlink_getAlinkActorClass();
    if (!link || !dComIfGp_getStageStagInfo()) return;
    if (s_restart) {
        // Always use a scene reload: destroys old actors, pending spawns, collision,
        // damage animations and native drop actors before another run starts.
        retire_enemies(); clear_attacks(); clear_loot(); clear_arena_geometry();
        s_map = s_requestedMap;
        prepare_save();
        dComIfGp_setNextStage(arena().stage, arena().point, arena().room, 0);
        s_run = {}; s_orbs = {}; s_restart = false; s_player = kNone;
        s_resultsStored = false; s_spawnFailures = 0;
        return;
    }
    if (s_player != fopAcM_GetID(link)) {
        reset_midna_dialog();
        s_player = fopAcM_GetID(link); s_run.phase = Phase::Loading; s_warmup = 0;
        s_arenaSafe = s_arenaSpawn = false;
    }
    if (fpcM_IsCreating(s_player)) return;
    update_midna_dialog();
    update_arena_safety(link);
    if (s_run.phase == Phase::Loading) {
        // Never freeze a falling or partially initialized Link in the preview.
        if (!playable() || !arena_grounded(link)) { s_warmup=0; return; }
        if (++s_warmup < 15) return;
        load_records(); s_run.phase = Phase::Ready;
    }
    open_powerup_window();
    if (s_run.phase == Phase::TreasureChoice) update_treasure_choice(link);
    if (s_run.phase == Phase::Treasure) update_treasure_demo(link);
    if (s_saveRequested) {
        if (ui_visible() || dComIfGp_event_runCheck() || dMeter2Info_getGameOverType()) return;
        const auto bytes = s_records.encode();
        s_storeFailed = svc_save->set_blob(mod_ctx, kBlob, bytes.data(), bytes.size()) != MOD_OK;
        if (s_storeFailed) { s_saveRequested = false; return; }
        dComIfGs_getSaveData()->getPlayer().getPlayerReturnPlace().set(arena().stage, arena().room, arena().point);
        s_savePrompt = d_GameOver_Create(1);
        s_saveRequested = s_savePrompt == kNone;
        return;
    }
    if ((s_run.phase == Phase::TreasureChoice && s_chosenTreasure == ChestReward::Count) ||
        s_run.phase == Phase::Ready || s_run.phase == Phase::Upgrade ||
        s_run.phase == Phase::Victory || s_run.phase == Phase::Defeat) open_window();
}
ModResult initialize_survivors(ModError* error) {
    ModResult result = initialize_midna_dialog();
    if (result == MOD_OK) initialize_menu_handoff();
#define PRE(H, F) if (result == MOD_OK) result = mods::hook_add_pre<H>(svc_hook, F)
#define POST(H, F) if (result == MOD_OK) result = mods::hook_add_post<H>(svc_hook, F)
    PRE(SurvivorsArenaActor, before_arena_stage_actor);
#include "survivors_roster_hooks.inc"
    PRE(SurvivorsEnemySpeed, before_enemy_speed); POST(SurvivorsEnemySpeed, after_enemy_facing);
    PRE(SurvivorsEnemyDamage, before_enemy_damage);
    PRE(SurvivorsPotionDrink, before_potion_drink); POST(SurvivorsPotionDrink, after_potion_drink);
    PRE(SurvivorsPotionHeal, before_potion_heal);
    PRE(SurvivorsCamera, before_run_camera); POST(SurvivorsCamera, after_run_camera);
    PRE(SurvivorsArenaExit, before_arena_exit);
    PRE(SurvivorsArenaRestart, before_arena_restart);
    PRE(SurvivorsProcess, before_process); POST(SurvivorsProcess, after_process);
    POST(SurvivorsPlayer, after_player); POST(SurvivorsDraw, after_draw);
    PRE(SurvivorsPlayer, before_chest_action);
    PRE(SurvivorsPlayer, before_midna_call);
    PRE(SurvivorsMidnaOrder, before_midna_order);
    PRE(SurvivorsMidnaFlow, before_midna_flow);
    PRE(SurvivorsBlock, before_shield_block); POST(SurvivorsBlock, after_player_damage); PRE(SurvivorsGuard, before_survivors_guard);
    PRE(SurvivorsBulblinHit, before_bulblin_hit); POST(SurvivorsBulblinHit, after_bulblin_hit);
    PRE(SurvivorsBulblinAggro, before_bulblin_aggro);
    PRE(SurvivorsEnemyArrow, before_enemy_arrow); POST(SurvivorsEnemyArrow, after_enemy_arrow);
    PRE(SurvivorsBokoblinHit, before_enemy_hit); POST(SurvivorsBokoblinHit, after_enemy_hit);
    PRE(SurvivorsBokoblinTargets, before_bokoblin_targets);
    PRE(SurvivorsRatRecovery, before_rat_recovery);
    POST(SurvivorsRatRecovery, after_rat_recovery);
    PRE(SurvivorsBokoblinAggro, before_bokoblin_aggro);
    PRE(SurvivorsBokoblinWait, before_bokoblin_wait);
    PRE(SurvivorsPotBreak, before_pot_break);
    POST(SurvivorsEliteMinimap, after_elite_minimap);
    PRE(SurvivorsKeeseAggro, before_keese_aggro);
    PRE(SurvivorsRatAggro, before_rat_aggro);
    PRE(SurvivorsKeeseHit, before_enemy_hit); POST(SurvivorsKeeseHit, after_enemy_hit);
    PRE(SurvivorsRatHit, before_enemy_hit); POST(SurvivorsRatHit, after_enemy_hit);
    PRE(SurvivorsMovement, before_auto_movement); POST(SurvivorsMovement, after_auto_movement);
    PRE(SurvivorsLedgeJump, before_survivors_ledge_jump); POST(SurvivorsLedgeJump, after_survivors_ledge_jump);
    POST(SurvivorsPosition, after_arena_position);
    POST(SurvivorsPosition, after_auto_position);
    PRE(SurvivorsRangedAim, before_ranged_aim);
    PRE(SurvivorsBowAction, before_ranged_native);
    PRE(SurvivorsBowNext, before_ranged_native);
    PRE(SurvivorsBallAction, before_ranged_native);
    PRE(SurvivorsBallNext, before_ranged_native);
    POST(SurvivorsScrollVisible, after_powerup_scroll_visible);
    PRE(SurvivorsCollectionWait, before_powerup_collection_wait);
    POST(SurvivorsCollectionWait, after_powerup_collection_action);
    POST(SurvivorsCollectionClick, after_powerup_collection_action);
    PRE(SurvivorsCollectionDelete, before_powerup_collection_delete);
    PRE(SurvivorsScrollTexture, before_powerup_icon);
    PRE(SurvivorsRingCursor, before_ring_assignment);
    PRE(SurvivorsRingAssign, before_ring_assignment);
    PRE(SurvivorsRingMix, before_ring_assignment);
    PRE(SurvivorsBind, before_item_binding);
    PRE(SurvivorsMixBind, before_item_mix);
    POST(SurvivorsExplain, after_item_explain);
    PRE(SurvivorsTreasureText, before_treasure_text_update);
    PRE(SurvivorsBombThrow, before_ranged_native);
    PRE(SurvivorsRangedCancel, before_ranged_native);
    PRE(SurvivorsTreasureDraw, before_treasure_draw);
    PRE(SurvivorsGetItem, before_treasure_get); POST(SurvivorsGetItem, after_treasure_get);
    PRE(SurvivorsDeath, before_death); PRE(SurvivorsManualAttack, before_cut);
    PRE(SurvivorsSwordAttack, before_sword_attack); PRE(SurvivorsDoor, before_cut);
    POST(SurvivorsPower, after_power);
    PRE(SurvivorsHitMaterial, before_hit_material); POST(SurvivorsHitMaterial, after_hit_material);
    POST(SurvivorsLightSword, after_light_sword);
    POST(SurvivorsJumpStrike, after_jump_strike); POST(SurvivorsJumpStrikeLand, after_jump_strike);
    PRE(SurvivorsJumpStrikeEffect, before_jump_strike_effect);
    POST(SurvivorsSpinMove, after_sword_move); PRE(SurvivorsLargeSpin, before_large_spin);
    POST(SurvivorsNormalMove, after_sword_move); POST(SurvivorsFinishMove, after_sword_move);
    PRE(SurvivorsStage, before_stage); POST(SurvivorsHud, draw_hud);
    PRE(SurvivorsGameStart, before_game_start); PRE(SurvivorsOpening, before_opening);
    PRE(SurvivorsDrops, before_drop); PRE(SurvivorsDirectDrops, before_drop);
#undef PRE
#undef POST
    if (result != MOD_OK) return mods::set_error(error, result, "failed to install Hyrule Survivors hooks");
    const GameModeDesc mode{
        .struct_size = sizeof(GameModeDesc), .game_mode_id = kMode,
        .full_name = "Hyrule Survivors", .save_name = "gczelda2-survivors", .user_data = nullptr,
        .on_activated = activated, .on_deactivated = deactivated, .on_play = activated,
        .on_save_loaded = loaded, .on_new_save = new_save, .on_new_save_select = nullptr,
        .on_game_reset = reset, .on_tick = nullptr,
    };
    result = svc_game_mode->register_game_mode(mod_ctx, &mode);
    if (result != MOD_OK) return mods::set_error(error, result, "failed to register Hyrule Survivors");
    s_registered = true;
    return MOD_OK;
}
void shutdown_survivors() {
    s_active = false; reset_runtime(); cleanup_enemies(); cleanup_pots(); cleanup_pickups();
    s_midnaGraph.reset(); s_midnaPrompt.reset(); s_midnaOptions.reset(); s_midnaStart=0;
    s_treasureMessage.reset(); s_itemNameMessage.reset(); s_itemDescriptionMessage.reset();
    if (s_savePrompt != kNone) { d_GameOver_Delete(s_savePrompt); s_savePrompt = kNone; }
    if (s_registered) svc_game_mode->unregister_game_mode(mod_ctx, kMode);
    s_registered = false;
    // Loader removes callbacks after shutdown. No native actor profile points at
    // this module; outstanding deletes belong to the scene and remain safe.
}
} // namespace dawnlight

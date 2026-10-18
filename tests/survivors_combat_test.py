"""Run production Survivors combat orchestration against observable engine boundaries."""
from pathlib import Path
import re
import os
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()
sdk = Path(sys.argv[1] if len(sys.argv) > 1 else root / 'dusklight')
native = (sdk / 'src/d/actor/d_a_alink_cut.inc').read_text()


def extract(name, text=source):
    match = re.search(r'^(?:void|bool|int|HookAction) ' + re.escape(name) + r'\([^;{}]*\)\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.start())
    depth = 0
    for end in range(brace, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if not depth:
            return text[match.start():end + 1]
    raise AssertionError(name)


stubs = r'''
#include <any>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using u32=unsigned; using u16=unsigned short; using BOOL=int; using fpc_ProcID=unsigned;
constexpr BOOL FALSE=0,TRUE=1;
constexpr int BUTTON_STATUS_PUT_AWAY=1,BUTTON_STATUS_NONE=0;
int dComIfGp_getDoStatus(){return BUTTON_STATUS_NONE;}
constexpr unsigned kNone=~0u;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {
template<class T> T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}
}
struct cXyz {
    float x=0,y=0,z=0;
    float abs(const cXyz& p)const{return std::sqrt((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));}
    cXyz operator+(cXyz p)const{return {x+p.x,y+p.y,z+p.z};}
    cXyz operator-(cXyz p)const{return {x-p.x,y-p.y,z-p.z};}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
struct csXyz {short x=0,y=0,z=0; csXyz(short a=0,short b=0,short c=0):x(a),y(b),z(c){}};
short cM_atan2s(float a,float b){return short(std::atan2(a,b)*32768/3.14159265);}
struct cCcD_Obj {
    float radius=0;void SetR(float r){radius=r;}
    unsigned power=1; bool active=true,hit=true;
    unsigned GetAtAtp()const{return power;}
    void OffAtSetBit(){active=false;} void ResetAtHit(){hit=false;}
};
struct fopAc_ac_c {unsigned id=7;int health=10;cXyz eyePos;struct {cXyz pos;csXyz angle;}current;};
constexpr int dItemNo_NONE_e=255,dItemNo_HEART_e=0,dItemNo_GREEN_RUPEE_e=1,dItemNo_BLUE_RUPEE_e=2,dItemNo_YELLOW_RUPEE_e=3,dItemNo_RED_RUPEE_e=4,dItemNo_PURPLE_RUPEE_e=5,dItemNo_ORANGE_RUPEE_e=6,dItemNo_SILVER_RUPEE_e=7;
HookAction before_sword_attack(ModContext*,void*,void*,void*);
struct daAlink_c : fopAc_ac_c {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,
          PROC_WAIT_TURN,PROC_MOVE_TURN,PROC_CUT_NORMAL,PROC_CUT_TURN,PROC_CUT_FINISH,PROC_DAMAGE,PROC_JUMP,PROC_DODGE,PROC_CUT_LARGE_JUMP,PROC_CUT_LARGE_JUMP_LAND};
    int mProcID=PROC_WAIT,mEquipItem=0x103,field_0x2fde=0;
    bool wolf=false,model=true; void* mSwordModel=&model;
    struct {bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;
    struct Frame {float frame=0,rate=1;float getFrame(){return frame;}void setRate(float r){rate=r;}}mUnderFrameCtrl[1],mUpperFrameCtrl[1];
    int field_0x3198=2;
    int procCutLargeJumpInit(){mProcID=PROC_CUT_LARGE_JUMP;return 1;}
    float field_0x3478=0,field_0x347c=10,field_0x3484=15,mNormalSpeed=0,mStickValue=0;short mMoveAngle=0;csXyz shape_angle;
    struct HIO {struct {struct {float mMaxSpeed=20;}m;}mMove;}hio;
    HIO* mpHIO=&hio;
    cCcD_Obj mAtCps[3],mAtSph,mGuardAtCps;int tevStr=0;
    unsigned normals=0,spins=0,equips=0,finishes=0,mComboCutCount=0;
    enum {FLG0_UNK_1000000,FLG0_UNK_8000,DIR_FORWARD=0,DIR_BACKWARD=1};
    struct {int field_0x300a=4;}mProcVar1;
    struct {short field_0x300c=0;}mProcVar2;
    int direction=4;bool locked=false,hit=false;
    std::vector<int> normalTypes,finishTypes,comboCounts;
    int commonCutAction();int nativeCheckCutAction();
    int checkCutAction(){
        int result=0;
        if(before_sword_attack(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL)return result;
        return nativeCheckCutAction();
    }
    void resetCombo(BOOL){mComboCutCount=0;}
    bool checkReinRide(){return false;}void procHorseCutInit(){assert(false);}
    void setDoStatus(int){}int getCutDirection(){return direction;}
    bool checkNoResetFlg0(int flag){return flag==FLG0_UNK_8000 && hit;}
    bool checkInputOnR(){return false;}bool checkAttentionLock(){return locked;}
    bool checkWolf(){return wolf;} bool checkItemSwordEquip(){return mEquipItem==0x103;}
    void swordEquip(int){++equips;field_0x2fde=0x103;}
    int procCutNormalInit(int type){normalTypes.push_back(type);comboCounts.push_back(mComboCutCount);
        ++normals;mProcID=PROC_CUT_NORMAL;mProcVar1.field_0x300a=4;
        field_0x3478=5;mUnderFrameCtrl[0].frame=0;return 1;}
    int procCutFinishInit(int type){finishTypes.push_back(type);comboCounts.push_back(mComboCutCount);
        ++finishes;mProcID=PROC_CUT_FINISH;field_0x3478=5;mUnderFrameCtrl[0].frame=0;return 1;}
    int procCutTurnInit(int quick,int dir){assert(quick==1 && dir==1);++spins;
        mProcID=PROC_CUT_TURN;field_0x3478=220;field_0x347c=130;return 1;}
} player;
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
int rewardRoom=51;
int fopAcM_GetRoomNo(fopAc_ac_c*){return rewardRoom;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
Run s_run;
enum class SwordAction {None,Normal,Spin,Jump};
SwordAction s_swordAction=SwordAction::None;
bool s_wavePending=false,s_allowDrop=false,s_active=true,arena=true,playing=true;
unsigned s_meleeCooldown=0,s_player=7,s_comboStep=0,s_comboTimeout=0;
bool s_autoSwordCall=false,s_trackMelee=false;unsigned s_meleeTarget=kNone;
std::array<unsigned,unsigned(Upgrade::Rupees)>s_cooldowns{};
bool in_arena(){return arena;} bool playable(){return arena && playing;}
struct Enemy {unsigned id=kNone;bool initialized=false,retiring=false,credited=false;};
std::array<Enemy,enemy_capacity>s_enemies;
Enemy* tracked(unsigned id){for(auto& e:s_enemies)if(e.id==id&&e.initialized)return &e;return nullptr;}
bool abilityBusy=false;bool jump_abilities_busy_for(daAlink_c*){return abilityBusy;}
fopAc_ac_c enemy,enemy2;
fopAc_ac_c potActor; bool potCarried=false;
struct ArenaPot{unsigned id=kNone;bool initialized=false,retiring=false,broken=false;};
std::array<ArenaPot,1>s_pots;
bool fopAcM_checkCarryNow(fopAc_ac_c*){return potCarried;}bool fopAcM_checkHookCarryNow(fopAc_ac_c*){return false;}
fopAc_ac_c* actor_by_id(unsigned id){return id==enemy.id?&enemy:id==enemy2.id?&enemy2:id==potActor.id?&potActor:nullptr;}
struct dBgS_LinChk {unsigned unused=0;void Set(const cXyz*,const cXyz*,daAlink_c*){}};
bool blocked=false;
struct World{bool LineCross(dBgS_LinChk*){return blocked;}}world;
World& dComIfG_Bgsp(){return world;}
cXyz target_direction(daAlink_c*){return {0,0,1};}
unsigned waves=0;
void launch(daAlink_c*,unsigned weapon,cXyz){assert(weapon==0);++waves;}
struct Emitter {
    int mMaxFrame=5,age=0;bool stopped=false,quit=false,invalid=false;cXyz position;
    int getAge(){return age;}void stopCreateParticle(){stopped=true;}
    void quitImmortalEmitter(){quit=true;}void becomeInvalidEmitter(){invalid=true;}
    void setGlobalTranslation(cXyz p){position=p;}
};
std::map<u32,Emitter>emitters;unsigned nextEmitter=1;
constexpr u16 ID_ZI_J_KAITENGIRI_A=11,ID_ZI_J_KAITENGIRI_B=12,ID_ZI_J_BOMBex_A=0x161,ID_ZI_J_BOMBex_B=0x162;
std::vector<u16>particles;std::vector<float>particleScales;
Emitter* dComIfGp_particle_getEmitter(u32 id){auto it=emitters.find(id);return it==emitters.end()?nullptr:&it->second;}
u32 dComIfGp_particle_set(u32 id,u16 effect,cXyz*,int*,const csXyz*,const cXyz* scale,int,void*,int,void*,void*,void*){
    assert(effect==ID_ZI_J_KAITENGIRI_A || effect==ID_ZI_J_KAITENGIRI_B || effect==0x161 || effect==0x162);
    particles.push_back(effect);particleScales.push_back(scale?scale->x:1);
    if(!id){id=nextEmitter++;emitters.emplace(id,Emitter{});}return id;
}
void dComIfGp_particle_levelEmitterOnEventMove(u32){}
struct Attack {bool active=true;unsigned life=10;cXyz position,velocity;cCcD_Obj collider;std::array<u32,2>emitters{};};
std::array<Attack,projectile_capacity>s_attacks;
struct Effect{cCcD_Obj collider;unsigned damage=0;};std::array<Effect,24>s_effects;
void new_melee_swing(){}
constexpr int HIT_TYPE_LINK_HEAVY_ATTACK=9;
struct dCcU_AtInfo {cCcD_Obj* mpCollider;unsigned mAttackPower=200;int mHitType=0;};
HookAction before_drop(ModContext*,void*,void*,void*);
std::vector<int>drops;
fpc_ProcID fopAcM_createItem(const cXyz*,int item,int bit,int room,void*,void*,int mode){
    fpc_ProcID result=0;
    assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
    assert(bit==-1 && room==rewardRoom && mode==0);drops.push_back(item);return 1;
}
unsigned maxLife=15,rupees=0,life=4;
unsigned dComIfGs_getMaxLife(){return maxLife;}
void dComIfGs_setMaxLife(unsigned v){maxLife=v;}
unsigned dComIfGs_getRupeeMax(){return 300;}unsigned dComIfGs_getRupee(){return rupees;}
void dComIfGs_setRupee(unsigned v){rupees=v;}
void close_window(){}
fpc_ProcID create_run_pickup(cXyz pos,int item){
 const bool old=s_allowDrop;s_allowDrop=true;const auto id=fopAcM_createItem(&pos,item,-1,rewardRoom,nullptr,nullptr,0);s_allowDrop=old;return id;}

'''
stubs += r'''
bool reuse_bulblin_power(dCcU_AtInfo*){return false;}
struct BulblinPowerCache{dCcU_AtInfo* info;~BulblinPowerCache(){}};
'''
main = r'''
void reset(){player.id=7;playing=arena=true;abilityBusy=false;s_pots={};potActor.id=99;potCarried=false;
    player.mProcID=daAlink_c::PROC_WAIT;player.mEquipItem=0x103;
    player.mLinkAcch.ground=true;player.wolf=false;player.mSwordModel=&player.model;
    player.normals=player.spins=player.equips=player.finishes=player.mComboCutCount=0;
    player.normalTypes.clear();player.finishTypes.clear();player.comboCounts.clear();
    player.direction=4;player.locked=player.hit=false;
    s_comboStep=s_comboTimeout=0;s_autoSwordCall=s_trackMelee=false;s_meleeTarget=kNone;blocked=false;
    enemy.health=10;enemy2.id=11;player.current.pos={};
    s_run.start(1234);s_swordAction=SwordAction::None;s_wavePending=false;
    s_meleeCooldown=0;s_cooldowns={};waves=0;
    enemy.id=10;enemy.current.pos={0,0,150};s_enemies={};s_enemies[0]={10,true,false,false};
}
int main(){
    reset();update_sword(&player);
    assert(player.normals==1 && player.spins==0 && waves==0 && !s_wavePending);
    player.mUnderFrameCtrl[0].frame=6;update_sword(&player);assert(waves==0);
    // Run the native directional selector through two complete four-hit combos.
    // Recovery can clear the native count; automatic cadence must preserve it.
    for(bool recoverToIdle:{false,true}){
        reset();update_sword(&player);
        for(unsigned hit=1;hit<8;++hit){
            player.mUnderFrameCtrl[0].frame=16;
            s_meleeCooldown=0;
            if(recoverToIdle || hit%4==0){player.mProcID=daAlink_c::PROC_WAIT;player.mComboCutCount=0;}
            update_sword(&player);assert(!s_autoSwordCall);
        }
        assert((player.comboCounts==std::vector<int>{1,2,3,4,1,2,3,4}));
        assert((player.normalTypes==std::vector<int>{CUT_NM_PARAM_LEFT,CUT_NM_PARAM_RIGHT,CUT_NM_PARAM_LEFT,
                                                  CUT_NM_PARAM_LEFT,CUT_NM_PARAM_RIGHT,CUT_NM_PARAM_LEFT}));
        assert((player.finishTypes==std::vector<int>{CUT_FINISH_PARAM_VERTICAL,CUT_FINISH_PARAM_VERTICAL}));
    }
    // The third-hit collision variant is a native finisher animation, but must
    // still chain into hit four and carry its scheduled Sword Wave.
    reset();player.hit=true;update_sword(&player);
    for(unsigned i=0;i<2;++i){player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);}
    assert(player.mProcID==daAlink_c::PROC_CUT_FINISH && s_comboStep==3);
    s_wavePending=true;player.mUnderFrameCtrl[0].frame=5;update_sword(&player);assert(waves==1);
    player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);
    assert(player.mComboCutCount==4 && s_comboStep==0);
    // Final recovery is preserved, even at zero cooldown.
    player.mUnderFrameCtrl[0].frame=16;s_meleeCooldown=0;update_sword(&player);assert(player.mComboCutCount==4);
    // Attack/cancel windows, interruption, timeout, and manual-input isolation.
    reset();update_sword(&player);s_meleeCooldown=0;
    player.mUnderFrameCtrl[0].frame=10;update_sword(&player);assert(player.normals==1);
    player.mUnderFrameCtrl[0].frame=15;update_sword(&player);assert(player.normals==1);
    player.mUnderFrameCtrl[0].frame=16;update_sword(&player);assert(player.normals==2);
    player.mProcID=daAlink_c::PROC_DAMAGE;update_sword(&player);assert(s_comboStep==0);
    player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;update_sword(&player);assert(player.mComboCutCount==1);
    player.mProcID=daAlink_c::PROC_WAIT;enemy.current.pos.z=2000;
    for(unsigned i=0;i<2*ticks_per_second;++i)update_sword(&player);
    enemy.current.pos.z=150;update_sword(&player);assert(player.mComboCutCount==1);
    auto count=player.normals;assert(!player.checkCutAction() && player.normals==count);
    // Ordinary sword combo targets nearby unbroken pots when no enemy is reachable.
    reset();s_enemies={};potActor.current.pos={0,0,150};s_pots[0]={99,true,false,false};
    update_sword(&player);assert(player.normals==1&&s_meleeTarget==99);
    player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;s_pots[0].broken=true;
    update_sword(&player);assert(player.normals==1);
    s_pots[0].broken=false;potCarried=true;update_sword(&player);assert(player.normals==1);
    potCarried=false;potActor.current.pos.y=200;update_sword(&player);assert(player.normals==1);
    // Movement is restored after every native normal/spin/finish procedure.
    // It changes travel direction, while retaining the sword-facing angle.
    for(int proc:{daAlink_c::PROC_CUT_NORMAL,daAlink_c::PROC_CUT_FINISH,daAlink_c::PROC_CUT_TURN}){
        reset();player.mProcID=proc;player.shape_angle.y=123;
        s_swordAction=proc==daAlink_c::PROC_CUT_TURN?SwordAction::Spin:SwordAction::Normal;
        std::any moveArgs[]={&player};
        for(float stick:{0.0f,0.5f,1.0f})for(short angle:{short(-16000),short(16000)}){
            player.mNormalSpeed=0;player.mStickValue=stick;player.mMoveAngle=angle;
            after_sword_move(nullptr,moveArgs,nullptr,nullptr);
            assert(player.mNormalSpeed==20*stick && player.current.angle.y==angle && player.shape_angle.y==123);
        }
        for(int reason=0;reason<4;++reason){
            player.mNormalSpeed=7;player.mProcID=reason==0?daAlink_c::PROC_DAMAGE:proc;
            s_run.phase=reason==1?Phase::Upgrade:Phase::Running;
            player.mLinkAcch.ground=reason!=2;arena=reason!=3;
            after_sword_move(nullptr,moveArgs,nullptr,nullptr);assert(player.mNormalSpeed==7);
        }
        arena=true;
    }
    // No attacks against dead, retiring, pending or absent targets.
    for(int reason=0;reason<4;++reason){
        reset();s_enemies[0].credited=reason==0;s_enemies[0].retiring=reason==1;
        s_enemies[0].initialized=reason!=2;if(reason==3)s_enemies[0].id=99;
        update_sword(&player);assert(player.normals==0);
    }
    reset();enemy.current.pos.z=900;update_sword(&player);assert(player.normals==0);
    // Drawing a sheathed sword is requested once and the native equip must finish.
    reset();player.mEquipItem=dItemNo_NONE_e;player.field_0x2fde=0;
    update_sword(&player);update_sword(&player);assert(player.equips==1 && player.normals==0);
    player.mEquipItem=0x103;update_sword(&player);assert(player.normals==1);
    // Idle locomotion can attack; damage, dodges and jumps must keep their procedure.
    for(int proc:{daAlink_c::PROC_DAMAGE,daAlink_c::PROC_JUMP,daAlink_c::PROC_DODGE}){
        reset();player.mProcID=proc;s_run.ranks[0]=s_run.ranks[1]=1;update_sword(&player);
        assert(player.mProcID==proc && player.normals==0 && player.spins==0);
    }
    for(int reason=0;reason<4;++reason){
        reset();if(reason==0)player.mLinkAcch.ground=false;if(reason==1)player.wolf=true;
        if(reason==2)player.mSwordModel=nullptr;
        if(reason==3)player.mEquipItem=2;
        update_sword(&player);assert(player.normals==0);
    }
    // Wave unlock adds a real swing, then fires once at its attack frame (never on windup).
    for(unsigned rank:{1u,3u,5u}){
        reset();s_run.ranks[0]=rank;enemy.current.pos.z=900;update_sword(&player);
        assert(player.normals==1 && waves==0 && s_wavePending);
        player.mUnderFrameCtrl[0].frame=4;update_sword(&player);assert(waves==0);
        player.mUnderFrameCtrl[0].frame=5;update_sword(&player);
        unsigned expected=(rank+1)/2;assert(waves==expected && !s_wavePending);
        update_sword(&player);assert(waves==expected && player.normals==1);
    }
    reset();s_run.ranks[0]=1;enemy.current.pos.z=900;
    for(unsigned shot=0;shot<4;++shot){
        player.mProcID=daAlink_c::PROC_WAIT;s_cooldowns[0]=0;s_comboStep=3;player.mComboCutCount=3;
        update_sword(&player);assert(player.normalTypes.back()==1&&player.mComboCutCount==0&&s_comboStep==0);
        player.mUnderFrameCtrl[0].frame=5;update_sword(&player);
    }
    assert(waves==4&&player.finishes==0);
    reset();s_run.ranks[0]=1;update_sword(&player);player.mProcID=daAlink_c::PROC_DAMAGE;
    player.mUnderFrameCtrl[0].frame=9;update_sword(&player);assert(waves==0 && !s_wavePending);
    // Native spin has priority, receives Reach and follows the stick at normal speed.
    reset();s_run.ranks[0]=s_run.ranks[1]=1;s_run.ranks[5]=2;
    update_sword(&player);assert(player.spins==1 && player.normals==0 && waves==0);
    assert(player.field_0x3478==220&&std::abs(player.mAtSph.radius-169)<0.01);
    for(unsigned reach:{0u,5u,30u}){
        s_run.ranks[unsigned(Upgrade::Reach)]=reach;
        for(float nativeRadius:{130.f,170.f,220.f}){
            player.field_0x347c=nativeRadius;
            std::any spinArgs[]={&player};after_sword_move(nullptr,spinArgs,nullptr,nullptr);
            assert(std::abs(player.mAtSph.radius-s_run.radius(nativeRadius))<.01f);
            after_sword_move(nullptr,spinArgs,nullptr,nullptr); // No cumulative multiplication.
            assert(std::abs(player.mAtSph.radius-s_run.radius(nativeRadius))<.01f);
        }
    }
    std::any args[]={&player};
    for(float stick:{0.0f,0.5f,1.0f})for(short angle:{short(-16000),short(16000)}){
        player.mStickValue=stick;player.mMoveAngle=angle;
        after_sword_move(nullptr,args,nullptr,nullptr);
        assert(player.current.angle.y==angle && player.mNormalSpeed==20*stick);
    }
    playing=false;player.mNormalSpeed=9;after_sword_move(nullptr,args,nullptr,nullptr);assert(player.mNormalSpeed==9);
    playing=true;player.id=8;after_sword_move(nullptr,args,nullptr,nullptr);assert(player.mNormalSpeed==9);player.id=7;
    BOOL large=1;assert(before_large_spin(nullptr,nullptr,&large,nullptr)==HOOK_SKIP_ORIGINAL && large==0);
    arena=false;assert(before_large_spin(nullptr,nullptr,&large,nullptr)==HOOK_CONTINUE);arena=true;
    // Only this Link's active native sword colliders receive linear Survivors damage.
    s_run.ranks[3]=2;dCcU_AtInfo info{&player.mAtSph};std::any powerArgs[]={&info};
    after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);
    s_swordAction=SwordAction::Normal;
    for(auto& capsule:player.mAtCps){info={&capsule};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);}
    s_run.master_sword=true;
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==8);
    assert(info.mHitType==HIT_TYPE_LINK_HEAVY_ATTACK); // Native Master Sword multiplier must not apply again.
    assert(s_run.attack_power(0)==7+s_run.ranks[0]);
    s_swordAction=SwordAction::Spin;info={&player.mAtSph};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==s_run.attack_power(1));s_run.master_sword=false;
    cCcD_Obj unrelated;info={&unrelated};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200 && info.mHitType==0);
    arena=false;info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200);arena=true;
    // Native sword/spin/shield and every projectile use one critical roll per hit.
    s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::CritDamage)]=30;
    s_run.dark_link=1;s_run.dark_remaining=150;
    auto checkCritical=[&](cCcD_Obj& collider,unsigned base){
        s_run.random=1;info={&collider};after_power(nullptr,powerArgs,nullptr,nullptr);
        assert(info.mAttackPower==((base*350+50)/100)*2);
        assert(s_run.random==270369); // one roll, no second Master Sword/Dark multiplier
    };
    s_swordAction=SwordAction::Normal;checkCritical(player.mAtCps[0],s_run.melee_power());
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==s_run.melee_power()*2&&s_run.random==67634689); // next target can miss its crit
    s_swordAction=SwordAction::Spin;checkCritical(player.mAtSph,s_run.attack_power(1));
    checkCritical(player.mGuardAtCps,s_run.attack_power(unsigned(Upgrade::ShieldAttack)));
    for(unsigned base:{4u,5u,6u,7u,8u}){s_attacks[0].collider.power=base;checkCritical(s_attacks[0].collider,base);}
    info={&unrelated};const auto critRng=s_run.random;after_power(nullptr,powerArgs,nullptr,nullptr);
    assert(info.mAttackPower==200&&s_run.random==critRng);
    s_run.ranks[unsigned(Upgrade::CritChance)]=s_run.ranks[unsigned(Upgrade::CritDamage)]=0;
    s_run.dark_link=s_run.dark_remaining=0;
    // Expired and missing emitters restart; all surviving emitters are invalidated on cleanup.
    Attack a;update_wave_effects(a,&player);auto first=a.emitters;
    a.position={10,20,30};update_wave_effects(a,&player);
    assert(a.emitters==first && emitters[first[0]].position.x==10);
    emitters[first[0]].age=5;emitters.erase(first[1]);update_wave_effects(a,&player);
    assert(a.emitters[0]!=first[0] && a.emitters[1]!=first[1] && emitters[first[0]].invalid);
    auto last=a.emitters;stop_attack(a);
    assert(!a.active && !a.collider.active && !a.collider.hit && a.emitters[0]==0 && a.emitters[1]==0);
    for(auto id:last)assert(emitters[id].stopped && emitters[id].quit && emitters[id].invalid);
    stop_attack(a); // Repeated cleanup is safe.
    // Circle a target through a whole turn at full running speed. Facing is
    // recomputed from the moved position every frame; travel remains untouched.
    reset();update_sword(&player);assert(s_trackMelee&&s_meleeTarget==enemy.id);
    for(unsigned frame=0;frame<120;++frame){
        const float a=frame*6.2831853f/120;
        player.current.pos={150*std::sin(a),0,150+150*std::cos(a)};
        player.current.angle.y=player.mMoveAngle=short(1234+frame);player.mNormalSpeed=20;
        track_melee_facing(&player);
        const auto delta=enemy.current.pos-player.current.pos;
        assert(player.shape_angle.y==cM_atan2s(delta.x,delta.z));
        assert(player.mProcVar2.field_0x300c==player.shape_angle.y);
        assert(player.current.angle.y==short(1234+frame)&&player.mMoveAngle==short(1234+frame)&&player.mNormalSpeed==20);
    }
    // A slightly nearer bystander does not steal a live lock mid-swing.
    player.current.pos={};enemy.current.pos={0,0,150};enemy2.current.pos={60,0,40};
    s_enemies[1]={11,true,false,false};track_melee_facing(&player);
    assert(s_meleeTarget==10&&player.shape_angle.y==0);
    enemy.health=0;track_melee_facing(&player);assert(s_meleeTarget==11&&player.shape_angle.y==cM_atan2s(60,40));
    player.mProcID=daAlink_c::PROC_CUT_FINISH;player.mProcVar2.field_0x300c=9;
    enemy2.current.pos={-80,0,40};track_melee_facing(&player);
    assert(player.shape_angle.y==cM_atan2s(-80,40)&&player.mProcVar2.field_0x300c==9);
    blocked=true;track_melee_facing(&player);assert(s_meleeTarget==kNone);blocked=false;
    enemy2.current.pos={0,250,30};assert(!melee_target(&player));
    enemy2.current.pos={0,0,1000};assert(!melee_target(&player));
    enemy2.current.pos={80,0,40};
    for(int reason=0;reason<6;++reason){
        player.mProcID=reason==0?daAlink_c::PROC_DAMAGE:daAlink_c::PROC_CUT_NORMAL;
        s_trackMelee=reason!=1;s_run.phase=reason==2?Phase::Upgrade:Phase::Running;
        arena=reason!=3;player.mLinkAcch.ground=reason!=4;player.id=reason==5?8:7;
        player.shape_angle.y=123;track_melee_facing(&player);assert(player.shape_angle.y==123);
    }
    reset();
    // Automatic Jump Strike owns only its native jump/landing poses and damage.
    reset();s_run.master_sword=1;s_run.ranks[unsigned(Upgrade::JumpStrike)]=1;
    for(unsigned i=0;i<100&&s_swordAction!=SwordAction::Jump;++i){
        player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;update_sword(&player);
    }
    assert(s_swordAction==SwordAction::Jump&&player.mProcID==daAlink_c::PROC_CUT_LARGE_JUMP);
    assert(s_cooldowns[unsigned(Upgrade::JumpStrike)]==180&&!s_wavePending&&!s_trackMelee);
    assert(player.mUnderFrameCtrl[0].rate==2&&player.mUpperFrameCtrl[0].rate==2);
    info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==6);
    player.mProcID=daAlink_c::PROC_CUT_LARGE_JUMP_LAND;player.field_0x3478=300;
    particles.clear();particleScales.clear();s_run.ranks[unsigned(Upgrade::Reach)]=5;
    assert(before_jump_strike_effect(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    assert((particles==std::vector<u16>{0x161,0x162}));
    assert(std::abs(particleScales[0]-2.625f)<.01f&&particleScales[0]==particleScales[1]);
    for(unsigned rank:{1u,30u}){
        s_run.ranks[unsigned(Upgrade::JumpStrike)]=rank;
        info={&player.mAtSph};after_power(nullptr,powerArgs,nullptr,nullptr);
        assert(info.mAttackPower==s_run.jump_splash_power());
    }
    s_run.ranks[unsigned(Upgrade::CritChance)]=30;s_run.ranks[unsigned(Upgrade::CritDamage)]=30;
    s_run.dark_link=1;s_run.dark_remaining=150;checkCritical(player.mAtSph,s_run.jump_splash_power());
    for(float radius:{130.f,200.f,300.f}){
        player.field_0x347c=radius;player.mUnderFrameCtrl[0].rate=1;
        after_jump_strike(nullptr,args,nullptr,nullptr);
        assert(player.mAtSph.radius==s_run.radius(radius)&&player.mUnderFrameCtrl[0].rate==2);
    }
    const auto jumpParticles=s_jumpEmitters;clear_jump_effects();clear_jump_effects();
    for(auto id:jumpParticles)assert(emitters[id].invalid);
    abilityBusy=true;assert(!update_jump_strike(&player));
    assert(before_jump_strike_effect(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);abilityBusy=false;
    player.mProcID=daAlink_c::PROC_DAMAGE;assert(!update_jump_strike(&player)&&s_swordAction==SwordAction::None);
    assert(before_jump_strike_effect(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    // Ordinary attacks still work when the jump's cooldown has not elapsed.
    player.mProcID=daAlink_c::PROC_WAIT;s_meleeCooldown=0;update_sword(&player);
    assert(s_swordAction==SwordAction::Normal);
    reset();
    // Native random drops stay suppressed; only scoped reward creation bypasses the hook.
    fpc_ProcID result=0;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==kNone);
    bool seen[8]{};
    for(int room : {51,6,0,50}) {
        rewardRoom=room;
        for(int i=0;i<25000;++i){drop_reward(&enemy);assert(!s_allowDrop);}
    }
    for(auto item:drops){assert(item>=0 && item<=7);seen[item]=true;}
    for(bool found:seen)assert(found);
    arena=false;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);arena=true;
    // Heart Container raises capacity only; fallback currency clamps to the wallet.
    reset();unsigned choice=0;s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Heart;
    maxLife=17; // Preserve two collected heart pieces when adding a container.
    select_upgrade(nullptr,&choice);assert(maxLife==22 && life==4);
    s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Rupees;rupees=290;
    select_upgrade(nullptr,&choice);assert(rupees==300 && life==4);
    std::cout<<"Survivors combat: native four-hit combos, sword movement, wave timing, particles and rewards passed\n";
}
'''
functions = ['stop_attack', 'update_wave_effects', 'sword_action_available', 'enemy_in_range',
             'before_cut', 'before_sword_attack', 'normal_sword_proc', 'targeting', 'begin_sword_combo',
             'release_sword_wave', 'jump_strike', 'update_sword', 'after_sword_move', 'before_large_spin',
             'native_sword_collider', 'after_power', 'before_drop', 'drop_reward', 'select_upgrade']
# Execute Dusklight's actual combo counter/selector, not a reimplementation of
# the sequence. Animation setup remains an observable boundary above.
native_combo = native[native.index('enum daAlink_CutNmParamType'):native.index('static fopAc_ac_c*')]
native_combo_functions = extract('daAlink_c::commonCutAction', native) + '\n' + extract(
    'daAlink_c::checkCutAction', native).replace('daAlink_c::checkCutAction', 'daAlink_c::nativeCheckCutAction')
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(native_combo + stubs + native_combo_functions + '\n'.join((root/'src/survivors_targeting.inc').read_text() if name=='targeting' else (root/'src/survivors_jump_strike.inc').read_text() if name=='jump_strike' else extract(name) for name in functions) + '\n' + main)
    subprocess.run([os.environ.get('CXX','c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

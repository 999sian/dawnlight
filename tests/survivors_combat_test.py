"""Run production Survivors combat orchestration against observable engine boundaries."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()


def extract(name):
    match = re.search(r'^(?:void|bool|HookAction) ' + name + r'\(', source, re.M)
    assert match, name
    brace = source.index('{', match.start())
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[match.start():end + 1]
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
constexpr BOOL FALSE=0;
constexpr unsigned kNone=~0u;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {
template<class T> T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}
}
struct cXyz {
    float x=0,y=0,z=0;
    float abs(const cXyz& p)const{return std::sqrt((x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)+(z-p.z)*(z-p.z));}
    float absXZ()const{return std::sqrt(x*x+z*z);}
};
struct csXyz {short x=0,y=0,z=0; csXyz(short a=0,short b=0,short c=0):x(a),y(b),z(c){}};
short cM_atan2s(float a,float b){return short(std::atan2(a,b)*32768/3.14159265);}
struct cCcD_Obj {
    unsigned power=1; bool active=true,hit=true;
    unsigned GetAtAtp()const{return power;}
    void OffAtSetBit(){active=false;} void ResetAtHit(){hit=false;}
};
struct fopAc_ac_c {unsigned id=7;struct {cXyz pos;csXyz angle;}current;};
constexpr int dItemNo_NONE_e=255,dItemNo_HEART_e=0,dItemNo_GREEN_RUPEE_e=1,dItemNo_BLUE_RUPEE_e=2;
struct daAlink_c : fopAc_ac_c {
    enum {PROC_WAIT,PROC_MOVE,PROC_ATN_MOVE,PROC_ATN_ACTOR_WAIT,PROC_ATN_ACTOR_MOVE,
          PROC_WAIT_TURN,PROC_MOVE_TURN,PROC_CUT_NORMAL,PROC_CUT_TURN,PROC_DAMAGE,PROC_JUMP,PROC_DODGE};
    int mProcID=PROC_WAIT,mEquipItem=0x103,field_0x2fde=0;
    bool wolf=false,model=true; void* mSwordModel=&model;
    struct {bool ground=true;bool ChkGroundHit(){return ground;}}mLinkAcch;
    struct Frame {float frame=0;float getFrame(){return frame;}}mUnderFrameCtrl[1];
    float field_0x3478=0,mNormalSpeed=0,mStickValue=0;short mMoveAngle=0;csXyz shape_angle;
    struct HIO {struct {struct {float mMaxSpeed=20;}m;}mMove;}hio;
    HIO* mpHIO=&hio;
    cCcD_Obj mAtCps[3],mAtSph;int tevStr=0;
    unsigned normals=0,spins=0,equips=0;
    bool checkWolf(){return wolf;} bool checkItemSwordEquip(){return mEquipItem==0x103;}
    void swordEquip(int){++equips;field_0x2fde=0x103;}
    int procCutNormalInit(int type){assert(type==1);++normals;mProcID=PROC_CUT_NORMAL;
        field_0x3478=5;mUnderFrameCtrl[0].frame=0;return 1;}
    int procCutTurnInit(int quick,int dir){assert(quick==1 && dir==1);++spins;
        mProcID=PROC_CUT_TURN;field_0x3478=220;return 1;}
} player;
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
Run s_run;
enum class SwordAction {None,Normal,Spin};
SwordAction s_swordAction=SwordAction::None;
bool s_wavePending=false,s_allowDrop=false,s_active=true,arena=true,playing=true;
unsigned s_meleeCooldown=0,s_player=7;
std::array<unsigned,3>s_cooldowns{};
bool in_arena(){return arena;} bool playable(){return arena && playing;}
struct Enemy {unsigned id=kNone;bool initialized=false,retiring=false,credited=false;};
std::array<Enemy,enemy_capacity>s_enemies;
fopAc_ac_c enemy;
fopAc_ac_c* actor_by_id(unsigned id){return id==enemy.id?&enemy:nullptr;}
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
constexpr u16 ID_ZI_J_KAITENGIRI_A=11,ID_ZI_J_KAITENGIRI_B=12;
Emitter* dComIfGp_particle_getEmitter(u32 id){auto it=emitters.find(id);return it==emitters.end()?nullptr:&it->second;}
u32 dComIfGp_particle_set(u32 id,u16 effect,cXyz*,int*,const csXyz*,void*,int,void*,int,void*,void*,void*){
    assert(effect==ID_ZI_J_KAITENGIRI_A || effect==ID_ZI_J_KAITENGIRI_B);
    if(!id){id=nextEmitter++;emitters.emplace(id,Emitter{});}return id;
}
void dComIfGp_particle_levelEmitterOnEventMove(u32){}
struct Attack {bool active=true;unsigned life=10;cXyz position,velocity;cCcD_Obj collider;std::array<u32,2>emitters{};};
std::array<Attack,projectile_capacity>s_attacks;
struct dCcU_AtInfo {cCcD_Obj* mpCollider;unsigned mAttackPower=200;};
HookAction before_drop(ModContext*,void*,void*,void*);
std::vector<int>drops;
fpc_ProcID fopAcM_createItem(const cXyz*,int item,int bit,int room,void*,void*,int mode){
    fpc_ProcID result=0;
    assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);
    assert(bit==-1 && room==51 && mode==0);drops.push_back(item);return 1;
}
unsigned maxLife=15,rupees=0,life=4;
void dComIfGs_setMaxLife(unsigned v){maxLife=v;}
unsigned dComIfGs_getRupeeMax(){return 300;}unsigned dComIfGs_getRupee(){return rupees;}
void dComIfGs_setRupee(unsigned v){rupees=v;}
void close_window(){}
'''
main = r'''
void reset(){
    player.mProcID=daAlink_c::PROC_WAIT;player.mEquipItem=0x103;
    player.mLinkAcch.ground=true;player.wolf=false;player.mSwordModel=&player.model;
    player.normals=player.spins=player.equips=0;
    s_run.start(1234);s_swordAction=SwordAction::None;s_wavePending=false;
    s_meleeCooldown=0;s_cooldowns={};waves=0;
    enemy.id=10;enemy.current.pos={0,0,150};s_enemies={};s_enemies[0]={10,true,false,false};
}
int main(){
    reset();update_sword(&player);
    assert(player.normals==1 && player.spins==0 && waves==0 && !s_wavePending);
    player.mUnderFrameCtrl[0].frame=6;update_sword(&player);assert(waves==0);
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
    reset();s_run.ranks[0]=1;update_sword(&player);player.mProcID=daAlink_c::PROC_DAMAGE;
    player.mUnderFrameCtrl[0].frame=9;update_sword(&player);assert(waves==0 && !s_wavePending);
    // Native spin has priority, receives Reach and follows the stick at normal speed.
    reset();s_run.ranks[0]=s_run.ranks[1]=1;s_run.ranks[5]=2;
    update_sword(&player);assert(player.spins==1 && player.normals==0 && waves==0);
    assert(std::abs(player.field_0x3478-286)<0.01);
    std::any args[]={&player};
    for(float stick:{0.0f,0.5f,1.0f})for(short angle:{short(-16000),short(16000)}){
        player.mStickValue=stick;player.mMoveAngle=angle;
        after_spin_move(nullptr,args,nullptr,nullptr);
        assert(player.current.angle.y==angle && player.mNormalSpeed==20*stick);
    }
    playing=false;player.mNormalSpeed=9;after_spin_move(nullptr,args,nullptr,nullptr);assert(player.mNormalSpeed==9);
    playing=true;player.id=8;after_spin_move(nullptr,args,nullptr,nullptr);assert(player.mNormalSpeed==9);player.id=7;
    BOOL large=1;assert(before_large_spin(nullptr,nullptr,&large,nullptr)==HOOK_SKIP_ORIGINAL && large==0);
    arena=false;assert(before_large_spin(nullptr,nullptr,&large,nullptr)==HOOK_CONTINUE);arena=true;
    // Only this Link's active native sword colliders receive linear Survivors damage.
    s_run.ranks[3]=2;dCcU_AtInfo info{&player.mAtSph};std::any powerArgs[]={&info};
    after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);
    s_swordAction=SwordAction::Normal;
    for(auto& capsule:player.mAtCps){info={&capsule};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==5);}
    cCcD_Obj unrelated;info={&unrelated};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200);
    arena=false;info={&player.mAtCps[0]};after_power(nullptr,powerArgs,nullptr,nullptr);assert(info.mAttackPower==200);arena=true;
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
    // Native random drops stay suppressed; only scoped reward creation bypasses the hook.
    fpc_ProcID result=0;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_SKIP_ORIGINAL && result==kNone);
    bool seen[3]{};
    for(int i=0;i<1000;++i){drop_reward(&enemy);assert(!s_allowDrop);}
    for(auto item:drops){assert(item>=0 && item<=2);seen[item]=true;}
    assert(seen[0] && seen[1] && seen[2]);
    arena=false;assert(before_drop(nullptr,nullptr,&result,nullptr)==HOOK_CONTINUE);arena=true;
    // Heart Container raises capacity only; fallback currency clamps to the wallet.
    reset();unsigned choice=0;s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Heart;
    select_upgrade(nullptr,&choice);assert(maxLife==20 && life==4);
    s_run.phase=Phase::Upgrade;s_run.offers[0]=Upgrade::Rupees;rupees=290;
    select_upgrade(nullptr,&choice);assert(rupees==300 && life==4);
    std::cout<<"Survivors combat: native sword scheduling, wave timing, moving spin, particles and rewards passed\n";
}
'''
functions = ['stop_attack', 'update_wave_effects', 'sword_action_available', 'enemy_in_range',
             'release_sword_wave', 'update_sword', 'after_spin_move', 'before_large_spin',
             'native_sword_collider', 'after_power', 'before_drop', 'drop_reward', 'select_upgrade']
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    binary = Path(tmp) / 'test'
    cpp.write_text(stubs + '\n'.join(extract(name) for name in functions) + '\n' + main)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root/'src'),
                    str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

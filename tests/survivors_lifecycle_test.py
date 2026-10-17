"""Exercise production death accounting, deferred cleanup and stage routing with engine stubs."""
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/survivors.cpp').read_text()
def extract(name):
    match = re.search(r'^(?:void|HookAction) ' + name + r'\(', source, re.M)
    assert match, name
    start = source.index('{', match.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[match.start():end + 1]
    raise AssertionError(name)
stubs = r'''
#include <cassert>
#include <any>
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_map>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned; using ProfileName=int; using s8=signed char; using s16=short;
constexpr ActorId kNone=~0u;
constexpr int fpcNm_E_OC_e=1, fpcNm_E_BA_e=2;
struct ModContext {};
enum HookAction { HOOK_CONTINUE, HOOK_SKIP_ORIGINAL };
namespace mods {
template<class T> T arg(void* args, int n) { return std::any_cast<T>(static_cast<std::any*>(args)[n]); }
template<class T> T& arg_ref(void* args, int n) { return *std::any_cast<T>(&static_cast<std::any*>(args)[n]); }
}
struct fopAc_ac_c { int health=0; struct { int pos=0; } current; };
struct e_ba_class : fopAc_ac_c { bool mIsDying=false; };
struct daE_OC_c : fopAc_ac_c { int action=0; int getActionMode() { return action; } };
struct Enemy {
    ActorId id=kNone; ProfileName profile=0;
    bool initialized=false, credited=false, retiring=false, elite=false;
    unsigned age=0;
};
std::array<Enemy,enemy_capacity> s_enemies;
Run s_run;
bool arena=true, transition=false, s_active=true, s_restart=false;
struct mDoRst {
    static inline bool resetting=false, menu=false;
    static bool isReset(){return resetting;} static bool isReturnToMenu(){return menu;}
};
bool in_arena(){return arena;} bool transitioning(){return transition;}
const char kStage[]="D_DLBR0";
unsigned restartParam=123;
void dComIfGs_setRestartRoomParam(unsigned value){restartParam=value;}
struct Collider { unsigned power=0; unsigned GetAtAtp(){return power;} };
struct Attack { Collider collider; };
std::array<Attack,16> s_attacks;
struct dCcU_AtInfo { Collider* mpCollider; unsigned mAttackPower; };
unsigned xp=0;
void add_orb(int,unsigned value){xp+=value;}
void finish(bool won){s_run.end(won);}
std::set<ActorId> pending;
std::unordered_map<ActorId,fopAc_ac_c*> actors;
bool fpcM_IsCreating(ActorId id){return pending.contains(id);}
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id); return it==actors.end()?nullptr:it->second;}
struct Service { unsigned attempts=0; void delete_actor(ModContext*,ActorId){++attempts;} } service;
Service* svc_actor=&service; ModContext* mod_ctx=nullptr;
'''
main = r'''
int main() {
    s_run.start(1);
    daE_OC_c actor; actor.health=0;
    Enemy enemy{7,fpcNm_E_OC_e,true,false,false,false,0};
    transition=true; credit_death(enemy,&actor); assert(xp==0);
    transition=false; enemy.retiring=true; credit_death(enemy,&actor); assert(xp==0);
    enemy.retiring=false; enemy.initialized=false; credit_death(enemy,&actor); assert(xp==0);
    enemy.initialized=true; actor.health=10; credit_death(enemy,&actor); assert(xp==0);
    actor.health=0; credit_death(enemy,&actor); assert(xp==3 && s_run.kills==1);
    credit_death(enemy,&actor); assert(xp==3 && s_run.kills==1);
    enemy={8,fpcNm_E_OC_e,true,false,false,true,0}; actor.health=1; actor.action=9;
    credit_death(enemy,&actor); assert(s_run.phase==Phase::Victory && xp==33);
    s_run.start(2);
    s_enemies[0]={9,fpcNm_E_OC_e,false,false,true,false,0}; pending.insert(9);
    cleanup_enemies(); assert(s_enemies[0].id==9 && service.attempts==0);
    pending.clear(); actors[9]=&actor;
    cleanup_enemies(); assert(service.attempts==1 && s_enemies[0].id==9);
    cleanup_enemies(); assert(service.attempts==2); // Retry refused/asynchronous deletes.
    actors.clear(); cleanup_enemies(); assert(s_enemies[0].id==kNone && s_run.kills==0);
    s_run.elite_spawned=true; s_enemies[0]={10,fpcNm_E_OC_e,true,false,false,true,0};
    cleanup_enemies(); assert(!s_run.elite_spawned && s_run.phase==Phase::Running);
    std::any args[]={static_cast<const char*>("F_SP102"),s16(100),s8(0),s8(10)};
    s_active=false;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(std::any_cast<const char*>(args[0]),"F_SP102")==0);
    s_active=true; arena=false;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    assert(std::strcmp(std::any_cast<const char*>(args[0]),kStage)==0);
    assert(std::any_cast<s8>(args[2])==51 && std::any_cast<s16>(args[1])==0 && restartParam==0);
    arena=true; s_run.phase=Phase::Running;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
    s_restart=true; assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    s_restart=false;mDoRst::resetting=true;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    mDoRst::resetting=false;mDoRst::menu=true;
    assert(before_stage(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
    s_active=true;
    for (unsigned power=1;power<=12;++power) {
        s_attacks[0].collider.power=power;
        dCcU_AtInfo info{&s_attacks[0].collider,200};std::any hit[]={&info};
        after_power(nullptr,hit,nullptr,nullptr);assert(info.mAttackPower==power);
    }
    Collider foreign{4};dCcU_AtInfo info{&foreign,200};std::any hit[]={&info};
    after_power(nullptr,hit,nullptr,nullptr);assert(info.mAttackPower==200);
    s_active=false;info.mpCollider=&s_attacks[0].collider;
    after_power(nullptr,hit,nullptr,nullptr);assert(info.mAttackPower==200);
    std::cout<<"Survivors lifecycle: true deaths, pending spawns, cleanup and routing passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp'; binary=Path(tmp)/'test'
    cpp.write_text(stubs+'\n'+ '\n'.join(extract(n) for n in ['cleanup_enemies','credit_death','before_stage','after_power'])+'\n'+main)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

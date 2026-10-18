"""Production elite schedule, authored placements, dormant AI and one-shot loot."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
runtime=(root/'src/survivors.cpp').read_text()
loot=(root/'src/survivors_loot.inc').read_text()
def extract(source,name):
    m=re.search(r'^(?:void|bool|HookAction) '+name+r'\(',source,re.M)
    assert m,name
    start=source.index('{',m.start());depth=0
    for end in range(start,len(source)):
        depth+=(source[end]=='{')-(source[end]=='}')
        if not depth:return source[m.start():end+1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include <iostream>
#include "survivors_rules.hpp"
#include "../include/survivors_maps.hpp"
using namespace dawnlight::survivors;
using s16=short;using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float n)const{return{x*n,y*n,z*n};}
 float abs2(cXyz b)const{auto d=*this-b;return d.x*d.x+d.y*d.y+d.z*d.z;}
 float absXZ(cXyz b)const{auto d=*this-b;return std::sqrt(d.x*d.x+d.z*d.z);}};
struct fopAc_ac_c{struct{cXyz pos;}current;struct{s16 y=0;}shape_angle;int health=120;};
struct daAlink_c:fopAc_ac_c{ActorId id=1;}player;
ActorId fopAcM_GetID(daAlink_c* actor){return actor->id;}
daAlink_c* daAlink_getAlinkActorClass(){return &player;}
struct daE_OC_c:fopAc_ac_c{ActorId id=0;float speedF=0;cXyz speed;unsigned animations=0;
 void setBck(int id,unsigned type,float,float){assert(id==0x18&&type==2);++animations;}
};
'''
fixture+=r'''
struct Enemy{ActorId id=kNone;int eliteSlot=-1;bool elite=false,dormant=false,retiring=false,credited=false,initialized=true,idleAnimation=false;unsigned age=0;cXyz lootPosition;};
std::array<Enemy,enemy_capacity>s_enemies;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id==id)return &e;return nullptr;}
ActorId fopAcM_GetID(daE_OC_c* a){return a->id;}
Run s_run;unsigned s_map=0,s_spawnCooldown=0,s_spawnFailures=0;
const Arena& arena(){return arenas[s_map];}
bool arenaPresent=true;bool in_arena(){return arenaPresent;}
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int i){return std::any_cast<T>(static_cast<std::any*>(args)[i]);}}
struct fopAcM_prm_class{struct{unsigned parameters=0;cXyz position;struct{s16 y=0;}angle;}base;int room_no=51;};
constexpr int fpcNm_DOOR20_e=3,fpcNm_DBDOOR_e=4,fpcNm_BOSS_DOOR_e=5,fpcNm_L1BOSS_DOOR_e=6;
struct stage_tgsc_data_class{char name[8]{};decltype(fopAcM_prm_class::base) base;};
struct stage_tgsc_class{int num=0;stage_tgsc_data_class* m_entries=nullptr;};
struct RoomData{stage_tgsc_class* doors=nullptr;
 auto* getDrTg(){return doors;}auto* getDoor(){return doors;}}roomData,stageData;
RoomData* dComIfGp_roomControl_getStatusRoomDt(int){return &roomData;}
RoomData* dComIfGp_getStage(){return &stageData;}
struct dStage_KeepDoorInfo{int mNum=0;stage_tgsc_data_class mDrTgData[64];}keepDoors,keepRoomDoors;
auto* dStage_GetKeepDoorInfo(){return &keepDoors;}auto* dStage_GetRoomKeepDoorInfo(){return &keepRoomDoors;}
struct NameInfo{int procname;};
NameInfo* dStage_searchName(const char* name){static NameInfo doorInfo{fpcNm_DOOR20_e};
 return std::strcmp(name,"door")==0?&doorInfo:nullptr;}
float cM_ssin(s16 a){return std::sin(float(a)*3.14159265f/32768.f);}float cM_scos(s16 a){return std::cos(float(a)*3.14159265f/32768.f);}
struct dBgS_ObjGndChk{cXyz pos;void SetPos(cXyz* p){pos=*p;}};
struct dBgS_LinChk{void Set(const cXyz*,const cXyz*,void*){}};
bool groundMissing=false,wallBlocked=false,barrierBlocked=false;int groundRoom=-2,groundCode=0;
struct Bg{float GroundCross(dBgS_ObjGndChk*){return groundMissing ? -1.e9f : 0.f;}
 int GetRoomId(dBgS_ObjGndChk&){return groundRoom==-2?arena().room:groundRoom;}
 int GetGroundCode(dBgS_ObjGndChk&){return groundCode;}
 bool LineCross(dBgS_LinChk*){return wallBlocked;}}bg;
Bg& dComIfG_Bgsp(){return bg;}
bool arena_path_clear(cXyz,cXyz,float){return !barrierBlocked;}
bool dry_arena_floor(cXyz& p,float,float){if(groundMissing||groundCode==4||groundCode==5||groundCode==10||
 (groundRoom!=-2&&groundRoom!=arena().room))return false;
 p.y=0;return true;}
bool walkable_spawn_path(cXyz,cXyz&,float){return !groundMissing&&!wallBlocked&&!barrierBlocked;}
bool failCreate=false;unsigned nextId=1,ordinarySpawns=0;
fopAc_ac_c* actor_by_id(ActorId){return &player;}
bool create_enemy_at(cXyz pos,s16,unsigned,bool elite,int slot=-1,bool dormant=false){
 if(failCreate)return false;
 for(auto& e:s_enemies)if(e.id==kNone){e={};e.id=nextId++;e.elite=elite;e.eliteSlot=slot;e.dormant=dormant;e.lootPosition=pos;return true;}
 return false;
}
bool spawn_enemy(daAlink_c*,bool elite,int slot=-1){if(!elite)++ordinarySpawns;return create_enemy_at({0,0,500},0,0,elite,slot);}
constexpr unsigned kPaidChestCount=2,kChestCount=2+elite_reward_capacity;
struct Chest{bool active=false,free=false;s16 yaw=0;cXyz position;};
std::array<Chest,kChestCount>s_chests;
bool s_active=true,paused=false;ActorId s_player=1;int life=12;unsigned finishes=0;
bool playable(){return arenaPresent&&!paused;}int dComIfGs_getLife(){return life;}
void finish(bool victory){++finishes;s_run.end(victory);}
void update_enemy_effects(){}void update_weapons(daAlink_c*){}void update_orbs(daAlink_c*){} void update_flutes(daAlink_c*){}
void clear_attacks(){}void update_chests(daAlink_c*){}void update_pots(daAlink_c*){}

'''
main=r'''
void fresh(unsigned map){s_map=map;roomData={};stageData={};keepDoors={};keepRoomDoors={};s_run.start(42);s_enemies={};s_chests={};clear_elite_sites();player.current.pos={};reset_elite_encounters();
 groundMissing=wallBlocked=barrierBlocked=failCreate=false;groundRoom=-2;groundCode=0;s_spawnCooldown=s_spawnFailures=ordinarySpawns=0;}
unsigned guards(){unsigned n=0;for(auto& e:s_enemies)n+=e.id!=kNone&&e.dormant;return n;}
unsigned waves(){unsigned n=0;for(auto& e:s_enemies)n+=e.id!=kNone&&e.elite&&!e.dormant;return n;}
void door(float x,float z){fopAcM_prm_class p;p.room_no=arena().room;p.base.parameters=unsigned(arena().room)<<13;p.base.position={x,0,z};capture_elite_site(fpcNm_DOOR20_e,p);}
void add_floor(float size){capture_elite_floor({-size,0,-size},{size,0,-size},{-size,0,size},1,false);capture_elite_floor({size,0,-size},{size,0,size},{-size,0,size},1,false);}
int main(){
 // Compact arenas have no map guards, even with captured doors or large floor.
 for(unsigned map:{0u,4u}){fresh(map);add_floor(10000);door(2000,0);assert(s_eliteDoorSites.size()==1);
  for(unsigned minute=0;minute<20;++minute){s_run.ticks=minute*60*30;update_elite_spawns(&player);
   assert(guards()==0&&waves()==minute/5);}
 }
 // Unaccepted creation retries; deletion is not a defeat and grants no chest.
 fresh(0);s_run.ticks=elite_interval;failCreate=true;update_elite_spawns(&player);assert(waves()==0);
 failCreate=false;update_elite_spawns(&player);assert(waves()==1);s_enemies[0]={};update_elite_spawns(&player);assert(waves()==1);
 for(const auto& c:s_chests)assert(!c.active);
 auto killed=s_enemies[0];drop_elite_chest(killed,&player);assert(s_eliteEncounters[0].defeated&&s_chests[2].active&&s_chests[2].free);
 assert(!s_chests[0].active&&!s_chests[1].active);s_chests[2]={};drop_elite_chest(killed,&player);assert(!s_chests[2].active);
 s_enemies[0]={};update_elite_spawns(&player);assert(waves()==0);
 // Missing corpse ground falls back to its last validated floor.
 killed.eliteSlot=1;killed.lootPosition={100,0,200};groundMissing=true;drop_elite_chest(killed,&player);
 assert(s_chests[2].position.x==100&&s_chests[2].position.z==200);groundMissing=false;
 // Exact milestone bypasses ordinary spawn cooldown, ordinary horde continues.
 fresh(0);s_run.ticks=elite_interval;s_spawnCooldown=30;update_spawns(&player);assert(waves()==1&&ordinarySpawns==1);
 // Four spread-out random guards, from the entire current room rather than Link's spawn ring.
 for(unsigned map:{1u,2u}){fresh(map);add_floor(10000);update_elite_spawns(&player);assert(guards()==4);
  for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)assert(s_enemies[i].lootPosition.absXZ(s_enemies[j].lootPosition)>=1800);
  for(auto& e:s_enemies)if(e.id!=kNone)e.age=6000;
  s_spawnCooldown=0;update_spawns(&player);for(auto& e:s_enemies)if(e.elite)assert(!e.retiring);
 }
 // One per authored building entrance; repeated layer records deduplicate.
 fresh(3);add_floor(10000);door(-4000,0);door(4000,0);door(0,6000);door(0,6000);update_elite_spawns(&player);assert(guards()==3);
 for(unsigned i=0;i<3;++i)assert(s_enemies[i].lootPosition.absXZ(s_eliteDoorSites[i].position)>=450);
 // Suspended/uncreated shared stage doors are recovered from native records.
 // Only doors adjoining room 0 count, regardless of the owning actor room.
 fresh(3);add_floor(10000);stage_tgsc_data_class rawDoors[4];
 for(unsigned i=0;i<4;++i){std::memcpy(rawDoors[i].name,"mapDoor",8);
  rawDoors[i].base.position={float(i)*2500-4000,600,0};
  rawDoors[i].base.parameters=((i+1)<<13);}
 rawDoors[3].base.parameters=(4u<<13)|(5u<<19);
 assert(!dStage_searchName(rawDoors[0].name));
 stage_tgsc_class doors{4,rawDoors};stageData.doors=&doors;
 update_elite_spawns(&player);assert(guards()==3&&s_eliteDoorSites.size()==3);
 update_elite_spawns(&player);assert(guards()==3); // cache refresh cannot append duplicates
 // Invalid room, holes, hazards, walls and exit curtains never accept placements.
 fresh(1);add_floor(10000);
 for(unsigned bad=0;bad<5;++bad){groundMissing=bad==0;groundRoom=bad==1?99:-2;groundCode=bad==2?4:0;wallBlocked=bad==3;barrierBlocked=bad==4;update_elite_spawns(&player);assert(guards()==0);}
 groundMissing=wallBlocked=barrierBlocked=false;groundRoom=-2;groundCode=0;update_elite_spawns(&player);assert(guards()==4);

 // Guards respawn three active minutes after the kill, not after spawning.
 fresh(3);add_floor(10000);door(2000,0);update_elite_spawns(&player);
 const auto guardSlot=s_enemies[0].eliteSlot;const auto home=s_enemies[0].lootPosition;
 s_run.ticks=713;drop_elite_chest(s_enemies[0],&player);s_enemies[0]={};
 const unsigned deadline=713+guard_respawn_interval;
 assert(s_eliteEncounters[guardSlot].respawnAt==deadline);
 s_run.ticks=deadline-1;update_elite_spawns(&player);assert(guards()==0);
 s_run.ticks=deadline;s_run.phase=Phase::Upgrade;update_elite_spawns(&player);assert(guards()==0);
 s_run.phase=Phase::Running;s_spawnCooldown=30;update_spawns(&player);assert(guards()==1);
 assert(s_enemies[0].lootPosition.absXZ(home)==0&&!s_eliteEncounters[guardSlot].defeated);
 // A second life earns its own chest, without overwriting the first unopened one.
 const auto previousChest=s_chests[2].position;player.current.pos={900,0,100};
 drop_elite_chest(s_enemies[0],&player);assert(s_chests[2].active&&s_chests[3].active);
 assert(s_chests[2].position.absXZ(previousChest)==0&&s_chests[3].position.x==900);
 drop_elite_chest(s_enemies[0],&player);assert(!s_chests[4].active);
 s_run.ticks+=guard_respawn_interval;update_elite_spawns(&player);
 assert(s_eliteEncounters[guardSlot].defeated); // Do not duplicate a still-tracked corpse.
 s_enemies[0]={};failCreate=true;update_elite_spawns(&player);assert(guards()==0);
 failCreate=false;update_elite_spawns(&player);assert(guards()==1&&s_enemies[0].dormant);
 // Outdoor respawns are separated from both the old position and other guards.
 for(unsigned map:{1u,2u}){fresh(map);add_floor(10000);update_elite_spawns(&player);
  const auto previous=s_enemies[0].lootPosition;drop_elite_chest(s_enemies[0],&player);s_enemies[0]={};
  s_run.ticks=guard_respawn_interval;groundMissing=true;update_elite_spawns(&player);assert(guards()==3);
  groundMissing=false;failCreate=true;update_elite_spawns(&player);assert(guards()==3);
  assert(s_eliteEncounters[timed_elite_count].position.absXZ(previous)==0);
  failCreate=false;update_elite_spawns(&player);assert(guards()==4&&s_enemies[0].dormant);
  assert(s_enemies[0].lootPosition.absXZ(previous)>=1800);
  for(unsigned j=1;j<4;++j)assert(s_enemies[0].lootPosition.absXZ(s_enemies[j].lootPosition)>=1800);
 }
 // All eight guard slots can leave every possible drop uncollected for 20 minutes.
 fresh(3);add_floor(30000);
 for(unsigned i=0;i<guard_capacity;++i)door(float(i)*3000-10000,0);
 unsigned drops=0;
 for(unsigned tick=0;tick<duration;++tick){
  s_run.ticks=tick;update_elite_spawns(&player);
  for(auto& e:s_enemies)if(e.id!=kNone){drop_elite_chest(e,&player);++drops;e={};}
 }
 assert(drops==elite_reward_capacity);
 for(unsigned i=kPaidChestCount;i<kChestCount;++i)assert(s_chests[i].active&&s_chests[i].free);
 s_run.ticks=duration;update_elite_spawns(&player);assert(guards()==0&&waves()==0);
 // Dormant AI suppresses only idle action. Collision/damage Execute stays native.
 fresh(3);add_floor(10000);door(2000,0);update_elite_spawns(&player);
 daE_OC_c actor;actor.id=s_enemies[0].id;actor.speedF=10;actor.speed={2,3,4};std::any args[]={&actor};
 assert(before_bokoblin_wait(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 assert(actor.speedF==0&&actor.speed.x==0&&actor.speed.y==3&&actor.animations==1);
 before_bokoblin_wait(nullptr,args,nullptr,nullptr);assert(actor.animations==1);
 s_enemies[0].dormant=false;assert(before_bokoblin_wait(nullptr,args,nullptr,nullptr)==HOOK_CONTINUE);
 // Production timer completion: pause and UI phases never consume run time.
 fresh(0);s_run.ticks=duration-1;std::any playerArgs[]={&player};paused=true;
 after_player(nullptr,playerArgs,nullptr,nullptr);assert(s_run.ticks==duration-1&&finishes==0);
 paused=false;s_run.phase=Phase::Upgrade;after_player(nullptr,playerArgs,nullptr,nullptr);assert(finishes==0);
 s_run.phase=Phase::Running;after_player(nullptr,playerArgs,nullptr,nullptr);
 assert(s_run.ticks==duration&&s_run.phase==Phase::Victory&&finishes==1&&waves()==0);
 after_player(nullptr,playerArgs,nullptr,nullptr);assert(finishes==1);
 fresh(0);life=0;after_player(nullptr,playerArgs,nullptr,nullptr);assert(s_run.phase==Phase::Defeat&&s_run.ticks==0);
 reset_elite_encounters();for(const auto& e:s_eliteEncounters)assert(!e.placed&&!e.defeated);
 clear_elite_sites();assert(s_eliteFloors.empty()&&s_eliteDoorSites.empty());
 std::cout<<"Survivors elites: schedule, all map placements, respawns, dormancy and non-overwriting free drops passed\n";
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'test.cpp';exe=Path(temp)/'test'
    cpp.write_text(fixture+(root/'src/survivors_elite_positions.inc').read_text()+
        (root/'src/survivors_elites.inc').read_text()+extract(loot,'drop_elite_chest')+
        extract(runtime,'update_spawns')+extract(runtime,'after_player')+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)

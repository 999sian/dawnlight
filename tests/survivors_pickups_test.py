"""Native pickup magnet, scheduled-elite flute and XP snapshot/lifecycle regression."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
start=core.index('void update_orbs(')
orbs=core[start:core.index('\nbool create_enemy_at',start)]
fixture=r'''
#include <any>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using s16=short;using ActorId=unsigned;constexpr ActorId kNone=~0u;
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}cXyz operator-(cXyz b)const{return{x-b.x,y-b.y,z-b.z};}
 cXyz operator*(float f)const{return{x*f,y*f,z*f};}void operator+=(cXyz b){x+=b.x;y+=b.y;z+=b.z;}
 float abs()const{return std::sqrt(x*x+y*y+z*z);}};
struct fopAc_ac_c{ActorId id=0;struct{cXyz pos;}current,old;cXyz speed;float speedF=0;};
struct daAlink_c:fopAc_ac_c{}player;ActorId s_player=1;
auto* daAlink_getAlinkActorClass(){return &player;}ActorId fopAcM_GetID(fopAc_ac_c* p){return p->id;}
struct daItem_c:fopAc_ac_c{enum{TYPE_FIXED_PLACE_e=1,TYPE_LAUNCH_e=0,STATUS_NORMAL_e=0,FLAG_NO_TIMER_e=8};
 int status=0,mWaitTimer=0;bool hidden=false;int flags=0;int getStatus(){return status;}
 void setFlag(int f){flags|=f;}void hide(){hidden=true;}};
struct daObjLife_c:daItem_c{enum{STATUS_WAIT_e=0};bool chkStatus(int s){return status==s;}};
struct Enemy{bool elite=false;int eliteSlot=-1;cXyz lootPosition;};
struct Orb{cXyz position;unsigned value=0;bool summoned=false;};std::array<Orb,orb_capacity>s_orbs;
Run s_run;bool s_allowDrop=false,playing=true,failCreate=false,dry=true,path=true;
bool playable(){return playing;}
struct Arena{int room=6;}data;const Arena& arena(){return data;}
enum{dItemNo_HORSE_FLUTE_e=132,dItemNo_KAKERA_HEART_e=33,dItemNo_HEART_e=0};
struct ModContext{};ModContext* mod_ctx=nullptr;enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int i){return std::any_cast<T>(static_cast<std::any*>(args)[i]);}}
std::map<ActorId,daObjLife_c>actors;std::set<ActorId>pending;unsigned nextId=2,creates=0;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:&it->second;}
bool fpcM_IsCreating(ActorId id){return pending.count(id);}
ActorId fopAcM_createItem(const cXyz* pos,int item,int bit,int room,void*,void*,int type){
 assert(s_allowDrop&&bit==-1&&room==6);assert(type==(item==132?1:0));++creates;
 if(failCreate)return kNone;
 const auto id=nextId++;actors[id].id=id;actors[id].current.pos=*pos;return id;
}
struct Service{void delete_actor(ModContext*,ActorId id){actors.erase(id);}}service;auto* svc_actor=&service;
bool dry_arena_floor(cXyz& pos,float,float){pos.y=0;return dry;}
bool walkable_spawn_path(cXyz,cXyz& pos,float){pos.y=0;return path;}
float cM_ssin(s16 a){return std::sin(float(a)*3.14159265f/32768);}float cM_scos(s16 a){return std::cos(float(a)*3.14159265f/32768);}
'''
main=r'''
void fresh(){s_run.start(123);s_orbs={};s_pickups.clear();s_flutes={};actors.clear();pending.clear();
 player={};player.id=1;playing=dry=path=true;failCreate=false;s_allowDrop=false;creates=0;}
int main(){
 for(int item:{0,1,2,3,4,5,6,7,33}){
  fresh();auto id=create_run_pickup({300,20,0},item);auto* p=tracked_pickup(id);auto* actor=actor_by_id(id);
  assert(p&&!s_allowDrop);move_pickup(*p,actor);assert(actor->current.pos.x==300);
  s_run.ranks[unsigned(Upgrade::Magnet)]=5;move_pickup(*p,actor);assert(actor->current.pos.x==275);
  playing=false;move_pickup(*p,actor);assert(actor->current.pos.x==275);playing=true;
  actors[id].status=2;move_pickup(*p,actor);assert(actor->current.pos.x==275); // Native Get Item must finish undisturbed.
  assert(!pickup_waiting(*p,actor));actors[id].status=0;assert(pickup_waiting(*p,actor));
 }
 fresh();s_allowDrop=true;create_run_pickup({},0);assert(s_allowDrop);s_allowDrop=false;
 fresh();Enemy guard{true,int(timed_elite_count),{}};queue_flute(guard,&player);update_flutes(&player);assert(creates==0);
 Enemy timed{true,0,{1000,0,0}};player.current.pos={1000,0,0};queue_flute(timed,&player);queue_flute(timed,&player);
 path=false;update_flutes(&player);assert(creates==0);path=true;failCreate=true;update_flutes(&player);assert(s_flutes[0].pending);
 failCreate=false;update_flutes(&player);const auto id=s_flutes[0].id;assert(id!=kNone&&!s_flutes[0].pending);
 update_flutes(&player);assert(creates==2);auto* p=tracked_pickup(id);auto* actor=actor_by_id(id);
 assert(std::abs((actor->current.pos-player.current.pos).abs()-160.7f)<2);
 std::any args[]={static_cast<daItem_c*>(&actors[id])};assert(before_flute_get(nullptr,args,nullptr,nullptr)==HOOK_SKIP_ORIGINAL);
 daItem_c foreign;std::any other[]={&foreign};assert(before_flute_get(nullptr,other,nullptr,nullptr)==HOOK_CONTINUE);
 s_orbs[0]={{50000,30,0},8,false};s_orbs[1]={{-50000,30,0},9,false};
 player.current.pos=actor->current.pos;playing=false;move_pickup(*p,actor);assert(!p->retiring&&!s_orbs[0].summoned);
 playing=true;move_pickup(*p,actor);assert(p->retiring&&s_flutes[0].collected&&s_orbs[0].summoned&&s_orbs[1].summoned);
 s_orbs[2]={{40000,30,0},10,false}; // Future XP is not part of the one-time sweep.
 for(int i=0;i<200;++i)update_orbs(&player);
 assert(s_run.xp==17&&s_orbs[2].value==10&&!s_orbs[2].summoned);
 cleanup_pickups();cleanup_pickups();assert(!tracked_pickup(id));update_flutes(&player);assert(creates==2);
 // Pending native actor creation is retained until it can be deleted, never rewarded.
 fresh();auto pendingId=create_run_pickup({},132);actors.erase(pendingId);pending.insert(pendingId);
 retire_pickups();cleanup_pickups();assert(tracked_pickup(pendingId));
 pending.erase(pendingId);actors[pendingId].id=pendingId;cleanup_pickups();cleanup_pickups();assert(s_pickups.empty());
 // Lost/failed flute actors retry, but teardown clears the request.
 fresh();queue_flute(timed,&player);update_flutes(&player);actors.clear();cleanup_pickups();update_flutes(&player);assert(creates==2);
 retire_pickups();cleanup_pickups();cleanup_pickups();update_flutes(&player);assert(creates==2&&s_run.xp==0);
 std::cout<<"Survivors pickups: native magnet, flute scoping, XP snapshot, pause and deferred cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_pickups.inc').read_text()+orbs+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)

"""Production enemy variants, elite health/scale and native enemy-arrow ownership."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
core=(root/'src/survivors.cpp').read_text()
def function(name):
 m=re.search(r'^bool '+name+r'\([^;{}]*\)\s*\{',core,re.M);start=core.index('{',m.start());depth=0
 for end in range(start,len(core)):
  depth+=(core[end]=='{')-(core[end]=='}')
  if not depth:return core[m.start():end+1]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned;using ProfileName=int;using u32=unsigned;using s16=short;
constexpr ActorId kNone=~0u;constexpr int fpcNm_E_OC_e=1,fpcNm_E_BA_e=2,fpcNm_E_MS_e=3,fpcNm_E_RD_e=4,fpcNm_E_ARROW_e=5;
struct ModContext{};enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
ModContext* mod_ctx=nullptr;
namespace mods{template<class T>T arg(void* args,int n){return std::any_cast<T>(static_cast<std::any*>(args)[n]);}}
struct cXyz{float x=0,y=0,z=0;cXyz()=default;cXyz(float a,float b,float c):x(a),y(b),z(c){}
 cXyz operator+(cXyz b)const{return{x+b.x,y+b.y,z+b.z};}};
struct csXyz{s16 x,y,z;};
struct fopAc_ac_c{int health=1,field_0x560=1;struct{cXyz pos;}current;cXyz scale{2,2,2};};
struct daAlink_c:fopAc_ac_c{};
struct Model{cXyz scale{1,1,1};void setBaseScale(cXyz s){scale=s;}};
struct Morf{Model model;Model* getModel(){return &model;}};
struct Sphere{float radius=0;void SetR(float f){radius=f;}};
struct daE_OC_c:fopAc_ac_c{Model model;Model* getOcModel(){return &model;}Sphere mSphs_cc[2],mSphs_at[2];};
struct e_ba_class{fopAc_ac_c enemy;Morf morph;Morf* mpMorf=&morph;Sphere mSph;};
struct e_ms_class:fopAc_ac_c{Morf morph;Morf* mpModelMorf=&morph;Sphere mSph1,mSph2;};
struct e_rd_class{fopAc_ac_c enemy;Sphere cc_sph[3];};
struct Enemy{ActorId id=kNone;ProfileName profile=0;bool initialized=false,credited=false,retiring=false,elite=false;
 unsigned age=0;int eliteSlot=-1;bool dormant=false;cXyz lootPosition{};};
std::array<Enemy,enemy_capacity>s_enemies;
Enemy* tracked(ActorId id){for(auto& e:s_enemies)if(e.id!=kNone&&e.id==id)return &e;return nullptr;}
Run s_run;bool arenaPresent=true;bool in_arena(){return arenaPresent;}
struct ActorSpawnParams{u32 parameters;int argument,room_num;cXyz position;csXyz angle;cXyz scale;void* create_function;};
struct Arena{int room=0;}map;const Arena& arena(){return map;}
constexpr int MOD_OK=0;bool createFailed=false;ActorId nextId=1;
ProfileName lastProfile;ActorSpawnParams lastParams;
int create_standalone_actor(ProfileName p,const ActorSpawnParams& params,ActorId& id){
 lastProfile=p;lastParams=params;if(createFailed)return 1;id=nextId++;return MOD_OK;}
float cM_ssin(s16 a){return std::sin(a*3.14159265f/32768);}float cM_scos(s16 a){return std::cos(a*3.14159265f/32768);}
bool dryPath=true;bool walkable_spawn_path(cXyz,cXyz& p,float){if(dryPath)p.y=30;return dryPath;}
std::map<ActorId,fopAc_ac_c*>actors;std::set<ActorId>creating,deleted;
fopAc_ac_c* actor_by_id(ActorId id){auto it=actors.find(id);return it==actors.end()?nullptr:it->second;}
bool fpcM_IsCreating(ActorId id){return creating.contains(id);}
struct Service{void delete_actor(ModContext*,ActorId id){deleted.insert(id);}}service;auto* svc_actor=&service;
'''
main=r'''
int main(){
 s_run.start(42);s_run.ticks=900;daAlink_c player;std::set<unsigned>variants;
 for(unsigned i=0;i<300;++i){s_enemies={};assert(spawn_enemy(&player,false));
  variants.insert((unsigned(lastProfile)<<24)|(lastParams.parameters&0xffffff));
  assert(lastParams.scale.x==1&&lastParams.position.y==(lastProfile==fpcNm_E_BA_e?210:30));}
 assert(variants.size()==5);
 assert(variants.contains((fpcNm_E_RD_e<<24)|0x100)&&variants.contains((fpcNm_E_RD_e<<24)|0x200));
 const int profiles[]={fpcNm_E_MS_e,fpcNm_E_BA_e,fpcNm_E_RD_e,fpcNm_E_OC_e};
 for(unsigned i=0;i<4;++i){s_enemies={};assert(spawn_enemy(&player,true,i));
  assert(lastProfile==profiles[i]&&lastParams.scale.x==2&&s_enemies[0].eliteSlot==int(i));
  if(i==2)assert(lastParams.parameters==0xff000200);}
 s_enemies={};dryPath=false;assert(!spawn_enemy(&player,true,0)&&s_enemies[0].id==kNone);dryPath=true;
 createFailed=true;assert(!spawn_enemy(&player,false)&&s_enemies[0].id==kNone);createFailed=false;
 assert(!create_enemy_at({},0,5,false));
 // Elite HP applies to every species, once, including difficulty; sizes do not accumulate.
 s_run.options.enemy_hp=10;
 daE_OC_c bok;e_ba_class bat;e_ms_class rat;e_rd_class bulblin;
 fopAc_ac_c* bases[]={&rat,&bat.enemy,&bulblin.enemy,&bok};
 for(unsigned i=0;i<4;++i){Enemy e;e.elite=true;e.profile=profiles[i];initialize_enemy(&e,bases[i]);
  assert(e.initialized&&bases[i]->health==1200&&bases[i]->field_0x560==1200);
  bases[i]->health=800;initialize_enemy(&e,bases[i]);assert(bases[i]->health==800);
  for(unsigned n=0;n<3;++n)scale_elite_colliders(&e,bases[i]);}
 assert(bok.model.scale.x==2&&rat.morph.model.scale.x==2&&bat.morph.model.scale.x==2);
 assert(rat.mSph1.radius==70&&rat.mSph2.radius==40&&bat.mSph.radius==60&&bulblin.cc_sph[2].radius==70);
 Enemy regular;regular.profile=fpcNm_E_RD_e;initialize_enemy(&regular,&bulblin.enemy);assert(bulblin.enemy.health==90);
 // Native arrows are not enemies/loot sources. They remain owned after shooter deletion.
 s_enemies={};s_enemies[0].id=42;s_enemies[0].profile=fpcNm_E_RD_e;
 std::any args[]={s16(fpcNm_E_ARROW_e),ActorId(42)};ActorId arrowId=100;
 assert(before_enemy_arrow(nullptr,args,&arrowId,nullptr)==HOOK_CONTINUE);
 after_enemy_arrow(nullptr,args,&arrowId,nullptr);assert(active_enemy_arrow(100));
 after_enemy_arrow(nullptr,args,&arrowId,nullptr); // no duplicate entries
 s_enemies[0]={};assert(active_enemy_arrow(100));creating.insert(100);retire_enemy_arrows();cleanup_enemy_arrows();
 assert(tracked_enemy_arrow(100)&&!active_enemy_arrow(100)&&deleted.empty());
 creating.clear();actors[100]=&player;cleanup_enemy_arrows();assert(deleted.contains(100));
 actors.clear();cleanup_enemy_arrows();assert(!tracked_enemy_arrow(100));
 // Foreign children and failed creates are ignored; capacity/pause reject new owned arrows.
 arrowId=101;after_enemy_arrow(nullptr,args,&arrowId,nullptr);assert(!tracked_enemy_arrow(101));
 s_enemies[0].id=42;s_enemies[0].profile=fpcNm_E_RD_e;
 arrowId=kNone;after_enemy_arrow(nullptr,args,&arrowId,nullptr);assert(!tracked_enemy_arrow(kNone));
 s_run.phase=Phase::Upgrade;assert(before_enemy_arrow(nullptr,args,&arrowId,nullptr)==HOOK_SKIP_ORIGINAL&&arrowId==kNone);
 s_run.phase=Phase::Running;for(auto& a:s_enemyArrows)a.id=nextId++;
 assert(before_enemy_arrow(nullptr,args,&arrowId,nullptr)==HOOK_SKIP_ORIGINAL);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+(root/'src/survivors_enemy_types.inc').read_text()+(root/'src/survivors_enemy_arrows.inc').read_text()+
  function('create_enemy_at')+function('spawn_enemy')+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors enemies: five normal variants, ordered elites, dry placement, HP/scale, projectile ownership/caps and deferred cleanup passed')

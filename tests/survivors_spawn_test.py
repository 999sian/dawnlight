"""Arena entry/grounding regression using production hooks and SDK entry modes."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
root = Path(__file__).resolve().parents[1]
sdk = Path(sys.argv[1]) if len(sys.argv) > 1 else root/'dusklight'
source = (root/'src/survivors_maps.inc').read_text()
runtime = (root/'src/survivors.cpp').read_text()
native = (sdk/'src/d/actor/d_a_alink.cpp').read_text()

def extract(name):
    match = re.search(r'^(?:void|bool|HookAction) '+name+r'\(', source, re.M)
    assert match, name
    start = source.index('{', match.start())
    depth = 0
    for end in range(start, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if not depth:
            return source[match.start():end+1]
    raise AssertionError(name)

# This is the native condition that enters the 35-frame entrance demo. Mode 0
# was mistakenly used as "no demo". Read the SDK rather than duplicate the modes.
entry_condition = re.search(r'else if \((start_mode == 1 \|\| start_mode == 3[^\n]+)\) \{', native).group(1)
start = runtime.index('    if (s_player != fopAcM_GetID(link))')
end = runtime.index('    open_powerup_window();', start)
loading = 'void load_frame(daAlink_c* link) {\n'+runtime[start:end]+'}\n'
names = sorted(set(re.findall(r'fpcNm_\w+', source)))
fixture = r'''
#include <any>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include "survivors_rules.hpp"
#include "../include/survivors_maps.hpp"
#include "../include/survivors_barriers.hpp"
using namespace dawnlight::survivors;
using ActorId=unsigned; using s16=short;
struct cXyz {float x=0,y=0,z=0; cXyz()=default; cXyz(float a,float b,float c):x(a),y(b),z(c){} void set(float a,float b,float c){x=a;y=b;z=c;}};
struct csXyz {short x=0,y=0,z=0;};
struct fopAc_ac_c {unsigned id=7; int room=51; struct{cXyz pos;}current,old;cXyz speed;};
struct daAlink_c:fopAc_ac_c {struct{bool hit=false;float floor=-1e9f;bool ChkGroundHit(){return hit;}float GetGroundH(){return floor;}}mLinkAcch;};
struct fopAcM_prm_class {struct{unsigned parameters=0;cXyz position;csXyz angle;}base;int room_no=51;struct{int x=10,z=10;}scale;};
struct stage_actor_data_class {char name[8]{};};
struct ModContext{}; enum HookAction{HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods{template<class T>T arg(void* args,int i){return std::any_cast<T>(static_cast<std::any*>(args)[i]);}}
unsigned s_map=0;const Arena& arena(){return arenas[s_map];}Run s_run;
ActorId s_player=7;unsigned s_warmup=0,recordsLoaded=0;
bool active=true,transition=false,event=false,creating=false;
bool in_arena(){return active;}bool transitioning(){return transition;}bool playable(){return active&&!event&&!transition;}
bool fpcM_IsCreating(ActorId){return creating;}
unsigned fopAcM_GetID(fopAc_ac_c* a){return a->id;}int fopAcM_GetRoomNo(fopAc_ac_c* a){return a->room;}
void fopAcM_SetRoomNo(fopAc_ac_c* a,int n){a->room=n;}
void load_records(){++recordsLoaded;}
void clear_elite_sites(){}
void capture_elite_actor(const char*,const fopAcM_prm_class&){}
void reset_midna_dialog(){}
void update_midna_dialog(){}
constexpr int fopAc_ENEMY_e=2,fopAc_NPC_e=4;
struct actor_process_profile_definition{int group=0;}profile;
const auto* fpcPf_Get(s16){return &profile;}
struct{int procname=0;}info;
const auto* dStage_searchName(const char*){return &info;}
unsigned freed=0;void JKRFree(void*){++freed;}
float cM_ssin(short a){return std::sin(a*3.14159265f/32768);}float cM_scos(short a){return std::cos(a*3.14159265f/32768);}
'''
main = r'''
int main(){
 stage_actor_data_class record;fopAcM_prm_class params;
 std::any spawnArgs[]={&record,&params};info.procname=fpcNm_ALINK_e;
 for(unsigned map=0;map<arenas.size();++map){
  s_map=map;params.base.parameters=0x2ab14a00u|unsigned(arena().room);
  params.base.position={12,34,56};params.base.angle.y=1234;
  const auto original=params.base.parameters;
  assert(before_arena_stage_actor(nullptr,spawnArgs,nullptr,nullptr)==HOOK_CONTINUE);
  assert(params.base.position.y==34&&params.base.angle.y==1234&&freed==0);
  if(map==0) assert(params.base.parameters==original);
  else {
   assert((params.base.parameters>>24)==255);
   assert(((params.base.parameters>>12)&31)==16);
   assert(!native_entrance_demo((params.base.parameters>>12)&31));
   assert((params.base.parameters&0x00fe0fff)==(original&0x00fe0fff));
  }
 }
 assert(native_entrance_demo(0)); // The old override entered this path.
 s_map=0;clear_arena_geometry();daAlink_c link;std::any moveArgs[]={&link};
 // Native entrance crosses an exit curtain from outside into the room. Applying
 // the running barrier sweep here traps Link on the wrong side of the floor.
 s_barriers.push_back({{0,0,-100},{0,0,100}});
 s_run.phase=Phase::Loading;s_arenaSafe=true;
 link.old.pos={-100,20,0};link.current.pos={100,0,0};
 after_arena_position(nullptr,moveArgs,nullptr,nullptr);
 assert(link.current.pos.x==100&&link.current.pos.y==0);
 // Once running, the same crossing is correctly blocked without changing Y.
 s_run.phase=Phase::Running;after_arena_position(nullptr,moveArgs,nullptr,nullptr);
 assert(link.current.pos.x<-39&&link.current.pos.y==0);
 active=false;link.current.pos={100,0,0};after_arena_position(nullptr,moveArgs,nullptr,nullptr);
 assert(link.current.pos.x==100);active=true;
 // Loading cannot time out into Ready while airborne, creating, in an event,
 // above stale collision, on an invalid floor, or in a neighbouring room.
 for(int reason=0;reason<6;++reason){
  clear_arena_geometry();s_run.phase=Phase::Loading;s_warmup=0;
  link.current.pos={0,reason==3?100.f:0.f,0};link.mLinkAcch.hit=reason!=0;
  link.mLinkAcch.floor=reason==4?-1e9f:0;link.room=reason==5?0:51;
  creating=reason==1;event=reason==2;
  for(int frame=0;frame<60;++frame)load_frame(&link);
  assert(s_run.phase==Phase::Loading&&s_warmup==0);
 }
 creating=event=false;link.room=51;link.current.pos={100,0,0};
 link.mLinkAcch.hit=true;link.mLinkAcch.floor=0;s_arenaSafe=false;
 for(int frame=0;frame<14;++frame){load_frame(&link);assert(s_run.phase==Phase::Loading);}
 // Capture native floor contact even if the derived barrier mesh is not ready.
 assert(s_arenaSafe&&!s_arenaMeshReady);
 load_frame(&link);assert(s_run.phase==Phase::Ready&&recordsLoaded==1);
 // Falling during loading must restore the recorded floor, not freeze below it.
 s_run.phase=Phase::Loading;link.current.pos.y=-600;link.mLinkAcch.hit=false;
 link.mLinkAcch.floor=-1e9f;link.speed.y=-50;load_frame(&link);
 assert(link.current.pos.y==0&&link.old.pos.y==0&&link.speed.y==0);
 assert(s_run.phase==Phase::Loading);
 // Replacement players cannot inherit the previous player's rescue position.
 link.id=8;link.current.pos.y=-100;creating=true;load_frame(&link);
 assert(!s_arenaSafe&&s_run.phase==Phase::Loading&&s_warmup==0);
 std::cout<<"Survivors spawn: native entry, barrier timing, grounding, recovery and actor replacement passed\n";
}
'''
functions = '\n'.join(extract(n) for n in ['add_exit_box','arena_story_actor','before_arena_stage_actor',
    'confine_arena_actor','after_arena_position','arena_grounded','update_arena_safety'])
globals = source[source.index('std::vector<barriers::Wall>'):source.index('void add_exit_box')]
with tempfile.TemporaryDirectory() as tmp:
    cpp,binary=Path(tmp)/'test.cpp',Path(tmp)/'test'
    cpp.write_text(fixture+'enum{'+','.join(names)+'};\n'+globals+functions+loading+
        'bool native_entrance_demo(int start_mode){return '+entry_condition+';}\n'+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)

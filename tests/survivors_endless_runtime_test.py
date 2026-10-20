"""Victory-to-Endless handoff preserves the live build and stages the checkpoint once."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/survivors.cpp').read_text()
start=source.index('void continue_endless(ModContext*,void*) {')
callback=source[start:source.index('void replay(',start)]
fixture=r'''
#include <cassert>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
struct ModContext{};ModContext* mod_ctx=nullptr;
constexpr unsigned kNone=~0u;constexpr int MOD_OK=0;constexpr char kBlob[]="test";
Run s_run;Records s_records;
bool s_resultsStored=true,s_restart=false,s_saveRequested=false,s_progressSaving=false,s_storeFailed=false;
unsigned s_refreshChoices=0,s_window=1;
unsigned s_savePrompt=kNone,s_spawnCooldown=99,s_spawnFailures=12;
bool transition=false,failSave=false,newMod=false;unsigned writes=0,closed=0,resets=0;
bool transitioning(){return transition;}
struct Encounter{unsigned respawnAt=0;};std::array<Encounter,elite_capacity>s_eliteEncounters;
void reset_elite_encounters(){s_eliteEncounters={};}
void close_window(){++closed;}void reset_ending(){++resets;}
void begin_survivors_integrity(bool,bool& modded){modded=newMod;}
struct Save{int set_blob(ModContext*,const char*,const void*,size_t){++writes;return failSave?1:0;}}save;
auto* svc_save=&save;
'''
main=r'''
int main(){
 s_run.start(123);s_run.ticks=duration;s_run.master_sword=19;s_run.dual_wield=25;
 s_run.ranks[unsigned(Upgrade::Lightning)]=20;s_run.xp=18;s_run.options.allow_cheats=true;s_run.modded=true;
 s_run.end(true);s_records.finish(s_run,2);const auto records=s_records.encode();
 const auto random=s_run.random;
 for(unsigned gate=0;gate<6;++gate){
  s_restart=gate==0;s_saveRequested=gate==1;s_progressSaving=gate==2;
  transition=gate==3;s_savePrompt=gate==4?9:kNone;s_resultsStored=gate!=5;
  continue_endless(nullptr,nullptr);assert(!writes&&!closed&&!s_run.endless);
 }
 s_restart=s_saveRequested=s_progressSaving=transition=false;s_savePrompt=kNone;s_resultsStored=true;
 failSave=true;continue_endless(nullptr,nullptr);
 assert(s_storeFailed&&closed==1&&s_refreshChoices==1&&s_run.phase==Phase::Victory&&s_records.encode()==records);
 failSave=false;continue_endless(nullptr,nullptr);
 assert(!s_storeFailed&&closed==2&&resets==1&&s_run.endless&&s_run.phase==Phase::Running);
 assert(!s_resultsStored&&!s_spawnCooldown&&!s_spawnFailures);
 assert(s_run.master_sword==19&&s_run.dual_wield==25&&s_run.rank(Upgrade::Lightning)==20&&s_run.xp==18);
 assert(s_run.random==random&&s_run.modded&&s_run.options.allow_cheats&&s_records.encode()==records);
 for(unsigned i=0;i<4;++i)assert(s_eliteEncounters[i].respawnAt==duration+(i+1)*elite_interval);
 continue_endless(nullptr,nullptr);assert(writes==2&&closed==2);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
    cpp.write_text(fixture+callback+main)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Endless handoff: checkpoint retry, save guards, live build, integrity and boss schedule passed')

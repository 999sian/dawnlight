"""Exercise the production save-to-progression mapping and effective settings."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/progression.cpp').read_text()
start = source.index('ProgressionState progression_state()')
function = source[start:source.index('\n}', start) + 2]
fixture = r'''
#include "general_modes.hpp"
#include <cassert>
using namespace dawnlight;
struct dSv_event_flag_c { enum {F_0026, M_016, M_019}; };
bool bossRush=false, flags[3]{};
constexpr int dStage_SaveTbl_LV6=6;
bool middleBoss[10]{}, liveMiddleBoss=false;
int currentStage=0;
bool hasStage=true;
int* dComIfGp_getStageStagInfo(){return hasStage ? &currentStage : nullptr;}
int dStage_stagInfo_GetSaveTbl(int* stage){assert(stage);return *stage;}
bool dComIfGs_isStageMiddleBoss(){assert(hasStage);return liveMiddleBoss;}
struct StageSave {
    StageSave& getBit(){return *this;}
    bool isStageBossEnemy2(){return middleBoss[dStage_SaveTbl_LV6];}
} stageSave;
struct SaveData {
    StageSave& getSave(int stage){assert(stage==dStage_SaveTbl_LV6);return stageSave;}
} saveData;
SaveData* dComIfGs_getSaveData(){return &saveData;}
constexpr int dItemNo_SWORD_e=40;
bool ordonSword=false;
bool dComIfGs_isItemFirstBit(int item){assert(item==dItemNo_SWORD_e);return ordonSword;}
int maxLife=15;
bool save_state_boss_rush_active(){return bossRush;}
bool dComIfGs_isEventBit(int flag){return flags[flag];}
int dComIfGs_getMaxLife(){return maxLife;}
// MAPPING
int main(){
    auto state=progression_state();
    assert(!state.glide&&!state.gale&&!state.fierceDeity&&!state.dualWield&&state.charges==1);
    bossRush=true;
    for(int hearts:{3,5,6,9,20}){
        maxLife=hearts*5;state=progression_state();
        assert(state.glide&&state.gale&&state.fierceDeity&&state.dualWield);
        assert(state.charges==hearts/3);
        for(bool dawnlight:{false,true}){
            int64_t value=0;
            for(auto setting:{ModeSetting::Glide,ModeSetting::GlideItem,
                    ModeSetting::Gale,ModeSetting::GaleCounter,ModeSetting::FierceDeity,ModeSetting::DualWield})
                assert(mode_override(setting,dawnlight,true,state,value)&&value==1);
            assert(mode_override(ModeSetting::GaleCharges,dawnlight,true,state,value)&&value==hearts/3);
            if(!dawnlight)assert(!mode_override(ModeSetting::Glide,false,false,state,value));
        }
    }
    assert(!flags[0]&&!flags[1]&&!flags[2]); // Never alter the story save.
    bossRush=false;state=progression_state();
    assert(!state.glide&&!state.gale&&!state.fierceDeity&&!state.dualWield);
    flags[0]=true;state=progression_state();assert(state.glide&&!state.gale&&!state.fierceDeity);
    flags[1]=true;state=progression_state();assert(state.glide&&state.gale&&!state.fierceDeity);
    flags[2]=true;state=progression_state();assert(state.glide&&state.gale&&!state.fierceDeity);
    // Faron and other dungeon minibosses no longer unlock Dark Link.
    for(int stage=0;stage<10;++stage) if(stage!=dStage_SaveTbl_LV6) middleBoss[stage]=true;
    state=progression_state();assert(!state.fierceDeity);
    // Victory is visible immediately in live Temple of Time memory, before
    // leaving the dungeon copies the flag into the persistent stage save.
    currentStage=dStage_SaveTbl_LV6;
    ProgressionNotifications notifications;
    assert(notifications.update(true,progression_state())==0);
    liveMiddleBoss=true;state=progression_state();assert(state.fierceDeity);
    assert(notifications.update(true,state)==4);
    assert(notifications.update(true,state)==0);
    middleBoss[dStage_SaveTbl_LV6]=true;currentStage=0;liveMiddleBoss=false;
    state=progression_state();assert(state.fierceDeity);
    // The saved flag works outside the dungeon and on older completed saves.
    flags[2]=false;state=progression_state();assert(state.fierceDeity);
    hasStage=false;state=progression_state();assert(state.fierceDeity);
    middleBoss[dStage_SaveTbl_LV6]=false;state=progression_state();assert(!state.fierceDeity);
    hasStage=true;middleBoss[dStage_SaveTbl_LV6]=true;state=progression_state();
    for(bool dawnlight:{false,true}){
        int64_t value=0;
        assert(mode_override(ModeSetting::FierceDeity,dawnlight,true,state,value)&&value==1);
        middleBoss[dStage_SaveTbl_LV6]=false;
        auto locked=progression_state();
        assert(mode_override(ModeSetting::FierceDeity,dawnlight,true,locked,value)&&value==0);
        middleBoss[dStage_SaveTbl_LV6]=true;
    }
    int64_t personal=0;
    assert(!mode_override(ModeSetting::FierceDeity,false,false,state,personal));
    middleBoss[dStage_SaveTbl_LV6]=false;state=progression_state();assert(!state.fierceDeity); // another save
    assert(!state.dualWield); // Other story milestones are not the Ordon Sword.
    ordonSword=true;state=progression_state();assert(state.dualWield);
    ordonSword=false;state=progression_state();assert(!state.dualWield); // another save
}
'''.replace('// MAPPING', function)
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/'test.cpp', Path(tmp)/'test'
    cpp.write_text(fixture)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('Progression: Darknut live/saved victory, Faron/other miniboss exclusion, notifications, Boss Rush and Off settings passed')

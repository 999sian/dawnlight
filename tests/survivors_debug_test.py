"""Debug defaults, forced-off behavior, rarity floor, charges and run clocks."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=r'''
#include <cassert>
#include "survivors_rules.hpp"
using namespace dawnlight::survivors;
int main(){
 RunOptions options;assert(!options.debug.unlock_all_maps);
 options.debug.unlock_all_maps=true;options.debug.clamp();assert(options.debug.unlock_all_maps==debug_enabled);
 options.debug={4,10,1,1,5};Run run;run.start(71,options);
 assert(run.run_duration()==(debug_enabled?5u:20u)*60*30);
 assert(run.boss_interval()==run.run_duration()/4);
 assert(run.player_damage(123)==(debug_enabled?1230u:123u));
 assert(run.options.debug.minimum_rarity==(debug_enabled?4u:0u));
 assert(run.options.debug.infinite_rerolls==unsigned(debug_enabled));
 assert(run.options.debug.infinite_banishes==unsigned(debug_enabled));
 // Even stale/injected state cannot bypass a disabled build.
 run.options.debug={4,10,1,1,5};
 for(unsigned floor=0;floor<5;++floor){run.options.debug.minimum_rarity=floor;
  for(unsigned n=0;n<5000;++n){auto rarity=run.roll_rarity();if(debug_enabled)assert(unsigned(rarity)>=floor);}
 }
 run.options.debug.minimum_rarity=4;run.roll_upgrades();
 if(debug_enabled)for(auto rarity:run.offer_rarities)assert(rarity==Rarity::Legendary);
 run.free_rerolls=run.banishes=0;run.phase=Phase::Upgrade;unsigned rupees=0;
 for(unsigned i=0;i<100;++i){assert(run.pay_reroll(rupees)==debug_enabled);assert(rupees==0&&run.paid_rerolls==0);}
 assert(run.can_banish()==debug_enabled);
 for(auto u:{Upgrade::Power,Upgrade::Haste,Upgrade::Reach})assert(run.banish(u)==debug_enabled);
 assert(run.banishes==0);run.phase=Phase::TreasureChoice;
 assert(run.banish(ChestReward::Bow,true)==debug_enabled);
 assert(!run.pay_reroll(rupees,20)); // Free reroll never pays for the chest.
 rupees=20;if(debug_enabled){assert(run.pay_reroll(rupees,20)&&rupees==20);}
 for(unsigned minutes:{20u,10u,5u}){run.options.debug.run_minutes=minutes;
  const unsigned end=(debug_enabled?minutes:20u)*60*30;assert(run.run_duration()==end);
  run.phase=Phase::Running;run.ticks=end-1;run.horde_remaining=150;assert(!run.tick(false)&&run.ticks==end-1);
  assert(run.tick(true)&&run.ticks==end&&!run.horde_remaining);
 }
 run.options.debug.player_damage=999;assert(run.player_damage(100000)==32767);
 run.start(1);assert(run.run_duration()==duration&&run.player_damage(10)==10&&!run.infinite_rerolls()&&!run.infinite_banishes());
 DebugOptions invalid{99,0,5,5,7};invalid.clamp();assert(invalid.run_minutes==20&&invalid.player_damage==1);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(source)
 for enabled in [0,1]:
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',f'-DDAWNLIGHT_SURVIVORS_DEBUG={enabled}','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('Survivors debug: enabled/disabled defaults, rarity floor, infinite resources, damage clamp and timers passed')

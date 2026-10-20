"""Equipment distribution, persistence corruption, capacity and actual Run stats."""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=r'''
#include "survivors_rules.hpp"
#include <cassert>
#include <algorithm>
using namespace dawnlight::survivors;
int main() {
 GearInventory gear;
 assert(gear.equipped(GearKind::Sword)==GearItem{});
 unsigned counts[5]{};for(unsigned i=0;i<100;++i)++counts[GearInventory::stat(i)-1];
 assert(counts[0]==50&&counts[1]==25&&counts[2]==15&&counts[3]==8&&counts[4]==2);
 unsigned rolls[]{0,99,75,50};unsigned pos=0;
 auto sword=GearInventory::roll([&](unsigned n){assert(rolls[pos]<n);return rolls[pos++];});
 assert(sword.kind==GearKind::Sword&&sword.first==5&&sword.second==3&&sword.third==2&&pos==4);
 unsigned tunicRolls[]{1,0,99};pos=0;
 auto tunic=GearInventory::roll([&](unsigned n){assert(tunicRolls[pos]<n);return tunicRolls[pos++];});
 assert(tunic.kind==GearKind::Tunic&&tunic.first==1&&tunic.second==5&&tunic.third==0&&pos==3);
 for(auto kind:{GearKind::Sword,GearKind::Tunic}) {
  unsigned rarities[5]{};
  for(unsigned a=1;a<=5;++a)for(unsigned b=1;b<=5;++b)for(unsigned c=kind==GearKind::Sword?1:0;c<=(kind==GearKind::Sword?5u:0u);++c){
   GearItem item{kind,uint8_t(a),uint8_t(b),uint8_t(c)};assert(item.valid());++rarities[item.rarity()];
   auto better=item;if(better.first<5){++better.first;assert(better.rarity()>=item.rarity());}
  }
  for(auto n:rarities)assert(n);
 }
 for(unsigned i=0;i<20;++i)assert(gear.add(i%2?tunic:sword)==i);
 auto full=gear;assert(gear.add(sword)==20&&gear==full);
 assert(gear.equip(0)&&gear.equip(1)&&gear.sword==0&&gear.tunic==1);
 assert(gear.equip(2)&&gear.sword==2&&gear.tunic==1);
 assert(gear.erase(2)&&gear.sword==20&&gear.tunic==1);
 assert(gear.add(sword)==2); // First free slot, row-major; never displace equipped items.
 assert(!gear.equip(20)&&!gear.erase(20)&&gear.add(GearItem{})==20);
 gear.equip(2);
 auto bytes=gear.encode();GearInventory loaded;assert(loaded.decode(bytes)&&loaded==gear);
 for(unsigned i=0;i<bytes.size();++i){auto broken=bytes;broken[i]^=0x80;assert(!loaded.decode(broken)&&loaded==gear);}
 assert(!loaded.decode(std::span(bytes.data(),bytes.size()-1))&&loaded==gear);
 Run base;base.start(123);Run equipped=base;equipped.equip_gear(gear);
 assert(equipped.maximum_hearts==base.maximum_hearts+tunic.first);
 assert(equipped.melee_power()==base.melee_power()+sword.first);
 assert(equipped.attack_power(unsigned(Upgrade::Spin))==base.attack_power(unsigned(Upgrade::Spin))+sword.first);
 assert(equipped.attack_power(unsigned(Upgrade::Wave))==base.attack_power(unsigned(Upgrade::Wave))+sword.first);
 assert(equipped.lightning_power()==base.lightning_power()+sword.first);
 assert(equipped.attack_power(unsigned(Upgrade::Bow))==base.attack_power(unsigned(Upgrade::Bow)));
 assert(equipped.melee_crit_bonus()==base.melee_crit_bonus()+sword.second*100);
 assert(equipped.dodge_basis_points()==base.dodge_basis_points()+tunic.second*100);
 assert(equipped.melee_speed_percent()==base.melee_speed_percent()+sword.third);
 gear.erase(1);assert(equipped.gear_tunic==tunic); // Active build never changes with inventory.
 equipped.phase=Phase::Victory;assert(equipped.continue_endless()&&equipped.gear_sword==sword);
 equipped.start(444);assert(equipped.gear_sword==GearItem{});
 // Equipment storage cannot be touched by high-score reset or migration.
 Records records;records.clear_high_scores();GearInventory check;assert(check.decode(bytes)&&check==loaded);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(source)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors equipment: distributions, all rarities, capacity, selection, corruption and combat bonuses passed')

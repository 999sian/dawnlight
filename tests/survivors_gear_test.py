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
 assert(gear.equipped(GearSlot::Weapon)==GearItem{});
 unsigned counts[5]{};for(unsigned i=0;i<100;++i)++counts[GearInventory::stat(i)-1];
 assert(counts[0]==50&&counts[1]==25&&counts[2]==15&&counts[3]==8&&counts[4]==2);
 unsigned rolls[]{0,0,99,75,50};unsigned pos=0;
 auto sword=GearInventory::roll([&](unsigned n){assert(rolls[pos]<n);return rolls[pos++];});
 assert(sword.kind==GearKind::Sword&&sword.first==5&&sword.second==3&&sword.third==2&&pos==5);
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
 for(unsigned i=0;i<GearInventory::capacity;++i)assert(gear.add(i%2?tunic:sword)==i);
 auto full=gear;assert(gear.add(sword)==GearInventory::none&&gear==full);
 assert(gear.equip(0)&&gear.equip(1)&&gear.weapon==0&&gear.tunic==1);
 assert(gear.equip(2)&&gear.weapon==2&&gear.tunic==1);
 assert(gear.erase(2)&&gear.weapon==GearInventory::none&&gear.tunic==1);
 assert(gear.add(sword)==2); // First free slot, row-major; never displace equipped items.
 assert(!gear.equip(GearInventory::none)&&!gear.erase(GearInventory::none)&&gear.add(GearItem{})==GearInventory::none);
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
 equipped.phase=Phase::Victory;assert(equipped.continue_endless()&&equipped.gear_weapon==sword);
 equipped.start(444);assert(equipped.gear_weapon==GearItem{});
 // Equipment storage cannot be touched by high-score reset or migration.
 Records records;records.clear_high_scores();GearInventory check;assert(check.decode(bytes)&&check==loaded);
 // Validate every variant, all roll combinations and normalized rarities.
 for(unsigned k=1;k<unsigned(GearKind::Count);++k) {
  auto kind=GearKind(k);unsigned rarities[5]{};
  GearItem item{kind,1,1,uint8_t(kind==GearKind::Tunic?0:1)};
  for(unsigned a=1;a<=5;++a)for(unsigned b=1;b<=5;++b)for(unsigned c=kind==GearKind::Tunic?0:1;c<=(kind==GearKind::Tunic?0u:5u);++c) {
   item.first=a;item.second=b;item.third=c;if(!item.valid())continue;
   ++rarities[item.rarity()];
   GearInventory inv;assert(inv.add(item)==0&&inv.equip(0));
   if(item.is_weapon())assert(inv.weapon==0&&inv.tunic==GearInventory::none);else assert(inv.tunic==0&&inv.weapon==GearInventory::none);
   GearInventory roundtrip;assert(roundtrip.decode(inv.encode())&&roundtrip==inv);
  }
  for(auto n:rarities)assert(n);
 }
 // Both legacy formats preserve their 20 entries/indices, initialize page
 // two empty and translate old sentinel 20 to new sentinel 40.
 const auto checksum=[](auto& b){uint32_t hash=2166136261u;for(unsigned i=0;i<b.size()-4;++i)hash=(hash^b[i])*16777619u;for(unsigned i=0;i<4;++i)b[b.size()-4+i]=uint8_t(hash>>(8*i));};
 GearInventory migrated,legacyExpected;
 std::copy_n(loaded.items.begin(),20,legacyExpected.items.begin());legacyExpected.weapon=loaded.weapon;legacyExpected.tunic=loaded.tunic;
 std::array<uint8_t,GearInventory::legacy_byte_count> legacy{};
 std::copy_n(bytes.begin(),legacy.size()-4,legacy.begin());
 for(uint8_t version:{1,2}) {
  legacy[4]=version;checksum(legacy);assert(migrated.decode(legacy)&&migrated==legacyExpected);
  for(unsigned i=20;i<40;++i)assert(migrated.items[i]==GearItem{});
  assert(migrated.encode()[4]==3);
  auto unequipped=legacy;unequipped[5]=unequipped[6]=20;checksum(unequipped);
  assert(migrated.decode(unequipped)&&migrated.weapon==40&&migrated.tunic==40);
 }
 // v2 can contain specialized weapons, v1 cannot. Page-two slot 20 is a
 // real equipped item in v3, never the legacy empty sentinel.
 legacy[4]=2;legacy[8]=uint8_t(GearKind::HerosBow);checksum(legacy);
 assert(migrated.decode(legacy)&&migrated.items[0].kind==GearKind::HerosBow);
 legacy[4]=1;checksum(legacy);assert(!migrated.decode(legacy));
 auto secondPage=loaded;assert(secondPage.equip(20)&&secondPage.weapon==20);
 auto secondBytes=secondPage.encode();assert(migrated.decode(secondBytes)&&migrated==secondPage);
 assert(secondPage.unequip(20)&&secondPage.weapon==40&&secondPage.items[20]==loaded.items[20]);
 assert(!secondPage.unequip(20)&&!secondPage.unequip(40));
 assert(migrated.decode(bytes)&&migrated==loaded);
 auto invalid=bytes;invalid[8]=uint8_t(GearKind::Count);checksum(invalid);assert(!migrated.decode(invalid)&&migrated==loaded);
 invalid=bytes;invalid[5]=1;checksum(invalid);assert(!migrated.decode(invalid));
 invalid=bytes;invalid[4]=4;checksum(invalid);assert(!migrated.decode(invalid));
 invalid=bytes;invalid[4]=2;checksum(invalid);assert(!migrated.decode(invalid)); // Version/size mismatch.
 invalid=bytes;invalid[8]=uint8_t(GearKind::SharpshooterBow);invalid[11]=3;checksum(invalid);assert(!migrated.decode(invalid));
 for(unsigned n=0;n<GearInventory::legacy_byte_count;++n)assert(!migrated.decode(std::span(bytes.data(),n)));
 // Inventory rows fill page one before page two, including holes after deletion.
 GearInventory pages;for(unsigned i=0;i<40;++i)assert(pages.add(sword)==i);
 assert(pages.add(tunic)==40&&pages.equip(39)&&pages.weapon==39);
 assert(pages.erase(20)&&pages.add(tunic)==20&&pages.equip(20)&&pages.tunic==20);
 assert(pages.unequip(39)&&pages.weapon==40&&pages.tunic==20&&pages.items[39]==sword);
 assert(pages.unequip(20)&&pages.tunic==40&&pages.items[20]==tunic);
 // Roll every variant deterministically; count affixes have exactly 90/10 odds.
 for(unsigned variant=0;variant<10;++variant)for(unsigned roll=0;roll<100;++roll) {
  unsigned calls=0;auto item=GearInventory::roll([&](unsigned n){unsigned value=calls++==0?0:calls==2?variant:roll;assert(value<n);return value;});
  assert(item.valid()&&item.is_weapon());
  if(item.kind==GearKind::SharpshooterBow)assert(item.third==(roll<90?1:2));
  if(item.kind==GearKind::HurricaneBoomerang)assert(item.second==(roll<90?1:2));
 }
 Run unarmed;unarmed.start(123);
 const auto make=[&](GearKind kind,unsigned value=5){auto r=unarmed;r.gear_weapon={kind,uint8_t(value),uint8_t(value),uint8_t(value)};if(kind==GearKind::SharpshooterBow)r.gear_weapon.third=value==5?2:1;if(kind==GearKind::HurricaneBoomerang)r.gear_weapon.second=value==5?2:1;assert(r.gear_weapon.valid());return r;};
 for(auto kind:{GearKind::HerosBow,GearKind::SharpshooterBow,GearKind::StormBoomerang,GearKind::HurricaneBoomerang,GearKind::BombBag,GearKind::SiegeBombs,GearKind::IronBall,GearKind::BreakerBall}) {
  auto r=make(kind);assert(r.melee_power()==unarmed.melee_power()&&r.melee_speed_percent()==100&&r.melee_crit_bonus()==0);
  for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball}) {
   assert(r.attack_power(unsigned(weapon))==unarmed.attack_power(unsigned(weapon))); // Still locked.
   r.ranks[unsigned(weapon)]=1;
  }
  Run plain=r;plain.gear_weapon={};
  for(auto weapon:{Upgrade::Bow,Upgrade::Boomerang,Upgrade::Bombs,Upgrade::Ball})
   assert(r.attack_power(unsigned(weapon))==plain.attack_power(unsigned(weapon))+(r.gear_family(weapon)==r.gear_weapon.family()?5:0));
 }
 for(unsigned value:{1u,5u}) {
  auto great=make(GearKind::Greatsword,value);
  assert(great.melee_power()==unarmed.melee_power()+value&&great.melee_crit_bonus()==0&&great.melee_speed_percent()==100);
  assert(std::abs(great.melee_reach(100)-float(100+2*value))<.001f);
  assert(std::abs(great.melee_area_radius(100)-float(100+2*value))<.001f);
  great.ranks[unsigned(Upgrade::Knockback)]=1;
  assert(std::abs(great.knockback_distance(false,false,DamageSource::Sword)-40.f*(1+.05f*value))<.001f);
  assert(great.knockback_distance(false,false,DamageSource::Bow)==40);
  assert(great.knockback_distance(false,true,DamageSource::Sword)==0);
  auto sniper=make(GearKind::SharpshooterBow,value);sniper.ranks[unsigned(Upgrade::Bow)]=1;
  assert(sniper.arrow_targets()==(value==5?3u:2u));
  assert(std::abs(sniper.item_range(Upgrade::Bow)-item_target_range*(1+.05f*value))<.001f);
  sniper.ranks[unsigned(Upgrade::Piercing)]=10;assert(sniper.arrow_targets()==(value==5?13u:12u));
  auto storm=make(GearKind::StormBoomerang,value);storm.ranks[unsigned(Upgrade::Boomerang)]=1;
  assert(std::abs(storm.boomerang_hop_range()-800*(1+.05f*value))<.001f);
  assert(storm.item_cooldown(Upgrade::Boomerang)<unarmed.item_cooldown(Upgrade::Boomerang));
  auto hurricane=make(GearKind::HurricaneBoomerang,value);hurricane.ranks[unsigned(Upgrade::Boomerang)]=1;
  assert(hurricane.boomerang_hops()==(value==5?3u:2u));
  assert(std::abs(hurricane.item_flight_speed(Upgrade::Boomerang)-42*(1+.05f*value))<.001f);
  assert(std::abs(hurricane.item_flight_speed(Upgrade::Boomerang,true)-55*(1+.05f*value))<.001f);
  assert(hurricane.hurricane_scale()==1); // Equipment name does not grant legendary Hurricane.
  hurricane.ranks[unsigned(Upgrade::Boomerang)]=30;assert(hurricane.boomerang_hops()==(value==5?12u:11u));
  for(auto kind:{GearKind::BombBag,GearKind::SiegeBombs}) {
   auto bombs=make(kind,value);bombs.ranks[unsigned(Upgrade::Bombs)]=1;
   assert(std::abs(bombs.bomb_radius()-182*(1+.02f*value))<.001f);
   assert(bombs.bomb_arrow_radius()==unarmed.bomb_arrow_radius());
   if(kind==GearKind::BombBag)assert(bombs.item_cooldown(Upgrade::Bombs)<unarmed.item_cooldown(Upgrade::Bombs));
   else {bombs.ranks[unsigned(Upgrade::Knockback)]=1;assert(std::abs(bombs.knockback_distance(false,false,DamageSource::Bombs)-40*(1+.05f*value))<.001f);assert(bombs.knockback_distance(false,false,DamageSource::Sword)==40);}
  }
  auto iron=make(GearKind::IronBall,value);iron.ranks[unsigned(Upgrade::Ball)]=1;
  assert(std::abs(iron.ball_range()-900*(1+.05f*value))<.001f);
  assert(iron.item_cooldown(Upgrade::Ball)<unarmed.item_cooldown(Upgrade::Ball));
  auto breaker=make(GearKind::BreakerBall,value);breaker.ranks[unsigned(Upgrade::Ball)]=1;
  assert(std::abs(breaker.item_flight_speed(Upgrade::Ball)-40*(1+.05f*value))<.001f);
  assert(std::abs(breaker.item_flight_speed(Upgrade::Ball,true)-55*(1+.05f*value))<.001f);
  assert(std::abs(breaker.knockback_distance(true,false,DamageSource::Ball)-240*(1+.05f*value))<.001f);
 }
 auto bow=make(GearKind::HerosBow);bow.ranks[unsigned(Upgrade::Bow)]=1;
 unsigned bowCrits=0;for(unsigned i=0;i<30000;++i){bool hit=false;bow.critical_power(10,&hit,false,true);bowCrits+=hit;}
 assert(bowCrits>1300&&bowCrits<1700);
 for(unsigned i=0;i<1000;++i){bool hit=true;bow.critical_power(10,&hit,true);assert(!hit);bow.critical_power(10,&hit);assert(!hit);}
 assert(bow.item_cooldown(Upgrade::Bow)<unarmed.item_cooldown(Upgrade::Bow));
 // Equipment hearts keep the 20-heart maximum; other equipment bonuses
 // remain effective after normal upgrade caps and cooldown floors.
 GearInventory cappedGear;cappedGear.add({GearKind::Tunic,5,5,0});cappedGear.equip(0);
 Run health;health.start(1);health.equip_gear(cappedGear);
 health.maximum_hearts=19;assert(health.eligible(Upgrade::Heart));
 health.phase=Phase::Upgrade;health.offers[0]=Upgrade::Heart;health.offer_rarities[0]=Rarity::Legendary;
 assert(health.choose(0)&&health.maximum_hearts==20&&!health.eligible(Upgrade::Heart));
 assert(health.dodge_basis_points()==health.talents.dodge_basis_points()+500);
 for(auto kind:{GearKind::HerosBow,GearKind::StormBoomerang,GearKind::BombBag,GearKind::IronBall}) {
  auto r=make(kind);auto weapon=kind==GearKind::HerosBow?Upgrade::Bow:kind==GearKind::StormBoomerang?Upgrade::Boomerang:kind==GearKind::BombBag?Upgrade::Bombs:Upgrade::Ball;
  r.ranks[unsigned(weapon)]=30;r.ranks[unsigned(Upgrade::Haste)]=30;
  auto plain=r;plain.gear_weapon={};assert(r.item_cooldown(weapon)<plain.item_cooldown(weapon));
 }
 auto fastSword=make(GearKind::Sword);fastSword.dual_wield=30;fastSword.ranks[unsigned(Upgrade::Haste)]=30;
 auto plainSword=fastSword;plainSword.gear_weapon={};assert(fastSword.melee_cooldown()<plainSword.melee_cooldown());
 // Switching families replaces the one Weapon slot without losing Tunic.
 GearInventory shared;shared.add(tunic);shared.equip(0);
 for(auto kind:{GearKind::Sword,GearKind::HerosBow,GearKind::StormBoomerang,GearKind::BombBag,GearKind::IronBall}) {
  auto index=shared.add({kind,1,1,1});assert(shared.equip(index)&&shared.weapon==index&&shared.tunic==0);
 }

}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(source)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors equipment: distributions, all rarities, capacity, selection, corruption and combat bonuses passed')

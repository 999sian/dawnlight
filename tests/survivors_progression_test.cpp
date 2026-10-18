#include "../src/survivors_rules.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight;
using namespace dawnlight::survivors;
int main(){
    Run r;r.start(1);assert(r.score()==0);
    r.ticks=60*30;r.kills=100;r.level=5;assert(r.score()==1320);
    r.timed_elites=1;r.guard_elites=2;r.kills=103;assert(r.score()==2320);
    r.options.horde=5;r.kills=503;assert(r.score()==3248); // Same normalized kills, +40% difficulty.
    r.options={10,10,3,5,500};assert(r.options.score_percent()==415&&r.score()==9628);
    r.end(true);assert(r.score()==22078);
    Records records;records.finish(r,2);assert(records.high_scores[2]==22078&&records.total_xp==22078);
    assert(records.high_scores[0]==0&&records.runs==1&&records.wins==1);
    assert(records.level()==7&&records.available_points()==6&&records.level_xp()==2578&&records.next_level_xp()==5000);
    Records untouched=records;r.start(1);records.finish(r,0);assert(records.encode()==untouched.encode());
    r.end(false);records.finish(r,5);assert(records.encode()==untouched.encode());
    records.finish(r,2);assert(records.high_scores[2]==22078); // Never replace a higher score.
    records.total_xp=meta_threshold(27);assert(records.available_points()==27);
    assert(!records.buy(Talent::SprintSpeed)&&!records.buy(Talent::Glider)&&!records.buy(Talent::Gale)&&!records.buy(Talent::GaleCharges));
    for(unsigned i=0;i<unsigned(Talent::Count);++i){
        for(unsigned n=0;n<talent_caps[i];++n)assert(records.buy(Talent(i)));
        assert(!records.buy(Talent(i)));
    }
    assert(records.available_points()==0&&records.talents.spent()==27&&records.talents.valid());
    r.start(3,{},records.talents);
    assert(r.maximum_hearts==8&&r.free_rerolls==3&&r.banishes==3);
    assert(r.rank(Upgrade::Heart)==0&&r.rank(Upgrade::Magnet)==0&&r.rank(Upgrade::CritChance)==0);
    assert(r.magnet_range()==225&&r.crit_chance_basis_points()==500);
    r.ranks[unsigned(Upgrade::CritChance)]=30;r.ranks[unsigned(Upgrade::Luck)]=20;
    assert(r.crit_chance_basis_points()==10000);
    for(int i=0;i<10000;++i)assert(r.critical_power(100)==200);
    r.ranks[unsigned(Upgrade::Magnet)]=30;assert(r.magnet_range()==3150);
    unsigned money=0;for(int i=0;i<3;++i){r.phase=Phase::Upgrade;assert(r.pay_reroll(money));assert(r.banish(Upgrade(i==0?3:i==1?4:5)));}
    assert(!r.pay_reroll(money)&&r.reroll_cost()==50&&!r.banish(Upgrade::Magnet));
    r.start(3,{},records.talents);assert(r.free_rerolls==3&&r.banishes==3&&r.paid_rerolls==0);
    r.start(3);assert(r.maximum_hearts==3&&r.crit_chance_basis_points()==0&&r.magnet_range()==150);
    int64_t value=99;
    for(auto setting:{ModeSetting::Sprint,ModeSetting::Jump,ModeSetting::Glide,ModeSetting::Gale}){
        assert(talent_mode_override({},setting,value)&&value==0);
        assert(talent_mode_override(records.talents,setting,value)&&value==1);
    }
    assert(talent_mode_override(records.talents,ModeSetting::GaleCharges,value)&&value==3);
    assert(talent_mode_override({},ModeSetting::GaleCharges,value)&&value==1);
    assert(records.talents.sprint_multiplier()==1.5f);
    Records loaded;const auto bytes=records.encode();assert(loaded.decode(bytes)&&loaded.encode()==bytes);
    auto corrupt=bytes;corrupt[4]=99;assert(!loaded.decode(corrupt)&&loaded.encode()==bytes);
    corrupt=bytes;corrupt[56]=6;assert(!loaded.decode(corrupt)); // Max Health exceeds cap.
    corrupt=bytes;corrupt[76]=0;assert(!loaded.decode(corrupt)); // Glider without Manual Jump.
    corrupt=bytes;for(unsigned i=48;i<56;++i)corrupt[i]=0;assert(!loaded.decode(corrupt)); // Unfunded ranks.
    assert(!loaded.decode(std::span(bytes.data(),bytes.size()-1)));
    std::array<uint8_t,28> legacy{};const std::array<uint32_t,7> words{0x56525348,1,5,2,36000,400,18};
    for(unsigned i=0;i<7;++i)for(unsigned b=0;b<4;++b)legacy[i*4+b]=uint8_t(words[i]>>(8*b));
    assert(loaded.decode(legacy)&&loaded.runs==5&&loaded.wins==2&&loaded.best_level==18);
    assert(loaded.total_xp==0&&loaded.level()==1&&loaded.available_points()==0&&loaded.talents.spent()==0);
    for(unsigned n=0;n<100;++n){assert(meta_levels(meta_threshold(n))==n);if(n)assert(meta_levels(meta_threshold(n)-1)==n-1);}
    std::cout<<"Survivors progression: score, map records, XP, talents, run reset and save migration passed\n";
}

#include "../src/survivors_rules.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    // Critical chance scales with full Luck, independently of the drop bonus cap.
    Run crit;crit.start(98765);
    const auto initialRandom=crit.random;
    assert(crit.critical_power(7)==7&&crit.random==initialRandom);
    assert(crit.crit_damage_percent()==200);
    for(unsigned chanceRank:{1u,30u})for(unsigned luckRank:{0u,10u,20u}){
        crit.start(98765);crit.ranks[unsigned(Upgrade::CritChance)]=chanceRank;
        crit.ranks[unsigned(Upgrade::Luck)]=luckRank;
        crit.ranks[unsigned(Upgrade::CritDamage)]=30;
        const unsigned expected=chanceRank*(100+10*luckRank);
        assert(crit.crit_chance_basis_points()==expected&&crit.crit_damage_percent()==350);
        const auto before=crit.random;assert(crit.critical_power(0)==0&&crit.random==before);
        unsigned criticalHits=0;
        constexpr unsigned trials=200000;
        for(unsigned i=0;i<trials;++i){const auto hit=crit.critical_power(100);
            assert(hit==100||hit==350);criticalHits+=hit==350;}
        assert(std::abs(int(criticalHits*10000/trials)-int(expected))<40);
    }
    for(auto power:{Upgrade::CritChance,Upgrade::CritDamage}){
        assert(upgrade_cap(power)==30);
        crit.start(1);crit.ranks[unsigned(power)]=29;crit.phase=Phase::Upgrade;
        crit.offers[0]=power;crit.offer_rarities[0]=Rarity::Legendary;
        assert(crit.eligible(power)&&crit.upgrade_gain(power,Rarity::Legendary)==1);
        assert(crit.upgrade_rupees(power,Rarity::Legendary)==80&&crit.choose(0));
        assert(crit.rank(power)==30&&!crit.eligible(power));
        crit.start(1);crit.ranks[unsigned(power)]=5;crit.phase=Phase::Upgrade;
        assert(crit.banish(power)&&crit.rank(power)==5&&!crit.eligible(power));
        crit.start(1);assert(crit.rank(power)==0&&crit.eligible(power));
    }
    for(unsigned rank=0;rank<=30;++rank){
        crit.ranks[unsigned(Upgrade::CritDamage)]=rank;
        assert(crit.crit_damage_percent()==200+5*rank);
    }
    crit.ranks[unsigned(Upgrade::CritChance)]=30;crit.random=1;
    assert(crit.critical_power(3)==11); // x3.5 rounds once to native integer health.
    // Published chart checkpoints (basis points), sampled from actual rarity rolls.
    constexpr std::array<std::array<unsigned,5>,7> chart{{
        {6000,2500,1000,400,100}, {5640,2585,1137,500,138},
        {4957,2685,1396,726,236}, {4330,2706,1624,974,365},
        {3030,2525,2020,1616,808}, {2094,2181,2181,2181,1363},
        {1449,1812,2174,2609,1957}}};
    unsigned row=0;
    for(unsigned luck:{0u,1u,3u,5u,10u,15u,20u}) {
        Run sample;sample.start(98765);sample.ranks[unsigned(Upgrade::Luck)]=luck;
        assert(sample.luck_percent()==100+10*luck);
        assert(sample.lucky_chance(2000)==2000+200*std::min(5u,luck));
        constexpr unsigned trials=500000;
        std::array<unsigned,5> rarities{};
        std::array<unsigned,unsigned(Run::Drop::Count)> loot{};
        for(unsigned n=0;n<trials;++n){++rarities[unsigned(sample.roll_rarity())];++loot[unsigned(sample.roll_drop())];}
        for(unsigned i=0;i<5;++i)assert(std::abs(int(rarities[i]*10000/trials)-int(chart[row][i]))<40);
        ++row;
        assert(std::abs(int(loot[1]*10000/trials)-int(sample.lucky_chance(500)))<30);
        assert(std::abs(int(loot[0]*10000/trials)-int(10000-sample.lucky_chance(5000)))<40);
        const unsigned rupees=trials-loot[0]-loot[1];
        const unsigned valuable=rupees-loot[unsigned(Run::Drop::GreenRupee)];
        assert(std::abs(int(valuable*10000/rupees)-int(sample.lucky_chance(1700)))<80);
        for(unsigned i=1;i<loot.size();++i)assert(loot[i]); // Every native color is reachable.
    }
    // All Luck ranks remain eligible up to 20; rarity grants/overflow use that cap.
    for(unsigned initial=0;initial<=20;++initial)for(unsigned tier=0;tier<5;++tier){
        Run r;r.start(1);r.ranks[unsigned(Upgrade::Luck)]=initial;
        assert(r.eligible(Upgrade::Luck)==(initial<20));
        const unsigned gain=std::min(20-initial,tier+1);
        assert(r.upgrade_gain(Upgrade::Luck,Rarity(tier))==gain);
        assert(r.upgrade_rupees(Upgrade::Luck,Rarity(tier))==20*(tier+1-gain));
        if(initial<20){r.phase=Phase::Upgrade;r.offers[0]=Upgrade::Luck;r.offer_rarities[0]=Rarity(tier);
            assert(r.choose(0)&&r.luck_percent()==100+10*(initial+gain));}
    }
    // Higher rarity Luck must not inflate enemy or pot drops past their old cap.
    Run capped,maximum;capped.start(58);maximum.start(58);
    capped.ranks[unsigned(Upgrade::Luck)]=5;maximum.ranks[unsigned(Upgrade::Luck)]=20;
    for(unsigned n=0;n<10000;++n){assert(capped.roll_drop()==maximum.roll_drop());
        assert(capped.roll_pot_drop()==maximum.roll_pot_drop());}
    maximum.start(58);assert(maximum.luck_percent()==100&&maximum.eligible(Upgrade::Luck));
    for(unsigned rarity=0;rarity<5;++rarity) {
        for(unsigned u=0;u<unsigned(Upgrade::Rupees);++u)for(unsigned initial:{0u,4u}) {
            Run r;r.start(13);r.shield=1;r.red_potion=true;r.ranks[8]=1;r.ranks[u]=initial;
            // Weapons must be chest-unlocked first.
            if(u>=unsigned(Upgrade::Bow)&&u<=unsigned(Upgrade::Ball)&&!initial)continue;
            r.phase=Phase::Upgrade;r.offers[0]=Upgrade(u);r.offer_rarities[0]=Rarity(rarity);
            const auto random=r.random;const unsigned gain=std::min(upgrade_cap(Upgrade(u))-initial,rarity+1);
            assert(r.upgrade_rupees(Upgrade(u),Rarity(rarity))==20*(rarity+1-gain));
            assert(r.choose(0)&&r.rank(Upgrade(u))==initial+gain&&r.random==random);
            assert(!r.choose(0));
        }
        for(unsigned item=0;item<unsigned(ChestReward::Count);++item) {
            Run r;r.start(14);r.phase=Phase::Treasure;r.master_sword=4;r.shield=9;r.ranks[8]=4;
            const auto reward=ChestReward(item);const auto gain=r.chest_gain(reward,Rarity(rarity),3);
            const unsigned extra=r.chest_rupees(reward,Rarity(rarity),3);unsigned money=20;
            assert(gain+extra/20==rarity+1);
            assert(r.buy_chest(money,reward,3,Rarity(rarity))&&money==extra&&r.chests_opened==1);
            assert(r.master_sword<=5&&r.shield<=10&&r.rank(Upgrade::Bow)<=30);
        }
    }
    Run run;
    assert(std::all_of(run.ranks.begin(),run.ranks.end(),[](auto r){return r==0;}));
    std::array<unsigned,unsigned(Run::Drop::Count)> drops{};
    run.start(123);
    for(unsigned i=0;i<10000;++i) ++drops[static_cast<unsigned>(run.roll_drop())];
    assert(drops[1]>350 && drops[1]<650); // Rare hearts, not an every-kill heal.
    assert(drops[2]>3500 && drops[2]<4500 && drops[3]>350 && drops[3]<650);
    run = {};
    assert(!run.tick(true));
    run.start(0);
    for (unsigned i = 0; i < duration; ++i) {
        const auto before = run.ticks;
        assert(!run.tick(false) && run.ticks == before);
        assert(run.tick(true));
        assert(run.target_population() < enemy_capacity);
        assert(run.spawn_interval() >= 15);
    }
    assert(run.ticks == duration && run.phase == Phase::Running);
    run.xp = 100;
    assert(run.level_up());
    const auto offers = run.offers;
    assert(offers[0] != offers[1] && offers[0] != offers[2] && offers[1] != offers[2]);
    assert(!run.tick(true) && run.ticks == duration);
    assert(!run.level_up() && run.offers == offers); // Closing/reopening cannot reroll.
    assert(!run.choose(3));
    const auto previous = run.rank(offers[0]);
    const auto gain = run.upgrade_gain(offers[0], run.offer_rarities[0]);
    assert(run.choose(0));
    if (offers[0] != Upgrade::Rupees) assert(run.rank(offers[0]) == previous + gain);
    assert(!run.choose(0)); // Double activation is ignored.
    assert(run.level_up()); // Excess XP remains available.
    for (uint32_t seed = 1; seed < 1000; ++seed) {
        run.start(seed);
        run.ranks.fill(30); run.ranks[unsigned(Upgrade::Luck)]=20; run.shield=1; run.ranks[2] = 29; run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::ShieldAttack || offer == Upgrade::Rupees);
        assert(std::count(run.offers.begin(), run.offers.end(), Upgrade::ShieldAttack) == 1);
        run.start(seed); run.ranks.fill(30); run.ranks[unsigned(Upgrade::Luck)]=20; run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::Rupees);
        assert(run.choose(2));
        assert(run.cooldown(40) == 6);
    }
    run.start(1);
    run.ranks[3] = run.ranks[4] = run.ranks[5] = run.ranks[6] = 1;
    assert(run.eligible(Upgrade::Heart)); // Hearts are independent of other passives.
    assert(run.eligible(Upgrade::Power) && run.eligible(Upgrade::Spin) && run.eligible(Upgrade::Rupees));
    // Chest weapons cannot be acquired from level-up offers before a purchase.
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i)
        assert(!run.eligible(static_cast<Upgrade>(i)));
    assert(!run.eligible(Upgrade::FireArrows) && !run.eligible(Upgrade::ShieldAttack));
    run.ranks[8]=1;run.shield=1;
    assert(run.eligible(Upgrade::FireArrows) && run.eligible(Upgrade::ShieldAttack));
    run.ranks[8]=0;run.shield=0;
    for(unsigned rank=1;rank<=10;++rank){run.shield=rank;assert(run.block_chance()==rank*5);
        assert(run.shield_tier()==(rank>=7?2u:rank>=4?1u:0u));}
    run.shield=0;
    unsigned wallet = 19;
    ChestReward reward = ChestReward::Bow;
    run.phase = Phase::Treasure;
    const auto rng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 19 && run.random == rng && run.chest_cost() == 20);
    wallet = 20;
    assert(run.buy_chest(wallet, reward) && wallet == 0 && run.chest_cost() == 25);
    assert((reward == ChestReward::OrdonSword && run.dual_wield) || (reward == ChestReward::Shield && run.shield==1) ||
           (reward == ChestReward::DarkLink && run.dark_link) ||
           (reward == ChestReward::MasterSword && run.master_sword) ||
           (run.rank(chest_weapon(reward)) == 1 && run.eligible(chest_weapon(reward)))); // Even with four occupied passives.
    assert(!run.tick(true)); // Get Item cannot advance encounter time.
    run.start(20); run.phase = Phase::Treasure;
    wallet = 20000000;
    for (unsigned n = 0; n < 120; ++n) {
        assert(run.chest_cost() == 20+n*(n+4));
        reward = ChestReward(n%4);
        assert(run.buy_chest(wallet, reward) && run.rank(chest_weapon(reward)) <= 30);
    }
    assert(wallet == 19400220 && !run.chest_rewards_left());
    assert(run.equipment_count()==4&&!run.chest_eligible(ChestReward::MasterSword));
    assert(!run.chest_rewards_left(true)); // Four weapons leave no slot for a first potion.
    const auto maxedRng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 19400220 && run.random == maxedRng);
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i) {
        const auto weapon = static_cast<Upgrade>(i);
        assert(run.rank(weapon) == 30 && !run.eligible(weapon));
        const auto fast = run.item_cooldown(weapon);
        run.ranks[i] = 1;
        assert(run.item_cooldown(weapon) > fast);
        run.ranks[i] = 30;
    }
    run.start(20);run.phase=Phase::Treasure;run.banned_items.fill(true);run.banned_items[unsigned(ChestReward::RedPotion)]=false;
    wallet = 100000;
    reward = run.roll_chest_offers(true)[0];
    assert(run.buy_chest(wallet, reward, true) && reward == ChestReward::RedPotion);
    // Offers are distinct, capped and do not purchase anything. Buy exactly the chosen reward.
    for (unsigned seed=1; seed<100; ++seed) {
        Run choices; choices.start(seed);
        const auto offers=choices.roll_chest_offers(true);
        assert(offers[0]!=offers[1]&&offers[0]!=offers[2]&&offers[1]!=offers[2]);
        for(auto offered:offers)assert(choices.chest_eligible(offered,true));
        assert(choices.chests_opened==0);
        choices.phase=Phase::Treasure;unsigned money=20;const auto rng=choices.random;
        assert(choices.buy_chest(money,ChestReward::Shield)&&choices.shield==1);
        assert(!choices.master_sword&&!choices.dual_wield&&choices.rank(Upgrade::Bow)==0);
        assert(choices.random==rng&&money==0);
        assert(!choices.buy_chest(money,ChestReward::Count));
        choices.ranks.fill(30);choices.ranks[unsigned(Upgrade::Ball)]=0;choices.shield=9;choices.banned_items[unsigned(ChestReward::OrdonSword)]=true;
        auto few=choices.roll_chest_offers(true);
        assert(few[0]==ChestReward::Shield&&few[1]==ChestReward::Count&&few[2]==ChestReward::Count);
        assert((few[0]==ChestReward::Shield||few[1]==ChestReward::Shield));
        few=choices.roll_chest_offers(false);
        assert(few[0]==ChestReward::Shield&&few[1]==ChestReward::Count&&few[2]==ChestReward::Count);
        choices.shield=10;few=choices.roll_chest_offers();
        for(auto offered:few)assert(offered==ChestReward::Count);
    }
    run.start(2);
    assert(!run.master_sword && run.sword_bonus() == 0);
    assert(run.chests_opened == 0 && run.chest_cost() == 20 && run.chest_rewards_left());
    wallet = 100;
    assert(!run.buy_chest(wallet, reward) && wallet == 100); // No purchase outside the reward phase.
    // Exact pricing, burn ranks, timed Dark Link and capped multihit grants.
    Run effects; effects.start(7);
    for(unsigned n=0;n<5;++n){effects.chests_opened=n;assert((effects.chest_cost()==std::array<unsigned,5>{20,25,32,41,52}[n]));}
    for(unsigned rank=1;rank<=5;++rank){effects.ranks[unsigned(Upgrade::FireArrows)]=rank;
        assert(effects.burn_damage()==rank&&effects.burn_duration()==(rank<4?90u:120u));}
    effects.dark_link=1;
    for(unsigned i=0;i<599;++i)effects.tick(true);
    assert(!effects.dark_active()&&effects.damage_multiplier()==1);
    effects.tick(true);assert(effects.dark_active()&&effects.dark_remaining==150&&effects.damage_multiplier()==2);
    for(auto phase:{Phase::Upgrade,Phase::TreasureChoice,Phase::Treasure}){
        effects.phase=phase;for(int i=0;i<100;++i)effects.tick(true);
        assert(effects.dark_remaining==150&&effects.dark_clock==0);
    }
    effects.phase=Phase::Running;effects.tick(false);assert(effects.dark_remaining==150);
    for(unsigned i=0;i<150;++i)effects.tick(true);
    assert(!effects.dark_active());
    for(unsigned rank=2;rank<=11;++rank){effects.dark_link=rank;
        assert(effects.dark_interval()==(20-2*(rank/2))*30);
        assert(effects.dark_duration()==(5+(rank-1)/2)*30);
        assert(effects.chest_eligible(ChestReward::DarkLink)==(rank<11));}
    assert(effects.dark_permanent()&&effects.dark_active()&&effects.damage_multiplier()==2);
    effects.dark_link=9;effects.phase=Phase::Treasure;effects.chests_opened=0;unsigned money=100;
    assert(effects.buy_chest(money,ChestReward::DarkLink,false,Rarity::Legendary));
    assert(effects.dark_link==11&&effects.dark_permanent()&&money==140);
    effects.dual_wield=0;effects.chests_opened=0;
    assert(effects.buy_chest(money,ChestReward::OrdonSword,false,Rarity::Common)&&effects.melee_targets()==2);
    assert(effects.buy_chest(money,ChestReward::OrdonSword,false,Rarity::Legendary)&&effects.melee_targets()==4);
    assert(effects.chest_eligible(ChestReward::OrdonSword));
    effects.dual_wield=10;assert(!effects.chest_eligible(ChestReward::OrdonSword));
    effects.start(7);assert(!effects.dark_active()&&!effects.dark_clock&&!effects.dual_wield&&effects.damage_multiplier()==1);
    // Free elite chests grant exactly one item with no currency or price change.
    Run freeRun; freeRun.start(9); freeRun.phase=Phase::Treasure;
    freeRun.chests_opened=7; unsigned zero=0;
    assert(freeRun.buy_chest(zero,ChestReward::Bow,0,Rarity::Common,true));
    assert(zero==0 && freeRun.chests_opened==7 && freeRun.rank(Upgrade::Bow)==1);
    assert(duration==20*60*30 && elite_interval==5*60*30 && timed_elite_count==3);
    // Every ordinary upgrade and weapon can cross the old cap safely.
    for (unsigned rank=0;rank<=30;++rank) {
        Run high;high.start(1);high.shield=1;high.red_potion=true;high.ranks.fill(rank);
        for (unsigned u=0;u<unsigned(Upgrade::Rupees);++u) {
            if (Upgrade(u)==Upgrade::Luck) continue;
            assert(upgrade_cap(Upgrade(u))==30);
            if (rank) assert(high.eligible(Upgrade(u))==(rank<30));
        }
        const auto interval=high.item_cooldown(Upgrade::Bow);
        assert(interval>=6 && interval<=50);
        assert(high.shield_cooldown()>=6 && high.shield_cooldown()<=100);
        if(rank){Run previous=high;previous.ranks.fill(rank-1);
            assert(high.cooldown(100)<=previous.cooldown(100));
            assert(interval<=previous.item_cooldown(Upgrade::Bow));}
    }
    Run heart;heart.start(2);heart.ranks[3]=heart.ranks[4]=heart.ranks[5]=heart.ranks[6]=30;
    assert(heart.eligible(Upgrade::Heart));heart.maximum_hearts=19;
    assert(heart.upgrade_gain(Upgrade::Heart,Rarity::Legendary)==1);
    assert(heart.upgrade_rupees(Upgrade::Heart,Rarity::Legendary)==80);
    heart.phase=Phase::Upgrade;heart.offers[0]=Upgrade::Heart;heart.offer_rarities[0]=Rarity::Legendary;
    assert(heart.choose(0)&&heart.maximum_hearts==20&&!heart.eligible(Upgrade::Heart));
    for(unsigned horde=1;horde<=5;++horde){
        Run swarm;RunOptions settings;settings.horde=horde;swarm.start(3,settings);
        assert(swarm.target_population()==6*horde);
        for(unsigned kill=1;kill<=50;++kill)assert(swarm.normal_kill_drops()==(kill%horde==0));
        swarm.ticks=duration;assert(swarm.target_population()==23*horde&&swarm.target_population()+elite_capacity<enemy_capacity);
    }
    RunOptions invalid{0,100,100,0,1000};invalid.clamp();
    assert(invalid.enemy_hp==1&&invalid.enemy_damage==10&&invalid.enemy_speed==3&&invalid.horde==1&&invalid.camera_distance==500);
    assert(RunOptions{}.camera_distance==200);
    // Equipment slots never cap passives; the offhand replaces within a full build.
    Run slots;slots.start(51);slots.ranks.fill(1);slots.ranks[unsigned(Upgrade::Ball)]=0;
    slots.dual_wield=8;assert(slots.equipment_count()==4);
    assert(slots.eligible(Upgrade::Heart)&&slots.eligible(Upgrade::Magnet));
    assert(!slots.chest_eligible(ChestReward::Ball)&&!slots.chest_eligible(ChestReward::MasterSword));
    assert(slots.chest_eligible(ChestReward::Bow)&&!slots.chest_eligible(ChestReward::RedPotion,true));
    wallet=10000;
    for(auto reward:{ChestReward::Shield,ChestReward::OrdonSword,ChestReward::Shield}) {
        assert(slots.chest_eligible(reward)&&slots.replaces_offhand(reward));
        slots.phase=Phase::Treasure;assert(slots.buy_chest(wallet,reward,0,Rarity::Legendary));
        assert(slots.owns(reward)&&slots.equipment_count()==4&&slots.shield==0&&slots.dual_wield==0);
        assert(slots.owns(ChestReward::Shield)!=slots.owns(ChestReward::OrdonSword));
        assert(slots.buy_chest(wallet,reward)&&slots.equipment_count()==4);
        assert((reward==ChestReward::Shield?slots.shield:slots.dual_wield)==1);
    }
    for(unsigned rank=1;rank<=30;++rank){
        slots.ranks[unsigned(Upgrade::Bow)]=slots.ranks[unsigned(Upgrade::Boomerang)]=rank;
        assert(slots.bow_shots()==1+(rank-1)/3&&slots.boomerang_hops()==1+(rank-1)/3);
    }
    for(unsigned rank=1;rank<=10;++rank){slots.dual_wield=rank;assert(slots.melee_targets()==std::min(5u,2+rank/3));}
    slots.shield=0;slots.offhand=ChestReward::OrdonSword;
    assert(!slots.chest_eligible(ChestReward::OrdonSword)&&slots.chest_eligible(ChestReward::Shield));
    // Shared reroll budget, increasing paid costs and reservation of the purchase price.
    Run actions;actions.start(7);actions.phase=Phase::Upgrade;wallet=250;
    assert(actions.pay_reroll(wallet)&&wallet==250&&actions.reroll_cost()==50);
    actions.phase=Phase::TreasureChoice;
    assert(actions.pay_reroll(wallet,20)&&wallet==200&&actions.reroll_cost()==100);
    actions.phase=Phase::Upgrade;
    assert(actions.pay_reroll(wallet)&&wallet==100&&actions.reroll_cost()==150);
    assert(!actions.pay_reroll(wallet)&&wallet==100&&actions.paid_rerolls==2);
    // Banish preserves ownership/ranks and excludes a weapon from both sources.
    actions.ranks[unsigned(Upgrade::Bow)]=6;
    assert(actions.banish(Upgrade::Bow)&&actions.rank(Upgrade::Bow)==6&&actions.owns(ChestReward::Bow));
    assert(!actions.eligible(Upgrade::Bow)&&!actions.chest_eligible(ChestReward::Bow));
    assert(!actions.banish(Upgrade::Power));
    for(unsigned i=0;i<100;++i){actions.roll_upgrades();for(auto u:actions.offers)assert(u!=Upgrade::Bow);}
    actions.start(7);assert(actions.free_rerolls==1&&actions.banishes==1&&actions.reroll_cost()==0);
    actions.phase=Phase::TreasureChoice;actions.shield=6;
    assert(actions.banish(ChestReward::Shield,true)&&actions.shield==6&&actions.owns(ChestReward::Shield));
    assert(!actions.chest_eligible(ChestReward::Shield));
    actions.banned_upgrades.fill(true);actions.roll_upgrades();
    for(auto u:actions.offers)assert(u==Upgrade::Count);
    Records records;
    records.finish(run); assert(records.runs == 0); // Live runs cannot become records.
    run.end(false); assert(run.phase == Phase::Defeat && !run.tick(true));
    records.finish(run); assert(records.runs == 1 && records.wins == 0);
    run.end(true); assert(run.phase == Phase::Defeat); // A late kill cannot change a loss.
    run.start(42); run.ticks = duration + 100; run.kills = 123; run.level = 24; run.end(true);
    records.finish(run);
    assert(records.wins == 1 && records.runs == 2 && records.best_kills == 123);
    assert(records.best_ticks == duration + 100 && records.best_level == 24);
    Records loaded;
    auto bytes = records.encode();
    assert(loaded.decode(bytes) && loaded.encode() == bytes);
    auto corrupt = bytes; corrupt[4] = 2;
    assert(!loaded.decode(corrupt) && loaded.encode() == bytes);
    assert(!loaded.decode(std::span(bytes.data(), bytes.size() - 1)));
    corrupt = bytes; corrupt[12] = 250; assert(!loaded.decode(corrupt));
    run.start(9); assert(run.xp == 0 && run.kills == 0 && run.ticks == 0 && run.level == 1);
    assert(run.ranks[0] == 0 && run.ranks[1] == 0 && run.ranks[7] == 0);
    std::cout << "Survivors rules: timing, choices, caps, restart and records passed\n";
}

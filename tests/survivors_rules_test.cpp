#include "../src/survivors_rules.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
using namespace dawnlight::survivors;
int main() {
    // Exact base chances and multiplicative Luck; no additive percentage points.
    for(unsigned luck:{0u,1u,5u}) {
        Run sample;sample.start(98765);sample.ranks[unsigned(Upgrade::Luck)]=luck;
        assert(sample.luck_percent()==100+10*luck);
        assert(sample.lucky_chance(2000)==2000+200*luck);
        constexpr unsigned trials=200000;
        std::array<unsigned,5> rarities{};
        std::array<unsigned,unsigned(Run::Drop::Count)> loot{};
        for(unsigned n=0;n<trials;++n){++rarities[unsigned(sample.roll_rarity())];++loot[unsigned(sample.roll_drop())];}
        const std::array<unsigned,5> expected{10000-sample.lucky_chance(3600),sample.lucky_chance(2000),sample.lucky_chance(1000),sample.lucky_chance(500),sample.lucky_chance(100)};
        for(unsigned i=0;i<5;++i)assert(std::abs(int(rarities[i]*10000/trials)-int(expected[i]))<40);
        assert(std::abs(int(loot[1]*10000/trials)-int(sample.lucky_chance(500)))<30);
        assert(std::abs(int(loot[0]*10000/trials)-int(10000-sample.lucky_chance(5000)))<40);
        const unsigned rupees=trials-loot[0]-loot[1];
        const unsigned valuable=rupees-loot[unsigned(Run::Drop::GreenRupee)];
        assert(std::abs(int(valuable*10000/rupees)-int(sample.lucky_chance(1700)))<80);
        for(unsigned i=1;i<loot.size();++i)assert(loot[i]); // Every native color is reachable.
    }
    for(unsigned rarity=0;rarity<5;++rarity) {
        for(unsigned u=0;u<unsigned(Upgrade::Rupees);++u)for(unsigned initial:{0u,4u}) {
            Run r;r.start(13);r.shield=1;r.ranks[8]=1;r.ranks[u]=initial;
            // Weapons must be chest-unlocked first.
            if(u>=unsigned(Upgrade::Bow)&&u<=unsigned(Upgrade::Ball)&&!initial)continue;
            r.phase=Phase::Upgrade;r.offers[0]=Upgrade(u);r.offer_rarities[0]=Rarity(rarity);
            const auto random=r.random;const unsigned gain=std::min(5-initial,rarity+1);
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
            assert(r.master_sword<=5&&r.shield<=10&&r.rank(Upgrade::Bow)<=5);
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
        run.ranks.fill(5); run.shield=1; run.ranks[2] = 4; run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::ShieldAttack || offer == Upgrade::Rupees);
        assert(std::count(run.offers.begin(), run.offers.end(), Upgrade::ShieldAttack) == 1);
        run.start(seed); run.ranks.fill(5); run.xp = 100;
        assert(run.level_up());
        for (const auto offer : run.offers) assert(offer == Upgrade::Rupees);
        assert(run.choose(2));
        assert(run.cooldown(40) == 20);
    }
    run.start(1);
    run.ranks[3] = run.ranks[4] = run.ranks[5] = run.ranks[6] = 1;
    assert(!run.eligible(Upgrade::Heart)); // Four passive slots, existing ranks can improve.
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
    assert(run.buy_chest(wallet, reward) && wallet == 0 && run.chest_cost() == 40);
    assert((reward == ChestReward::OrdonSword && run.dual_wield) || (reward == ChestReward::Shield && run.shield==1) ||
           (reward == ChestReward::MasterSword && run.master_sword) ||
           (run.rank(chest_weapon(reward)) == 1 && run.eligible(chest_weapon(reward)))); // Even with four occupied passives.
    assert(!run.tick(true)); // Get Item cannot advance encounter time.
    run.start(20); run.phase = Phase::Treasure;
    wallet = 100000;
    for (unsigned n = 0; n < 36; ++n) {
        assert(run.chest_cost() == 20 * (n + 1));
        reward = run.roll_chest_offers()[0];
        assert(run.buy_chest(wallet, reward) && run.rank(chest_weapon(reward)) <= 5);
    }
    assert(wallet == 86680 && !run.chest_rewards_left());
    assert(run.master_sword==5 && run.sword_bonus() == 7 && run.dual_wield && run.shield==10);
    assert(run.chest_rewards_left(true)); // A refill remains available after all weapons.
    const auto maxedRng = run.random;
    assert(!run.buy_chest(wallet, reward) && wallet == 86680 && run.random == maxedRng);
    for (unsigned i = unsigned(Upgrade::Bow); i <= unsigned(Upgrade::Ball); ++i) {
        const auto weapon = static_cast<Upgrade>(i);
        assert(run.rank(weapon) == 5 && !run.eligible(weapon));
        const auto fast = run.item_cooldown(weapon);
        run.ranks[i] = 1;
        assert(run.item_cooldown(weapon) > fast);
        run.ranks[i] = 5;
    }
    wallet = 10000;
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
        choices.ranks.fill(5);choices.master_sword=5;choices.dual_wield=true;choices.shield=9;
        auto few=choices.roll_chest_offers(true);
        assert(few[0]!=few[1]&&few[2]==ChestReward::Count);
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

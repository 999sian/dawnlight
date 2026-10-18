#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include "survivors_progression.hpp"

namespace dawnlight::survivors {
// All durations are simulation ticks (TP runs its gameplay at 30 Hz).
inline constexpr unsigned ticks_per_second = 30;
inline constexpr unsigned duration = 20 * 60 * ticks_per_second;
inline constexpr unsigned elite_interval = 5 * 60 * ticks_per_second;
inline constexpr unsigned timed_elite_count = duration / elite_interval - 1;
inline constexpr unsigned guard_capacity = 8;
inline constexpr unsigned elite_capacity = timed_elite_count + guard_capacity;
inline constexpr unsigned guard_respawn_interval = 3 * 60 * ticks_per_second;
// Even instant kills cannot produce more drops before the 20-minute run ends.
inline constexpr unsigned elite_reward_capacity = timed_elite_count +
    guard_capacity * (1 + (duration - 1) / guard_respawn_interval);
inline constexpr unsigned enemy_capacity = 24 * 5 + elite_capacity;
inline constexpr unsigned projectile_capacity = 40;
inline constexpr unsigned item_projectile_begin(unsigned type) { return type ? 28 + (type - 1) * 4 : 8; }
inline constexpr unsigned item_projectile_count(unsigned type) { return type ? 4 : 20; }
inline constexpr unsigned orb_capacity = 64;
inline constexpr unsigned pot_capacity = 72;
inline unsigned pot_budget(unsigned map, float floorArea) {
    // Density follows the loaded floor area, bounded per arena for native actor
    // budgets. Outdoor maps always have substantially more pots than indoors.
    constexpr std::array<unsigned,5> minimum{6,40,40,12,8}, maximum{10,72,72,24,12};
    if (map>=minimum.size()) return 0;
    const float density=floorArea>0 ? floorArea/2000000.f : 0.f;
    return unsigned(std::clamp(density,float(minimum[map]),float(maximum[map])));
}
enum class Phase { Loading, Ready, Running, Upgrade, TreasureChoice, Treasure, Victory, Defeat };
enum class Upgrade : unsigned { Wave, Spin, ShieldAttack, Power, Haste, Reach, Magnet, Heart, Bow, Boomerang, Bombs, Ball, FireArrows, Luck, PotionHealing, CritChance, CritDamage, JumpStrike, Rupees, Count };
inline constexpr unsigned upgrade_cap(Upgrade u) { return u == Upgrade::Luck ? 20 : 30; }
enum class Rarity : unsigned { Common, Uncommon, Rare, Epic, Legendary };
inline constexpr unsigned reward_units(Rarity rarity) { return 1 + unsigned(rarity); }
inline constexpr std::array rarity_names{"Common", "Uncommon", "Rare", "Epic", "Legendary"};
inline constexpr std::array rarity_colors{"#f4d78b", "#7bdd83", "#72b7ff", "#cb91ff", "#ffad54"};
enum class ChestReward : unsigned { Bow, Boomerang, Bombs, Ball, MasterSword, RedPotion, OrdonSword, Shield, DarkLink, Count };
inline constexpr Upgrade chest_weapon(ChestReward reward) {
    return reward <= ChestReward::Ball ? static_cast<Upgrade>(unsigned(Upgrade::Bow) + unsigned(reward)) : Upgrade::Count;
}
inline constexpr std::array names{
    "Sword Wave", "Whirling Blade", "Shield Attack", "Power", "Haste", "Reach", "Magnet", "Heart Container", "Hero's Bow", "Boomerang", "Bombs", "Ball and Chain", "Fire Arrows", "Luck", "Potion Healing", "Crit Chance", "Crit Damage", "Jump Strike", "Rupees"};
inline constexpr std::array descriptions{
    "Automatic piercing sword waves. Higher levels add damage and extra waves.",
    "Unlock or improve a moving spin attack.",
    "Unlock or improve an automatic shield bash that staggers enemies.",
    "Increase automatic weapon damage.", "Reduce automatic weapon cooldowns.",
    "Increase attack size, including Spin and Jump Strike splash radii.", "Attract experience, rupees, hearts and heart pieces from farther away.",
    "Gain one maximum heart (does not heal).",
    "Faster, stronger arrows. Every three upgrades adds an arrow, up to ten.",
    "A damaging returning boomerang. Upgrades increase damage, size and firing rate.",
    "Lob bombs at enemies. Upgrades increase blast size, damage and firing rate.",
    "Launch a heavy ball and retract its chain. Upgrades increase damage, size and firing rate.",
    "Ignite enemies for periodic damage. Hits refresh the burn; upgrades improve damage and duration.",
    "Improve reward rarities and critical chance. +10% Luck per level, max +200%. Heart and rupee bonuses stop at +50%.",
    "Red potions restore an extra quarter heart per level.",
    "Direct hits gain +1% base critical chance per level. Luck multiplies this chance.",
    "Critical hits start at x2 damage. Each level adds +0.05 to the multiplier.",
    "Master Sword: occasionally perform a quick Jump Strike. Landing splash starts at 30% melee damage; +5 percentage points per rank.",
    "Receive 20 rupees."};

struct RunOptions {
    unsigned enemy_hp = 1, enemy_damage = 1, enemy_speed = 1, horde = 1;
    unsigned camera_distance = 200;
    unsigned score_percent() const {
        return 100+10*(std::clamp(enemy_hp,1u,10u)-1)+15*(std::clamp(enemy_damage,1u,10u)-1)+
            25*(std::clamp(enemy_speed,1u,3u)-1)+10*(std::clamp(horde,1u,5u)-1);
    }
    void clamp() {
        enemy_hp=std::clamp(enemy_hp,1u,10u); enemy_damage=std::clamp(enemy_damage,1u,10u);
        enemy_speed=std::clamp(enemy_speed,1u,3u); horde=std::clamp(horde,1u,5u);
        camera_distance=std::clamp(camera_distance,100u,500u);
    }
};
struct Run {
    Phase phase = Phase::Loading;
    unsigned ticks = 0, kills = 0, level = 1, xp = 0;
    uint32_t random = 1;
    RunOptions options;
    Talents talents;
    unsigned timed_elites = 0, guard_elites = 0;
    unsigned drop_progress = 0, maximum_hearts = 3;
    unsigned master_sword = 0, shield = 0, dual_wield = 0, dark_link = 0;
    bool red_potion = false;
    ChestReward offhand = ChestReward::Count;
    unsigned free_rerolls = 1, banishes = 1, paid_rerolls = 0;
    std::array<bool,unsigned(Upgrade::Count)> banned_upgrades{};
    std::array<bool,unsigned(ChestReward::Count)> banned_items{};
    unsigned dark_clock = 0, dark_remaining = 0;
    std::array<unsigned, unsigned(Upgrade::Rupees)> ranks{};
    unsigned chests_opened = 0;
    std::array<Upgrade, 3> offers{};
    std::array<Rarity, 3> offer_rarities{};
    uint32_t next_random() {
        random ^= random << 13; random ^= random >> 17; random ^= random << 5;
        return random;
    }
    void start(uint32_t seed, RunOptions settings = {}, Talents perks = {}) {
        *this={}; options=settings; options.clamp(); talents=perks;
        maximum_hearts=talents.starting_hearts();
        free_rerolls+=talents.rank(Talent::Rerolls); banishes+=talents.rank(Talent::Banishes);
        random=seed ? seed : 1; phase=Phase::Running;
    }
    unsigned magnet_range() const { return (150+65*rank(Upgrade::Magnet))*(100+10*talents.rank(Talent::Magnet))/100; }
    unsigned normal_kills() const { return kills-std::min(kills,timed_elites+guard_elites); }
    unsigned score() const {
        const uint64_t base=2*(std::min(ticks,duration)/ticks_per_second)+uint64_t(normal_kills())*10/std::clamp(options.horde,1u,5u)+
            uint64_t(timed_elites)*500+uint64_t(guard_elites)*250+uint64_t(level ? level-1 : 0)*50+
            (phase==Phase::Victory ? 3000 : 0);
        return unsigned(std::min<uint64_t>(UINT32_MAX,base*options.score_percent()/100));
    }
    unsigned required_xp() const { return 5 + level * 3; }
    unsigned rank(Upgrade u) const { const auto i = static_cast<unsigned>(u); return i < ranks.size() ? ranks[i] : 0; }
    unsigned potion_heal_quarters() const { return 4 + rank(Upgrade::PotionHealing); }
    unsigned luck_percent() const { return 100 + 10 * std::min(20u, rank(Upgrade::Luck)); }
    unsigned crit_chance_basis_points() const { return std::min(10000u, (std::min(30u,rank(Upgrade::CritChance))+talents.rank(Talent::CritChance))*luck_percent()); }
    unsigned crit_damage_percent() const { return 200 + 5*std::min(30u,rank(Upgrade::CritDamage)); }
    unsigned critical_power(unsigned base) {
        const auto chance=crit_chance_basis_points();
        if (!base || !chance) return base;
        // Rejection sampling avoids modulo bias. Zero chance consumes no RNG.
        constexpr uint32_t limit=UINT32_MAX-UINT32_MAX%10000;
        uint32_t roll;do {roll=next_random()-1;} while(roll>=limit);
        if (roll%10000>=chance) return base;
        // Native health/damage are integral; round once, before Dark Link.
        return unsigned(std::min<uint64_t>(65535,(uint64_t(base)*crit_damage_percent()+50)/100));
    }
    // Drop frequency/quality retain their old +50% cap independently of rarities.
    unsigned lucky_chance(unsigned basis_points) const { return basis_points * std::min(150u, luck_percent()) / 100; }
    std::array<uint32_t, 5> rarity_weights() const {
        // 60 / 25k / 10k^2 / 4k^3 / k^4, scaled by 10,000 for exact integers.
        const uint32_t t = luck_percent() / 10; // k = t / 10; t is in [10, 30].
        return {600000, 25000*t, 1000*t*t, 40*t*t*t, t*t*t*t};
    }
    Rarity roll_rarity() {
        const auto weights = rarity_weights();
        uint32_t total = 0;
        for (auto weight : weights) total += weight;
        // Xorshift visits 1..UINT32_MAX. Reject the short tail before reducing.
        const uint32_t limit = UINT32_MAX - UINT32_MAX % total;
        uint32_t roll;
        do { roll = next_random() - 1; } while (roll >= limit);
        roll %= total;
        for (unsigned i = 0; i < weights.size(); ++i) {
            if (roll < weights[i]) return Rarity(i);
            roll -= weights[i];
        }
        return Rarity::Common;
    }
    unsigned upgrade_gain(Upgrade u, Rarity rarity) const {
        unsigned remaining = u < Upgrade::Rupees ? upgrade_cap(u) - std::min(upgrade_cap(u), rank(u)) : 0;
        if (u == Upgrade::Heart) remaining = std::min(remaining, 20 - std::min(20u, maximum_hearts));
        return std::min(reward_units(rarity), remaining);
    }
    unsigned upgrade_rupees(Upgrade u, Rarity rarity) const {
        return 20 * (reward_units(rarity) - upgrade_gain(u, rarity));
    }
    bool eligible(Upgrade u) const {
        if (u >= Upgrade::Count || banned_upgrades[unsigned(u)]) return false;
        if (u >= Upgrade::Bow && u <= Upgrade::Ball && banned_items[unsigned(u)-unsigned(Upgrade::Bow)]) return false;
        if (u == Upgrade::Rupees) return true;
        if (u >= Upgrade::Count || rank(u) >= upgrade_cap(u)) return false;
        if (u == Upgrade::ShieldAttack && !owns(ChestReward::Shield)) return false;
        if (u == Upgrade::PotionHealing && !red_potion) return false;
        if (u == Upgrade::JumpStrike && !master_sword) return false;
        if (u == Upgrade::FireArrows && !rank(Upgrade::Bow)) return false;
        if (u >= Upgrade::Bow && u <= Upgrade::Ball && rank(u) == 0) return false;
        if (u == Upgrade::Heart && maximum_hearts >= 20) return false;
        return true;
    }
    void roll_upgrades() {
        std::array<Upgrade, static_cast<unsigned>(Upgrade::Count)> pool{};
        unsigned count = 0;
        for (unsigned i = 0; i < static_cast<unsigned>(Upgrade::Count); ++i)
            if (eligible(static_cast<Upgrade>(i))) pool[count++] = static_cast<Upgrade>(i);
        for (auto& offer : offers) {
            if (!count) { offer = eligible(Upgrade::Rupees) ? Upgrade::Rupees : Upgrade::Count; continue; }
            const unsigned choice = next_random() % count;
            offer = pool[choice]; pool[choice] = pool[--count];
        }
        for (auto& rarity : offer_rarities) rarity = roll_rarity();
    }
    bool level_up() {
        if (phase != Phase::Running || xp < required_xp()) return false;
        xp -= required_xp(); ++level; roll_upgrades(); phase = Phase::Upgrade; return true;
    }
    bool choose(unsigned choice) {
        if (phase != Phase::Upgrade || choice >= offers.size()) return false;
        const auto u = offers[choice];
        if (!eligible(u)) return false;
        const unsigned gain = upgrade_gain(u, offer_rarities[choice]);
        if (u != Upgrade::Rupees) ranks[static_cast<unsigned>(u)] += gain;
        if (u == Upgrade::Heart) maximum_hearts += gain;
        phase = Phase::Running;
        return true;
    }
    unsigned chest_cost() const { return 20 + chests_opened * (chests_opened + 4); }
    unsigned burn_damage() const { return rank(Upgrade::FireArrows); }
    unsigned burn_duration() const { return (3 + (std::max(1u, rank(Upgrade::FireArrows)) - 1) / 3) * ticks_per_second; }
    unsigned melee_power() const { return 3 + rank(Upgrade::Power) + sword_bonus(); }
    unsigned jump_splash_percent() const {
        const auto level = std::min(30u, rank(Upgrade::JumpStrike));
        return level ? 30 + 5 * (level - 1) : 0;
    }
    unsigned jump_splash_power() const {
        return jump_splash_percent() ? std::max(1u, (melee_power() * jump_splash_percent() + 50) / 100) : 0;
    }
    unsigned jump_cooldown() const { return cooldown(6 * ticks_per_second); }
    bool roll_jump_strike() { return master_sword && rank(Upgrade::JumpStrike) && next_random() % 100 < 20; }
    unsigned melee_targets() const { return dual_wield ? std::min(5u, 2 + dual_wield / 3) : 1; }
    unsigned bow_shots() const { return rank(Upgrade::Bow) ? std::min(10u,1+(rank(Upgrade::Bow)-1)/3) : 0; }
    unsigned boomerang_hops() const { return rank(Upgrade::Boomerang) ? std::min(10u,1+(rank(Upgrade::Boomerang)-1)/3) : 0; }
    bool owns(ChestReward reward) const {
        if (reward<=ChestReward::Ball) return rank(chest_weapon(reward))!=0;
        if (reward==ChestReward::MasterSword) return master_sword!=0;
        if (reward==ChestReward::DarkLink) return dark_link!=0;
        if (reward==ChestReward::RedPotion) return red_potion;
        if (reward==ChestReward::OrdonSword) return dual_wield || offhand==reward;
        if (reward==ChestReward::Shield) return shield || offhand==reward;
        return false;
    }
    unsigned equipment_count() const {
        unsigned count=0;for(unsigned i=0;i<unsigned(ChestReward::Count);++i) count+=owns(ChestReward(i));
        return count;
    }
    bool replaces_offhand(ChestReward reward) const {
        return (reward==ChestReward::Shield && owns(ChestReward::OrdonSword)) ||
               (reward==ChestReward::OrdonSword && owns(ChestReward::Shield));
    }
    unsigned reroll_cost() const { return free_rerolls ? 0 : 50*(paid_rerolls+1); }
    bool pay_reroll(unsigned& rupees, unsigned reserved=0) {
        if ((phase!=Phase::Upgrade && phase!=Phase::TreasureChoice) || rupees<reserved || rupees-reserved<reroll_cost()) return false;
        rupees-=reroll_cost(); if(free_rerolls)--free_rerolls;else ++paid_rerolls;return true;
    }
    bool banish(Upgrade u) {
        if (!banishes || phase!=Phase::Upgrade || u>=Upgrade::Count || !eligible(u)) return false;
        --banishes; banned_upgrades[unsigned(u)]=true;
        if(u>=Upgrade::Bow && u<=Upgrade::Ball)banned_items[unsigned(u)-unsigned(Upgrade::Bow)]=true;
        return true;
    }
    bool banish(ChestReward reward, bool bottles) {
        if (!banishes || phase!=Phase::TreasureChoice || !chest_eligible(reward,bottles)) return false;
        --banishes;banned_items[unsigned(reward)]=true;
        const auto weapon=chest_weapon(reward);if(weapon!=Upgrade::Count)banned_upgrades[unsigned(weapon)]=true;
        return true;
    }
    static constexpr unsigned dark_max_rank = 11;
    unsigned dark_interval() const { return (20 - 2 * (std::min(dark_max_rank, dark_link) / 2)) * ticks_per_second; }
    unsigned dark_duration() const { return (5 + (std::max(1u, dark_link) - 1) / 2) * ticks_per_second; }
    bool dark_permanent() const { return dark_link >= dark_max_rank; }
    bool dark_active() const { return dark_link && (dark_permanent() || dark_remaining); }
    unsigned damage_multiplier() const { return dark_active() ? 2 : 1; }
    bool chest_eligible(ChestReward reward, bool bottle_available = false) const {
        if (reward>=ChestReward::Count || banned_items[unsigned(reward)]) return false;
        if (!owns(reward) && !replaces_offhand(reward) && equipment_count()>=4) return false;
        if (reward <= ChestReward::Ball) return rank(chest_weapon(reward)) < upgrade_cap(chest_weapon(reward));
        if (reward == ChestReward::MasterSword) return master_sword < 5;
        if (reward == ChestReward::OrdonSword) return dual_wield < 10;
        if (reward == ChestReward::DarkLink) return !dark_permanent();
        if (reward == ChestReward::Shield) return shield < 10;
        return reward == ChestReward::RedPotion && bottle_available;
    }
    bool chest_rewards_left(bool bottle_available = false) const {
        for (unsigned i = 0; i < unsigned(ChestReward::Count); ++i)
            if (chest_eligible(ChestReward(i), bottle_available)) return true;
        return false;
    }
    std::array<ChestReward, 3> roll_chest_offers(bool bottle_available = false) {
        std::array<ChestReward, unsigned(ChestReward::Count)> pool{};
        unsigned count = 0;
        for (unsigned i = 0; i < pool.size(); ++i)
            if (chest_eligible(ChestReward(i), bottle_available)) pool[count++] = ChestReward(i);
        std::array<ChestReward, 3> result;
        result.fill(ChestReward::Count);
        for (auto& reward : result) {
            if (!count) break;
            const unsigned choice = next_random() % count;
            reward = pool[choice]; pool[choice] = pool[--count];
        }
        return result;
    }
    unsigned chest_gain(ChestReward reward, Rarity rarity, unsigned bottle_capacity = 0) const {
        if (replaces_offhand(reward)) return 0; // A replacement starts at rank zero.
        unsigned remaining = 0;
        if (reward <= ChestReward::Ball) remaining = upgrade_cap(chest_weapon(reward)) - std::min(upgrade_cap(chest_weapon(reward)), rank(chest_weapon(reward)));
        else if (reward == ChestReward::MasterSword) remaining = 5 - std::min(5u, master_sword);
        else if (reward == ChestReward::Shield) remaining = 10 - std::min(10u, shield);
        else if (reward == ChestReward::OrdonSword) remaining = 10 - std::min(10u, dual_wield);
        else if (reward == ChestReward::DarkLink) remaining = dark_max_rank - std::min(dark_max_rank, dark_link);
        else if (reward == ChestReward::RedPotion) remaining = bottle_capacity;
        return std::min(reward_units(rarity), remaining);
    }
    unsigned chest_rupees(ChestReward reward, Rarity rarity, unsigned bottle_capacity = 0) const {
        return 20 * (reward_units(rarity) - chest_gain(reward, rarity, bottle_capacity));
    }
    // Apply exactly the selected reward. No reroll, payment or rank change on
    // an invalid/stale selection; presentation commits this preview only once.
    bool buy_chest(unsigned& rupees, ChestReward reward, unsigned bottle_capacity = 0, Rarity rarity = Rarity::Common, bool free = false) {
        const unsigned price = free ? 0 : chest_cost();
        if (phase != Phase::Treasure || rupees < price || !chest_eligible(reward, bottle_capacity != 0)) return false;
        const unsigned gain = chest_gain(reward, rarity, bottle_capacity);
        rupees = rupees - price + chest_rupees(reward, rarity, bottle_capacity);
        if (!free) ++chests_opened;
        if (replaces_offhand(reward)) { shield=dual_wield=0; offhand=reward; }
        if (reward == ChestReward::Shield || reward == ChestReward::OrdonSword) offhand=reward;
        if (reward == ChestReward::RedPotion) red_potion = true;
        if (reward == ChestReward::MasterSword) master_sword += gain;
        else if (reward == ChestReward::OrdonSword) dual_wield += gain;
        else if (reward == ChestReward::DarkLink) dark_link += gain;
        else if (reward == ChestReward::Shield) shield += gain;
        else if (reward <= ChestReward::Ball) ranks[unsigned(chest_weapon(reward))] += gain;
        return true;
    }
    unsigned item_cooldown(Upgrade weapon) const {
        const unsigned base = weapon == Upgrade::Bow ? 50 : weapon == Upgrade::Boomerang ? 85 :
            weapon == Upgrade::Bombs ? 110 : 100;
        return cooldown(base - std::min(base - 6, 5 * (std::max(1u, rank(weapon)) - 1)));
    }
    bool tick(bool playable) {
        if (phase != Phase::Running || !playable) return false;
        ++ticks;
        if (dark_remaining) --dark_remaining;
        if (dark_link && !dark_permanent() && ++dark_clock >= dark_interval()) {
            dark_clock = 0; dark_remaining = dark_duration();
        }
        return true;
    }
    unsigned target_population() const { return std::min(6u + ticks / (45 * ticks_per_second), 23u) * options.horde; }
    unsigned spawn_interval() const { return std::max(15u, 75u - std::min(60u, ticks / (10 * ticks_per_second))); }
    unsigned cooldown_percent() const {
        const auto rank = std::min(30u, this->rank(Upgrade::Haste));
        return rank <= 5 ? 100 - rank * 10 : 1250 / (25 + 3 * (rank - 5));
    }
    unsigned cooldown(unsigned base) const { return std::max(6u, base * cooldown_percent() / 100); }
    unsigned shield_cooldown() const { return cooldown(100 - std::min(90u, 10 * rank(Upgrade::ShieldAttack))); }
    bool normal_kill_drops() { if (++drop_progress < options.horde) return false; drop_progress=0; return true; }
    // At base Luck: 5% heart, 45% rupee, 50% nothing. A second lucky
    // roll chooses rupee value, so Luck improves both frequency and quality.
    enum class Drop { None, Heart, GreenRupee, BlueRupee, YellowRupee, RedRupee, PurpleRupee, OrangeRupee, SilverRupee, Count };
    Drop roll_drop() {
        const unsigned roll = next_random() % 10000;
        if (roll < lucky_chance(500)) return Drop::Heart;
        if (roll >= lucky_chance(5000)) return Drop::None;
        const unsigned color = next_random() % 10000;
        unsigned threshold = 0;
        constexpr std::array<unsigned, 6> chances{5, 15, 80, 200, 400, 1000};
        for (unsigned i = 0; i < chances.size(); ++i) {
            threshold += lucky_chance(chances[i]);
            if (color < threshold) return Drop(unsigned(Drop::SilverRupee) - i);
        }
        return Drop::GreenRupee;
    }
    enum class PotDrop { HeartPiece, GreenRupee, BlueRupee, YellowRupee, RedRupee,
                         PurpleRupee, OrangeRupee, SilverRupee, Count };
    PotDrop roll_pot_drop() {
        if (next_random()%10000 < lucky_chance(200)) return PotDrop::HeartPiece;
        const unsigned color=next_random()%10000;
        constexpr std::array<unsigned,6> chances{10,40,150,600,1000,2200};
        unsigned threshold=0;
        for (unsigned i=0;i<chances.size();++i) {
            threshold+=lucky_chance(chances[i]);
            if (color<threshold) return PotDrop(unsigned(PotDrop::SilverRupee)-i);
        }
        return PotDrop::GreenRupee;
    }
    unsigned block_chance() const { return std::min(10u, shield) * 5; }
    unsigned shield_tier() const { return shield >= 7 ? 2 : shield >= 4 ? 1 : 0; }
    unsigned sword_bonus() const { return master_sword ? 2 + master_sword : 0; }
    unsigned attack_power(unsigned weapon) const { return 2 + ranks[weapon] + rank(Upgrade::Power) + (weapon < 2 ? sword_bonus() : 0); }
    float radius(float base) const { return base * (1.0f + 0.15f * rank(Upgrade::Reach)); }
    void end(bool won) { if (phase == Phase::Running || phase == Phase::Upgrade || phase == Phase::TreasureChoice || phase == Phase::Treasure) phase = won ? Phase::Victory : Phase::Defeat; }
};

// Explicit little-endian encoding, independent of ABI padding and game version.
struct Records {
    uint32_t runs = 0, wins = 0, best_ticks = 0, best_kills = 0, best_level = 0;
    std::array<uint32_t,5> high_scores{};
    uint64_t total_xp=0;
    Talents talents;
    unsigned level() const { return 1+meta_levels(total_xp); }
    unsigned available_points() const { return meta_levels(total_xp)-std::min(meta_levels(total_xp),talents.spent()); }
    uint64_t level_xp() const { return total_xp-meta_threshold(level()-1); }
    unsigned next_level_xp() const { return 2000+500*(level()-1); }
    bool buy(Talent t) { if(!available_points() || !talents.can_buy(t))return false;++talents.ranks[unsigned(t)];return true; }
    void finish(const Run& run, unsigned map=0) {
        if (run.phase != Phase::Victory && run.phase != Phase::Defeat) return;
        if(map>=high_scores.size())return;
        high_scores[map]=std::max(high_scores[map],run.score());
        total_xp=std::min(max_meta_xp,total_xp+run.score());
        ++runs; wins += run.phase == Phase::Victory;
        best_ticks = std::max(best_ticks, run.ticks);
        best_kills = std::max(best_kills, run.kills);
        best_level = std::max(best_level, run.level);
    }
    std::array<uint8_t,100> encode() const {
        std::array<uint32_t,25> words{0x56525348,2,runs,wins,best_ticks,best_kills,best_level};
        for(unsigned i=0;i<5;++i)words[7+i]=high_scores[i];
        words[12]=uint32_t(total_xp);words[13]=uint32_t(total_xp>>32);
        for(unsigned i=0;i<talents.ranks.size();++i)words[14+i]=talents.ranks[i];
        std::array<uint8_t,100> bytes{};
        for(unsigned i=0;i<words.size();++i)for(unsigned b=0;b<4;++b)bytes[i*4+b]=uint8_t(words[i]>>(8*b));
        return bytes;
    }
    bool decode(std::span<const uint8_t> bytes) {
        if(bytes.size()!=28 && bytes.size()!=100)return false;
        std::array<uint32_t,25> words{};
        for(unsigned i=0;i<bytes.size()/4;++i)for(unsigned b=0;b<4;++b)words[i]|=uint32_t(bytes[i*4+b])<<(8*b);
        if(words[0]!=0x56525348 || words[3]>words[2])return false;
        if(!((words[1]==1 && bytes.size()==28)||(words[1]==2 && bytes.size()==100)))return false;
        Records next;
        next.runs=words[2];next.wins=words[3];next.best_ticks=words[4];next.best_kills=words[5];next.best_level=words[6];
        if(words[1]==2){
            for(unsigned i=0;i<5;++i)next.high_scores[i]=words[7+i];
            next.total_xp=uint64_t(words[12])|(uint64_t(words[13])<<32);
            for(unsigned i=0;i<next.talents.ranks.size();++i)next.talents.ranks[i]=words[14+i];
            if(next.total_xp>max_meta_xp || !next.talents.valid() || next.talents.spent()>meta_levels(next.total_xp))return false;
        }
        *this=next;return true;
    }
};
} // namespace dawnlight::survivors

#pragma once
#include "survivors_rules.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace dawnlight::survivors {
struct ItemChange { std::string label, before, after; };
inline std::string number(unsigned value) { return std::to_string(value); }
inline std::string seconds(unsigned ticks) {
    char text[24]; std::snprintf(text, sizeof(text), "%.2fs", ticks / float(ticks_per_second)); return text;
}
inline unsigned arrow_count(const Run& run) { return run.rank(Upgrade::Bow) >= 5 ? 3 : run.rank(Upgrade::Bow) >= 3 ? 2 : 1; }
inline unsigned item_damage(const Run& run, Upgrade weapon) {
    return run.attack_power(unsigned(weapon)) + (weapon == Upgrade::Ball ? 3 : 0);
}
inline unsigned item_radius(const Run& run, Upgrade weapon) {
    return unsigned(run.radius(weapon == Upgrade::Bombs ? 170 + 30 * run.rank(weapon) :
        weapon == Upgrade::Bow ? 35 : 55 + 8 * run.rank(weapon)));
}
inline const char* reward_name(ChestReward reward, const Run& run) {
    if (reward <= ChestReward::Ball) return names[unsigned(chest_weapon(reward))];
    if (reward == ChestReward::MasterSword) return run.master_sword >= 5 ? "Light Sword" : "Master Sword";
    if (reward == ChestReward::OrdonSword) return "Ordon Sword";
    if (reward == ChestReward::Shield) return run.shield_tier() == 2 ? "Hylian Shield" : run.shield_tier() == 1 ? "Ordon Shield" : "Wooden Shield";
    return "Red Potion";
}
inline std::vector<ItemChange> reward_changes(ChestReward reward, const Run& before, const Run& after) {
    std::vector<ItemChange> changes;
    const auto add = [&](const char* label, std::string a, std::string b) {
        if (a != b) changes.push_back({label, std::move(a), std::move(b)});
    };
    if (reward <= ChestReward::Ball) {
        const auto weapon = chest_weapon(reward);
        const bool owned = before.rank(weapon) != 0;
        add("Level", number(before.rank(weapon)), number(after.rank(weapon)));
        add("Damage", owned ? number(item_damage(before, weapon)) : "off", number(item_damage(after, weapon)));
        add("Interval", owned ? seconds(before.item_cooldown(weapon)) : "off", seconds(after.item_cooldown(weapon)));
        add(weapon == Upgrade::Bombs ? "Blast radius" : "Hit radius", owned ? number(item_radius(before, weapon)) : "off", number(item_radius(after, weapon)));
        if (weapon == Upgrade::Bow) add("Arrows", owned ? number(arrow_count(before)) : "0", number(arrow_count(after)));
        if (weapon == Upgrade::Boomerang) add("Extra targets", number(before.rank(weapon)), number(after.rank(weapon)));
    } else if (reward == ChestReward::MasterSword) {
        add("Sword bonus", "+" + number(before.sword_bonus()), "+" + number(after.sword_bonus()));
        add("Melee damage", number(3 + before.rank(Upgrade::Power) + before.sword_bonus()), number(3 + after.rank(Upgrade::Power) + after.sword_bonus()));
        for (auto weapon : {Upgrade::Wave, Upgrade::Spin}) if (after.rank(weapon))
            add(weapon == Upgrade::Wave ? "Wave damage" : "Spin damage", number(before.attack_power(unsigned(weapon))), number(after.attack_power(unsigned(weapon))));
        if (after.master_sword == 5) add("Light Sword", "off", "on");
    } else if (reward == ChestReward::OrdonSword) add("Dual Wield", "off", "on");
    else if (reward == ChestReward::Shield) add("Block chance", number(before.block_chance()) + "%", number(after.block_chance()) + "%");
    return changes;
}
// Compact four-line descriptions fit the native explanation pane. Effective
// values include collected Power/Haste/Reach and Fire Arrows upgrades.
inline std::string item_description(ChestReward reward, const Run& run) {
    const std::string bonuses = "Power +" + number(run.rank(Upgrade::Power)) + " | Haste -" + number(run.rank(Upgrade::Haste) * 10) + "% | Reach +" + number(run.rank(Upgrade::Reach) * 15) + "%";
    if (reward <= ChestReward::Ball) {
        const auto weapon = chest_weapon(reward);
        const std::string stats = "Lv." + number(run.rank(weapon)) + " | Damage " + number(item_damage(run, weapon)) + " | Interval " + seconds(run.item_cooldown(weapon));
        if (weapon == Upgrade::Bow) return "Auto-aim; no ammo. " + number(arrow_count(run)) + " arrows/shot.\n" + stats + "\nFire Lv." + number(run.rank(Upgrade::FireArrows)) + " (+" + number(run.rank(Upgrade::FireArrows) * 2) + " dmg); radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
        if (weapon == Upgrade::Boomerang) return "Auto Gale throw; hits out and back.\n" + stats + "\n" + number(run.rank(weapon)) + " extra targets; radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
        if (weapon == Upgrade::Bombs) return "Auto-aim throw; area explosion, no ammo.\n" + stats + "\nBlast radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
        return "Auto throw; pierces out/back, staggers.\n" + stats + "\nRange " + number(unsigned(run.radius(900))) + "; radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
    }
    if (reward == ChestReward::Shield) {
        const unsigned rank = run.rank(Upgrade::ShieldAttack);
        return "Auto block " + number(run.block_chance()) + "% (max 50%); faces attacker.\nShield Attack Lv." + number(rank) + (rank ? ": staggers, damage " + number(run.attack_power(unsigned(Upgrade::ShieldAttack))) : ": locked") +
            "\n" + (rank ? "Bash interval " + seconds(run.cooldown(100 - 10 * rank)) + "; radius " + number(unsigned(run.radius(60 + 8 * rank))) : "Shield Attack available at level-up.") + "\n" + bonuses;
    }
    if (reward == ChestReward::OrdonSword || reward == ChestReward::MasterSword) {
        const unsigned waves = run.rank(Upgrade::Wave) >= 5 ? 3 : run.rank(Upgrade::Wave) >= 3 ? 2 : 1;
        return (reward == ChestReward::OrdonSword ? std::string("Dual Wield; auto combos") : "Sword Lv." + number(run.master_sword) + "; bonus +" + number(run.sword_bonus())) +
            "; melee " + number(3 + run.rank(Upgrade::Power) + run.sword_bonus()) +
            "\nWave Lv." + number(run.rank(Upgrade::Wave)) + ": " + (run.rank(Upgrade::Wave) ? number(waves) + " piercing, " + number(run.attack_power(0)) + " dmg/" + seconds(run.cooldown(40)) : "locked") +
            "\nSpin Lv." + number(run.rank(Upgrade::Spin)) + ": " + (run.rank(Upgrade::Spin) ? "moving, " + number(run.attack_power(1)) + " dmg/" + seconds(run.cooldown(100)) : "locked") + "\n" + bonuses;
    }
    return "Restores up to 8 hearts when used.\nBottles can be assigned to item buttons.\nChest rewards add bottles up to four,\nthen refill one empty bottle.";
}
inline std::string upgrade_value(Upgrade upgrade, const Run& run) {
    const auto rank = run.rank(upgrade);
    switch (upgrade) {
    case Upgrade::Power: return "+" + number(rank) + " damage";
    case Upgrade::Haste: return "-" + number(10 * rank) + "% cooldown";
    case Upgrade::Reach: return "+" + number(15 * rank) + "% attack size";
    case Upgrade::Magnet: return number(150 + 65 * rank) + " pickup range";
    case Upgrade::Heart: return number(3 + rank) + " maximum hearts";
    case Upgrade::FireArrows: return "+" + number(2 * rank) + " fire damage";
    case Upgrade::Rupees: return "+20 rupees";
    default: return "Level " + number(rank);
    }
}
inline std::string upgrade_description(Upgrade upgrade, const Run& run) {
    if (upgrade >= Upgrade::Count) return {};
    if (upgrade >= Upgrade::Bow && upgrade <= Upgrade::Ball)
        return item_description(ChestReward(unsigned(upgrade) - unsigned(Upgrade::Bow)), run);
    std::string text = descriptions[unsigned(upgrade)];
    if (upgrade == Upgrade::Wave || upgrade == Upgrade::Spin || upgrade == Upgrade::ShieldAttack) {
        text += "\nDamage " + number(run.attack_power(unsigned(upgrade)));
        text += "; interval " + seconds(run.cooldown(upgrade == Upgrade::Wave ? 40 :
            upgrade == Upgrade::Spin ? 100 : 100 - 10 * run.rank(upgrade)));
        if (upgrade == Upgrade::Wave) text += "; waves " + number(run.rank(upgrade) >= 5 ? 3 : run.rank(upgrade) >= 3 ? 2 : 1);
        if (upgrade == Upgrade::ShieldAttack) text += "; radius " + number(unsigned(run.radius(60 + 8 * run.rank(upgrade))));
    } else text += "\n" + upgrade_value(upgrade, run);
    return text;
}
inline std::vector<ItemChange> upgrade_changes(Upgrade upgrade, const Run& before, const Run& after) {
    if (upgrade >= Upgrade::Bow && upgrade <= Upgrade::Ball)
        return reward_changes(ChestReward(unsigned(upgrade) - unsigned(Upgrade::Bow)), before, after);
    if (upgrade == Upgrade::Rupees) return {{"Reward", "0", "+20 rupees"}};
    std::vector<ItemChange> changes{{"Level", number(before.rank(upgrade)), number(after.rank(upgrade))}};
    if (upgrade == Upgrade::Wave || upgrade == Upgrade::Spin || upgrade == Upgrade::ShieldAttack) {
        changes.push_back({"Damage", before.rank(upgrade) ? number(before.attack_power(unsigned(upgrade))) : "off",
                           number(after.attack_power(unsigned(upgrade)))});
        if (upgrade == Upgrade::Wave) {
            const auto count = [](unsigned rank) { return rank == 0 ? 0 : rank >= 5 ? 3 : rank >= 3 ? 2 : 1; };
            if (count(before.rank(upgrade)) != count(after.rank(upgrade)))
                changes.push_back({"Waves", number(count(before.rank(upgrade))), number(count(after.rank(upgrade)))});
        }
        if (upgrade == Upgrade::ShieldAttack) {
            changes.push_back({"Interval", before.rank(upgrade) ? seconds(before.cooldown(100 - 10 * before.rank(upgrade))) : "off",
                               seconds(after.cooldown(100 - 10 * after.rank(upgrade)))});
            changes.push_back({"Radius", before.rank(upgrade) ? number(unsigned(before.radius(60 + 8 * before.rank(upgrade)))) : "off",
                               number(unsigned(after.radius(60 + 8 * after.rank(upgrade))))});
        }
    } else changes.push_back({"Effect", upgrade_value(upgrade, before), upgrade_value(upgrade, after)});
    return changes;
}
} // namespace dawnlight::survivors

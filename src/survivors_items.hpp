#pragma once
#include "survivors_rules.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace dawnlight::survivors {
struct ItemChange { std::string label, before, after; };
inline std::string number(unsigned value) { return std::to_string(value); }
inline std::string decimal(float value) { char text[24];std::snprintf(text,sizeof(text),"%.1f",value);return text; }
inline std::string seconds(unsigned ticks) {
    char text[24]; std::snprintf(text, sizeof(text), "%.2fs", ticks / float(ticks_per_second)); return text;
}
inline std::string potion_healing(const Run& run) {
    char text[32]; std::snprintf(text,sizeof(text),"%.2f hearts",run.potion_heal_quarters()/4.f); return text;
}
inline std::string crit_chance(const Run& run) {
    char text[24];std::snprintf(text,sizeof(text),"%.2f%%",run.crit_chance_basis_points()/100.f);return text;
}
inline std::string crit_multiplier(const Run& run) {
    char text[24];std::snprintf(text,sizeof(text),"x%.2f",run.crit_damage_percent()/100.f);return text;
}
inline std::string dark_multiplier(const Run& run) {
    char text[24];std::snprintf(text,sizeof(text),"x%.2f",run.dark_damage_percent()/100.f);return text;
}
inline unsigned arrow_count(const Run& run) { return run.bow_shots(); }
inline unsigned item_damage(const Run& run, Upgrade weapon) {
    return run.attack_power(unsigned(weapon)) + (weapon == Upgrade::Ball ? 3 : 0);
}
inline unsigned item_radius(const Run& run, Upgrade weapon) {
    if (weapon == Upgrade::Bow) return 35;
    if (weapon == Upgrade::Bombs) return unsigned(run.bomb_radius());
    return unsigned(run.radius(weapon == Upgrade::Boomerang ? 63 : 55 + 8 * run.rank(weapon)));
}
inline const char* reward_name(ChestReward reward, const Run& run) {
    if (reward <= ChestReward::Ball) return names[unsigned(chest_weapon(reward))];
    if (reward == ChestReward::MasterSword) return run.master_sword >= 5 ? "Light Sword" : "Master Sword";
    if (reward == ChestReward::OrdonSword) return "Ordon Sword";
    if (reward == ChestReward::DarkLink) return "Dark Link";
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
        if (weapon == Upgrade::Bow) {
            add("Arrows", owned ? number(arrow_count(before)) : "0", number(arrow_count(after)));
            add("Fan angle", decimal(before.bow_fan_degrees())+" degrees", decimal(after.bow_fan_degrees())+" degrees");
        }
        if (weapon == Upgrade::Boomerang) add("Extra targets", number(before.boomerang_hops()), number(after.boomerang_hops()));
    } else if (reward == ChestReward::MasterSword) {
        add("Sword bonus", "+" + number(before.sword_bonus()), "+" + number(after.sword_bonus()));
        add("Melee damage", number(before.melee_power()), number(after.melee_power()));
        if(after.rank(Upgrade::Lightning))add("Lightning damage",number(before.lightning_power()),number(after.lightning_power()));
        for (auto weapon : {Upgrade::Wave, Upgrade::Spin}) if (after.rank(weapon))
            add(weapon == Upgrade::Wave ? "Wave damage" : "Spin damage", number(before.attack_power(unsigned(weapon))), number(after.attack_power(unsigned(weapon))));
        if (after.rank(Upgrade::JumpStrike))
            add("Jump splash damage", number(before.jump_splash_power()), number(after.jump_splash_power()));
        if (before.master_sword < 5 && after.master_sword >= 5) add("Light Sword", "off", "on");
    } else if (reward == ChestReward::OrdonSword) {
        add("Dual Wield level", number(before.dual_wield), number(after.dual_wield));
        add("Melee targets", number(before.melee_targets()), number(after.melee_targets()));
        add("Melee speed", number(before.melee_speed_percent())+"%", number(after.melee_speed_percent())+"%");
        add("Melee crit bonus", decimal(before.melee_crit_bonus()/100.f)+"%", decimal(after.melee_crit_bonus()/100.f)+"%");
    } else if (reward == ChestReward::DarkLink) {
        add("Level", number(before.dark_link), number(after.dark_link));
        add("Damage multiplier", dark_multiplier(before), dark_multiplier(after));
        add("Activation interval", before.dark_link ? seconds(before.dark_interval()) : "off",
            after.dark_permanent() ? "always active" : seconds(after.dark_interval()));
        add("Duration", before.dark_link ? seconds(before.dark_duration()) : "off",
            after.dark_permanent() ? "infinite" : seconds(after.dark_duration()));
    }
    else if (reward == ChestReward::RedPotion && !before.red_potion && after.red_potion)
        add("Healing", "off", potion_healing(after));
    else if (reward == ChestReward::Shield) add("Block chance", number(before.block_chance()) + "%", number(after.block_chance()) + "%");
    return changes;
}
// Compact four-line descriptions fit the native explanation pane. Effective
// values include collected Power/Haste/Reach and Fire Arrows upgrades.
inline std::string item_description(ChestReward reward, const Run& run) {
    const std::string bonuses = "Power +" + number(run.rank(Upgrade::Power)) + " | Haste -" + number(100 - run.cooldown_percent()) + "% | Reach +" + decimal(run.rank(Upgrade::Reach) * 7.5f) + "%";
    if (reward <= ChestReward::Ball) {
        const auto weapon = chest_weapon(reward);
        const std::string stats = "Lv." + number(run.rank(weapon)) + " | Damage " + number(item_damage(run, weapon)) + " | Interval " + seconds(run.item_cooldown(weapon));
        if (weapon == Upgrade::Bow) return number(arrow_count(run)) + " arrows, " + decimal(run.bow_fan_degrees()) + " deg; " + number(run.arrow_targets()) + " targets; extra hits 25%.\n" + stats + "\nFire Lv." + number(run.rank(Upgrade::FireArrows)) + " (" + number(run.burn_damage()) + "/s, " + seconds(run.burn_duration()) + ")" + "\n" + bonuses;
        if (weapon == Upgrade::Boomerang) return number(run.ranged_shots()) + " auto Gale throws; hits out and back.\n" + stats + "\n" + number(run.boomerang_hops()) + " extra targets; radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
        if (weapon == Upgrade::Bombs) return number(run.ranged_shots()) + " auto-aim bombs; area explosion, no ammo.\n" + stats + "\nBlast radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
        return number(run.ranged_shots()) + " auto balls; pierces out/back, small push.\n" + stats + "\nRange " + number(unsigned(run.radius(900))) + "; radius " + number(item_radius(run, weapon)) + "\n" + bonuses;
    }
    if (reward == ChestReward::Shield) {
        const unsigned rank = run.rank(Upgrade::ShieldAttack);
        return "Auto block " + number(run.block_chance()) + "% (max 50%); faces attacker.\nShield Attack Lv." + number(rank) + (rank ? ": staggers, damage " + number(run.attack_power(unsigned(Upgrade::ShieldAttack))) : ": locked") +
            "\n" + (rank ? "Bash interval " + seconds(run.shield_cooldown()) + "; radius " + number(unsigned(run.shield_radius())) : "Shield Attack available at level-up.") + "\n" + bonuses;
    }
    if (reward == ChestReward::OrdonSword) return "Dual Wield Lv." + number(run.dual_wield) + "; " + number(run.melee_targets()) + " targets" +
        "\nMelee damage " + number(run.melee_power()) + "; speed +" + number(run.melee_speed_percent()-100) + "%" +
        "; melee crit +" + decimal(run.melee_crit_bonus()/100.f) + "%\nTarget range " + number(unsigned(run.melee_reach(280))) + "; interval " + seconds(run.melee_cooldown()) + "\n" + bonuses;
    if (reward == ChestReward::MasterSword) {
        return ("Sword Lv." + number(run.master_sword) + "; bonus +" + number(run.sword_bonus())) +
            "; melee " + number(run.melee_power()) +
            ("; Reach +" + decimal(15.f * run.rank(Upgrade::Reach)) + "%") +
            "\nWave Lv." + number(run.rank(Upgrade::Wave)) + ": " + (run.rank(Upgrade::Wave) ? number(run.ranged_shots()) + (run.great_spin(Upgrade::Wave) ? " Great Spin, " : " piercing, ") + number(run.attack_power(0)) + " dmg/" + seconds(run.cooldown(60)) : "locked") +
            "\n" + std::string(run.upgrade_name(Upgrade::Spin)) + " Lv." + number(run.rank(Upgrade::Spin)) + ": " + (run.rank(Upgrade::Spin) ? std::string(run.great_spin(Upgrade::Spin) ? "Great Spin, " : "moving, ") + number(run.attack_power(1)) + " dmg/" + seconds(run.cooldown(120)) : "locked") +
            "\n" + ("Jump Strike Lv." + number(run.rank(Upgrade::JumpStrike)) +
                (run.rank(Upgrade::JumpStrike) ? ": " + number(run.jump_splash_percent()) + "% splash; 20%/" + seconds(run.jump_cooldown()) : ": level-up unlock"));
    }
    if (reward == ChestReward::DarkLink) return "Automatic Dark Link; all damage " + dark_multiplier(run) + ".\nLv." + number(run.dark_link) +
        (run.dark_permanent() ? "; permanently active" : "; every " + seconds(run.dark_interval())) +
        "\nDuration: " + (run.dark_permanent() ? std::string("infinite") : seconds(run.dark_duration())) +
        "\nUpgrades alternate damage and duration.";
    return "Restores " + potion_healing(run) + " when used.\nHealing Lv." + number(run.rank(Upgrade::PotionHealing)) + "; +0.25 hearts per upgrade.\nOne equipment slot for all four bottles.\nChests add/refill bottles; bind to use.";
}
inline std::string upgrade_value(Upgrade upgrade, const Run& run) {
    const auto rank = run.rank(upgrade);
    switch (upgrade) {
    case Upgrade::JumpStrike: return rank ? number(run.jump_splash_percent()) + "% melee splash (" + number(run.jump_splash_power()) + " damage); 20% trigger; " + seconds(run.jump_cooldown()) + " cooldown" : "off";
    case Upgrade::DualWield: return number(run.melee_targets()) + " targets; +" + number(run.melee_speed_percent()-100) + "% melee speed; +" + decimal(run.melee_crit_bonus()/100.f) + "% melee crit";
    case Upgrade::MasterSword: return rank ? "+" + number(run.sword_bonus()) + " sword damage; +" + number(run.master_sword/2) + " Lightning Strike damage" : "off";
    case Upgrade::MultiStrike: return "+" + number(run.multi_strikes()) + " extra attacks per activation";
    case Upgrade::Lightning: return rank ? number(run.lightning_power()) + " damage; " + seconds(run.item_cooldown(upgrade)) + " cooldown; " + number(run.lightning_hops()) + " hops; range " + decimal(run.lightning_hop_range()) + "; " + number(run.lightning_proc_chance()) + "% on melee hit; " + number(run.ranged_shots()) + " chains" : "off";
    case Upgrade::Power: return "+" + number(rank) + " damage";
    case Upgrade::Haste: return "-" + number(100 - run.cooldown_percent()) + "% cooldown";
    case Upgrade::Reach: { char value[160];std::snprintf(value,sizeof(value),"+%.1f%% melee/jump/lightning range; +%.1f%% other range; x%.2f Spin radius; x%.2f Bomb/Shield radius",15.f*rank,7.5f*rank,run.melee_area_radius(1.f),std::sqrt(run.radius(1.f)));return value; }
    case Upgrade::Magnet: return number(run.magnet_range()) + " pickup range";
    case Upgrade::Heart: return number(run.maximum_hearts) + " maximum hearts (max 20)";
    case Upgrade::FireArrows: return number(run.burn_damage()) + " damage/s for " + seconds(run.burn_duration());
    case Upgrade::CritChance: { Run base=run;base.ranks[unsigned(Upgrade::Luck)]=0;return crit_chance(base) + " base; " + crit_chance(run) + " with Luck"; }
    case Upgrade::CritDamage: return crit_multiplier(run) + " critical damage";
    case Upgrade::PotionHealing: return potion_healing(run) + " per red potion";
    case Upgrade::Luck: return "+" + number(5*rank) + "% rarity weight; " + crit_chance(run) + " crit; +" + number(std::min(50u,10*rank)) + "% drops";
    case Upgrade::Knockback: return decimal(run.knockback_distance(false,false)) + " push distance; heavy " + decimal(run.knockback_distance(true,false)) + "; no interruption; bosses/elites immune";
    case Upgrade::Piercing: return number(run.arrow_targets()) + " targets per arrow";
    case Upgrade::Multishot: return "+" + number(run.extra_projectiles()) + " projectiles per ranged cast";
    case Upgrade::LifeRegen: return "0.25 hearts every " + seconds(run.life_regen_interval());
    case Upgrade::LifeSteal: return rank ? "0.25 hearts per sword kill" : "off";
    case Upgrade::Hurricane: return rank ? "x2 Boomerang and wind size; piercing flight" : "off";
    case Upgrade::FireBomb: return rank ? "10% bomb damage/s for 5 seconds" : "off";
    case Upgrade::BombArrow: return rank ? "50% bow explosion; radius " + decimal(run.bomb_arrow_radius()) : "off";
    case Upgrade::FireEdge: return rank ? "10% melee damage/s for 5 seconds" : "off";
    case Upgrade::DarkAura: return rank ? "10% target max HP/s; radius " + decimal(run.dark_aura_radius()) : "off";
    case Upgrade::Shockwave: return rank ? number(run.melee_power()) + " damage; radius " + decimal(run.shockwave_radius()) : "off";
    case Upgrade::Rupees: return "+20 rupees";
    default: return "Level " + number(rank);
    }
}
inline std::string upgrade_description(Upgrade upgrade, const Run& run) {
    if (upgrade >= Upgrade::Count) return {};
    if (upgrade == Upgrade::DualWield) return item_description(ChestReward::OrdonSword,run);
    if (upgrade >= Upgrade::Bow && upgrade <= Upgrade::Ball)
        return item_description(ChestReward(unsigned(upgrade) - unsigned(Upgrade::Bow)), run);
    if(upgrade==Upgrade::Lightning) {
        if(!run.rank(upgrade))return std::string(descriptions[unsigned(upgrade)])+"\nLocked";
        return std::string(descriptions[unsigned(upgrade)])+
            "\nDamage " + number(run.lightning_power()) + " | Interval " + seconds(run.item_cooldown(upgrade))+
            "\nChance " + number(run.lightning_proc_chance()) + "% | Extra targets " + number(run.lightning_hops())+
            "\nJump range " + decimal(run.lightning_hop_range()) + " | Chains " + number(run.ranged_shots());
    }
    if(upgrade==Upgrade::LifeSteal)return std::string(descriptions[unsigned(upgrade)])+
        "\nHealing: " + upgrade_value(upgrade,run)+
        "\nApplies to Sword Attack, Dual Wield splash, Spin Attack, Sword Wave, Jump Strike and Lightning Strike.";
    std::string text = descriptions[unsigned(upgrade)];
    if (upgrade == Upgrade::Wave || upgrade == Upgrade::Spin || upgrade == Upgrade::ShieldAttack) {
        text += "\nDamage " + number(run.attack_power(unsigned(upgrade)));
        text += "; interval " + seconds(upgrade == Upgrade::ShieldAttack ? run.shield_cooldown() : run.cooldown(upgrade == Upgrade::Wave ? 60 : 120));
        if (upgrade == Upgrade::Wave) text += "; " + number(run.ranged_shots()) + " waves; radius " + decimal(run.wave_radius());
        if (upgrade == Upgrade::Wave || upgrade == Upgrade::Spin) text += run.great_spin(upgrade) ? "; Great Spin" : "; normal spin";
        if (upgrade == Upgrade::ShieldAttack) text += "; radius " + number(unsigned(run.shield_radius()));
    } else text += "\n" + upgrade_value(upgrade, run);
    return text;
}
inline std::vector<ItemChange> upgrade_changes(Upgrade upgrade, const Run& before, const Run& after, Rarity rarity = Rarity::Common) {
    if (upgrade == Upgrade::MasterSword) {
        auto changes = reward_changes(ChestReward::MasterSword,before,after);
        changes.insert(changes.begin(), {"Level", number(before.master_sword), number(after.master_sword)});
        return changes;
    }
    if (upgrade == Upgrade::DualWield) return reward_changes(ChestReward::OrdonSword,before,after);
    if (upgrade >= Upgrade::Bow && upgrade <= Upgrade::Ball)
        return reward_changes(ChestReward(unsigned(upgrade) - unsigned(Upgrade::Bow)), before, after);
    if (upgrade == Upgrade::Rupees) return {{"Reward", "0", "+" + number(20 * reward_units(rarity)) + " rupees"}};
    std::vector<ItemChange> changes{{"Level", number(before.rank(upgrade)), number(after.rank(upgrade))}};
    if (upgrade == Upgrade::Wave || upgrade == Upgrade::Spin || upgrade == Upgrade::ShieldAttack) {
        changes.push_back({"Damage", before.rank(upgrade) ? number(before.attack_power(unsigned(upgrade))) : "off",
                           number(after.attack_power(unsigned(upgrade)))});
        if ((upgrade == Upgrade::Wave || upgrade == Upgrade::Spin) && before.great_spin(upgrade) != after.great_spin(upgrade))
            changes.push_back({"Attack", "Normal Spin", "Great Spin (larger area)"});
        if (upgrade == Upgrade::Wave && (!before.rank(upgrade) || before.wave_radius() != after.wave_radius()))
            changes.push_back({"Radius", before.rank(upgrade) ? decimal(before.wave_radius()) : "off", decimal(after.wave_radius())});
        if (upgrade == Upgrade::ShieldAttack) {
            changes.push_back({"Interval", before.rank(upgrade) ? seconds(before.shield_cooldown()) : "off",
                               seconds(after.shield_cooldown())});
            changes.push_back({"Radius", before.rank(upgrade) ? number(unsigned(before.shield_radius())) : "off",
                               number(unsigned(after.shield_radius()))});
        }
    } else if(upgrade==Upgrade::Lightning) {
        const auto add=[&](const char* label,const std::string& a,const std::string& b){
            if(!before.rank(upgrade) || a!=b)changes.push_back({label,before.rank(upgrade)?a:"off",b});
        };
        add("Damage",number(before.lightning_power()),number(after.lightning_power()));
        add("Interval",seconds(before.item_cooldown(upgrade)),seconds(after.item_cooldown(upgrade)));
        add("Extra targets",number(before.lightning_hops()),number(after.lightning_hops()));
        add("Jump range",decimal(before.lightning_hop_range()),decimal(after.lightning_hop_range()));
        add("Trigger chance",number(before.lightning_proc_chance())+"%",number(after.lightning_proc_chance())+"%");
        add("Chains",number(before.ranged_shots()),number(after.ranged_shots()));
    } else if(upgrade==Upgrade::LifeSteal) {
        changes.push_back({"Healing per sword kill",before.rank(upgrade)?"0.25 hearts":"off","0.25 hearts"});
    } else if(upgrade==Upgrade::MultiStrike) {
        changes.push_back({"Extra attacks",number(before.multi_strikes()),number(after.multi_strikes())});
    } else changes.push_back({"Effect", upgrade_value(upgrade, before), upgrade_value(upgrade, after)});
    return changes;
}
} // namespace dawnlight::survivors

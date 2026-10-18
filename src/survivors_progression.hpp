#pragma once
#include "general_modes.hpp"
#include <array>
#include <algorithm>

namespace dawnlight::survivors {
enum class Talent : unsigned { MaxHealth, CritChance, Sprint, SprintSpeed, Magnet, ManualJump, Glider, Gale, GaleCharges, Rerolls, Banishes, Dodge, Count };
inline constexpr std::array talent_names{"Max Health", "Crit Chance", "Sprint", "Sprint Speed", "Magnet", "Manual Jump", "Glider", "Revali's Gale", "Gale Charges", "Rerolls", "Banishes", "Dodge"};
inline constexpr std::array<unsigned,unsigned(Talent::Count)> talent_caps{5,5,1,2,5,1,1,1,2,2,2,5};
inline constexpr std::array talent_help{
    "+1 starting heart per rank (up to 8). Run Heart Containers remain separate.",
    "+1 percentage point base critical chance per rank. Luck multiplies it; final chance caps at 100%.",
    "Hold the normal Sprint input to run at 125% speed, including automatic combat.",
    "Requires Sprint. +12.5 percentage points sprint speed per rank, up to 150%.",
    "+10% pickup range per rank, applied after run Magnet upgrades. Includes XP, rupees and hearts.",
    "Unlock Manual Jump using your configured Jump button or touch button.",
    "Requires Manual Jump. Press Jump in the air to deploy the Glider.",
    "Requires Glider. Hold Jump through landing to charge Revali's Gale. One charge; 120-second recovery.",
    "Requires Revali's Gale. +1 charge per rank, up to three. Charges refill for each new run.",
    "+1 free reroll per rank each run, up to three. Shared by level-ups and chests.",
    "+1 banishment per rank each run, up to three. Shared by level-ups and chests.",
    "Avoid enemy hits: 5% / 6.25% / 7.5% / 8.75% / 10%. Independent of Luck and shield blocks."};
inline constexpr Talent talent_parent(Talent t) {
    switch(t) {
    case Talent::SprintSpeed: return Talent::Sprint;
    case Talent::Glider: return Talent::ManualJump;
    case Talent::Gale: return Talent::Glider;
    case Talent::GaleCharges: return Talent::Gale;
    default: return Talent::Count;
    }
}
struct Talents {
    std::array<unsigned,unsigned(Talent::Count)> ranks{};
    unsigned rank(Talent t) const { return t<Talent::Count ? ranks[unsigned(t)] : 0; }
    unsigned spent() const { unsigned n=0;for(auto r:ranks)n+=r;return n; }
    bool valid() const {
        for(unsigned i=0;i<ranks.size();++i)
            if(ranks[i]>talent_caps[i] || (ranks[i] && talent_parent(Talent(i))!=Talent::Count && !rank(talent_parent(Talent(i)))))return false;
        return true;
    }
    bool can_buy(Talent t) const {
        return t<Talent::Count && rank(t)<talent_caps[unsigned(t)] &&
            (talent_parent(t)==Talent::Count || rank(talent_parent(t))>0);
    }
    bool refund(Talent t) {
        if(!rank(t))return false;
        --ranks[unsigned(t)];
        // Refund dependents recursively when their prerequisite reaches zero.
        bool changed;
        do { changed=false;
            for(unsigned i=0;i<ranks.size();++i)if(ranks[i] && talent_parent(Talent(i))!=Talent::Count && !rank(talent_parent(Talent(i)))) {
                ranks[i]=0;changed=true;
            }
        } while(changed);
        return true;
    }
    unsigned dodge_basis_points() const { const auto r=rank(Talent::Dodge); return r ? 500+125*(std::min(5u,r)-1) : 0; }
    unsigned starting_hearts() const { return 3+rank(Talent::MaxHealth); }
    float sprint_multiplier() const { return rank(Talent::Sprint) ? 1.25f+0.125f*rank(Talent::SprintSpeed) : 1.f; }
};
inline bool talent_mode_override(const Talents& talents, ModeSetting setting, int64_t& value) {
    switch(setting) {
    case ModeSetting::Sprint: value=talents.rank(Talent::Sprint)>0;return true;
    case ModeSetting::SprintSpeed: value=int64_t(talents.sprint_multiplier()*100+0.5f);return true; // Config UI rounds; gameplay keeps the exact fraction.
    case ModeSetting::Jump: value=talents.rank(Talent::ManualJump)>0;return true;
    case ModeSetting::Glide: value=talents.rank(Talent::Glider)>0;return true;
    case ModeSetting::GlideItem: value=1;return true;
    case ModeSetting::Gale: case ModeSetting::GaleCounter: value=talents.rank(Talent::Gale)>0;return true;
    case ModeSetting::GaleCharges: value=1+talents.rank(Talent::GaleCharges);return true;
    case ModeSetting::GaleRecovery: value=120;return true;
    case ModeSetting::GaleHeight: value=500;return true;
    default:return false;
    }
}
// Total XP required for n completed meta levels; each grants one Unlock Point.
inline constexpr uint64_t meta_threshold(unsigned n) { return uint64_t(n)*2000+uint64_t(n)*(n ? n-1 : 0)*250; }
inline constexpr uint64_t max_meta_xp=1000000000000ULL;
inline unsigned meta_levels(uint64_t xp) {
    unsigned low=0,high=100000;
    while(low<high){const unsigned mid=(low+high+1)/2;if(meta_threshold(mid)<=xp)low=mid;else high=mid-1;}
    return low;
}
} // namespace dawnlight::survivors

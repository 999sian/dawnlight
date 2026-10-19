#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>

namespace dawnlight::survivors {
enum class DamageSource : unsigned {
    Sword, Spin, Jump, Wave, Bow, Boomerang, Bombs, Ball, Shield, Shockwave,
    DualWield, FireArrows, FireBomb, BombArrow, FireEdge, DarkAura, Other, DarkLink, Count
};
inline constexpr unsigned damage_source_count=unsigned(DamageSource::Count);
inline constexpr std::array damage_source_names{
    "Sword Attack","Spin Attack","Jump Strike","Sword Wave","Hero's Bow","Boomerang",
    "Bombs","Ball and Chain","Shield Attack","Shockwave","Dual Wield",
    "Fire Arrows (burn)","Fire Bomb (burn)","Bomb Arrow (blast)","Fire Edge (burn)","Dark Aura","Other damage","Dark Link"};
using DamageAmounts=std::array<uint64_t,damage_source_count>;
inline uint64_t damage_sum(const DamageAmounts& values) {
    return std::accumulate(values.begin(),values.end(),uint64_t(0));
}
// Provenance travels alongside the existing aggregate, without changing combat.
struct DamageQueue {
    DamageAmounts pending{},fixedPending{},applied{},fixedApplied{};
    void add(DamageSource source,unsigned amount,bool fixed=false) {
        (fixed?fixedPending:pending)[unsigned(source)]+=amount;
    }
    void consume() {applied=pending;fixedApplied=fixedPending;pending={};fixedPending={};}
    DamageAmounts weights(unsigned raw,unsigned scaled,unsigned fixed) const {
        auto normal=applied,flat=fixedApplied;
        // Retain attribution for any future producer not yet explicitly tagged.
        if(damage_sum(normal)<raw)normal[unsigned(DamageSource::Other)]+=raw-damage_sum(normal);
        if(damage_sum(flat)<fixed)flat[unsigned(DamageSource::Other)]+=fixed-damage_sum(flat);
        const uint64_t denominator=std::max(uint64_t(1),damage_sum(normal));
        DamageAmounts result{};
        for(unsigned i=0;i<result.size();++i)result[i]=normal[i]*std::min(raw,scaled)+flat[i]*denominator;
        if(scaled>raw)result[unsigned(DamageSource::DarkLink)]+=uint64_t(scaled-raw)*denominator;
        return result;
    }
};
struct DamageTotals {
    DamageAmounts dealt{};
    uint64_t total() const {return damage_sum(dealt);}
    void record(const DamageAmounts& weights,int before,int after) {
        const uint64_t sum=damage_sum(weights);
        if(!sum || before<=0 || after>=before)return;
        const unsigned actual=unsigned(before-std::max(0,after));
        unsigned remaining=actual;
        std::array<long double,damage_source_count> fractions{};
        // Largest-remainder allocation preserves the exact HP delta, including
        // capped/overkill aggregate hits. No damage is counted at enqueue time.
        for(unsigned i=0;i<weights.size();++i) {
            const long double share=static_cast<long double>(actual)*weights[i]/sum;
            const auto whole=std::min(remaining,unsigned(share));
            dealt[i]+=whole;remaining-=whole;fractions[i]=share-whole;
        }
        while(remaining--) {
            const unsigned i=unsigned(std::max_element(fractions.begin(),fractions.end())-fractions.begin());
            ++dealt[i];fractions[i]=-1;
        }
    }
};
}

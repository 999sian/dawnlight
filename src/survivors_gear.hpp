#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace dawnlight::survivors {
// Values 0/1/2 are the original v1 Empty/Sword/Tunic identifiers.
enum class GearKind : uint8_t {
    Empty, Sword, Tunic, Greatsword, HerosBow, SharpshooterBow,
    StormBoomerang, HurricaneBoomerang, BombBag, SiegeBombs, IronBall, BreakerBall, Count
};
enum class GearSlot { Weapon, Tunic };
enum class GearFamily { None, Sword, Bow, Boomerang, Bombs, Ball, Tunic };
enum class GearStat { None, Damage, Crit, Speed, Reach, Knockback, Piercing, HopReach, Hops, FlightSpeed, BlastRadius, Hearts, Dodge };
struct GearAffix {
    GearStat stat=GearStat::None;
    const char* label="";
    unsigned scale=1;
    bool percent=false, count=false;
};
struct GearDefinition {
    const char* name;
    GearFamily family;
    std::array<GearAffix,3> affixes;
};
inline constexpr std::array<GearDefinition,unsigned(GearKind::Count)> gear_definitions{{
    {"Empty slot",GearFamily::None,{}},
    {"Hero's Sword",GearFamily::Sword,{{{GearStat::Damage,"sword damage"},{GearStat::Crit,"melee critical chance",1,true},{GearStat::Speed,"melee attack speed",1,true}}}},
    {"Tunic",GearFamily::Tunic,{{{GearStat::Hearts,"max hearts"},{GearStat::Dodge,"dodge chance",1,true},{}}}},
    {"Greatsword",GearFamily::Sword,{{{GearStat::Damage,"sword damage"},{GearStat::Reach,"melee reach",2,true},{GearStat::Knockback,"sword knockback",5,true}}}},
    {"Hero's Bow",GearFamily::Bow,{{{GearStat::Damage,"bow damage"},{GearStat::Crit,"bow critical chance",1,true},{GearStat::Speed,"bow attack speed",1,true}}}},
    {"Sharpshooter Bow",GearFamily::Bow,{{{GearStat::Damage,"bow damage"},{GearStat::Reach,"arrow range",5,true},{GearStat::Piercing,"additional pierced targets",1,false,true}}}},
    {"Storm Boomerang",GearFamily::Boomerang,{{{GearStat::Damage,"boomerang damage"},{GearStat::Speed,"boomerang attack speed",1,true},{GearStat::HopReach,"hop range",5,true}}}},
    {"Hurricane Boomerang",GearFamily::Boomerang,{{{GearStat::Damage,"boomerang damage"},{GearStat::Hops,"additional target hops",1,false,true},{GearStat::FlightSpeed,"boomerang flight speed",5,true}}}},
    {"Bomb Bag",GearFamily::Bombs,{{{GearStat::Damage,"bomb damage"},{GearStat::BlastRadius,"explosion radius",2,true},{GearStat::Speed,"bomb throw speed",1,true}}}},
    {"Siege Bombs",GearFamily::Bombs,{{{GearStat::Damage,"bomb damage"},{GearStat::BlastRadius,"explosion radius",2,true},{GearStat::Knockback,"bomb knockback",5,true}}}},
    {"Iron Ball",GearFamily::Ball,{{{GearStat::Damage,"ball and chain damage"},{GearStat::Speed,"ball and chain attack speed",1,true},{GearStat::Reach,"ball and chain range",5,true}}}},
    {"Breaker Ball",GearFamily::Ball,{{{GearStat::Damage,"ball and chain damage"},{GearStat::Knockback,"ball and chain knockback",5,true},{GearStat::FlightSpeed,"ball and chain flight speed",5,true}}}}
}};
struct GearItem {
    GearKind kind=GearKind::Empty;
    // Ordinary affixes store tiers 1..5; count affixes store their actual 1..2.
    uint8_t first=0, second=0, third=0;
    bool operator==(const GearItem&) const = default;
    const GearDefinition& definition() const {
        return gear_definitions[unsigned(kind)<gear_definitions.size()?unsigned(kind):0];
    }
    GearFamily family() const { return definition().family; }
    bool is_weapon() const { return family()!=GearFamily::None && family()!=GearFamily::Tunic; }
    std::array<uint8_t,3> stats() const { return {first,second,third}; }
    bool valid() const {
        if(unsigned(kind)>=gear_definitions.size())return false;
        const auto values=stats();
        for(unsigned i=0;i<values.size();++i) {
            const auto& affix=definition().affixes[i];
            if(affix.stat==GearStat::None) {if(values[i])return false;}
            else if(values[i]<1 || values[i]>(affix.count?2:5))return false;
        }
        return true;
    }
    unsigned bonus(GearStat stat) const {
        const auto values=stats();
        for(unsigned i=0;i<values.size();++i)if(definition().affixes[i].stat==stat && stat!=GearStat::None)
            return values[i]*definition().affixes[i].scale;
        return 0;
    }
    unsigned rarity() const {
        unsigned count=0,sum=0;const auto values=stats();
        for(unsigned i=0;i<values.size();++i)if(definition().affixes[i].stat!=GearStat::None) {
            ++count;sum+=definition().affixes[i].count?(values[i]==2?5:1):values[i];
        }
        return !count?0:sum*2>=count*9?4:sum>=count*4?3:sum>=count*3?2:sum>=count*2?1:0;
    }
};
struct GearInventory {
    static constexpr unsigned page_size=20, page_count=2, capacity=page_size*page_count, none=capacity;
    std::array<GearItem,capacity> items{};
    unsigned weapon=none,tunic=none;
    bool operator==(const GearInventory&) const = default;
    GearItem equipped(GearSlot slot) const {
        const auto i=slot==GearSlot::Weapon?weapon:tunic;
        if(i>=capacity)return {};
        const auto item=items[i];
        return (slot==GearSlot::Weapon?item.is_weapon():item.kind==GearKind::Tunic)?item:GearItem{};
    }
    bool equip(unsigned i) {
        if(i>=capacity||!items[i].valid()||items[i].kind==GearKind::Empty)return false;
        (items[i].is_weapon()?weapon:tunic)=i;return true;
    }
    bool unequip(unsigned i) {
        if(i>=capacity)return false;
        if(weapon==i){weapon=none;return true;}
        if(tunic==i){tunic=none;return true;}
        return false;
    }
    bool erase(unsigned i) {
        if(i>=capacity||items[i].kind==GearKind::Empty)return false;
        if(weapon==i)weapon=none;
        if(tunic==i)tunic=none;
        items[i]={};return true;
    }
    unsigned add(GearItem item) {
        if(!item.valid()||item.kind==GearKind::Empty)return none;
        for(unsigned i=0;i<capacity;++i)if(items[i].kind==GearKind::Empty){items[i]=item;return i;}
        return none;
    }
    static unsigned stat(unsigned roll) {return roll<50?1:roll<75?2:roll<90?3:roll<98?4:5;}
    template<class Random> static GearItem roll(Random&& random) {
        // 50% Tunic / 50% Weapon; five equally likely families, two variants each.
        static constexpr std::array kinds{GearKind::Sword,GearKind::Greatsword,GearKind::HerosBow,GearKind::SharpshooterBow,
            GearKind::StormBoomerang,GearKind::HurricaneBoomerang,GearKind::BombBag,GearKind::SiegeBombs,GearKind::IronBall,GearKind::BreakerBall};
        const auto kind=random(2)==0?kinds[random(kinds.size())]:GearKind::Tunic;
        GearItem item{kind};std::array<uint8_t*,3> values{&item.first,&item.second,&item.third};
        for(unsigned i=0;i<values.size();++i) {
            const auto& affix=item.definition().affixes[i];
            if(affix.stat!=GearStat::None)*values[i]=uint8_t(affix.count?(random(100)<90?1:2):stat(random(100)));
        }
        return item;
    }
    // Version 3 expands to two pages. Versions 1/2 keep their 20 slots and
    // translate the old empty equipped-slot sentinel (20) to the new one (40).
    static constexpr unsigned legacy_capacity=20, legacy_byte_count=12+legacy_capacity*4;
    static constexpr unsigned byte_count=12+capacity*4;
    using Bytes=std::array<uint8_t,byte_count>;
    Bytes encode() const {
        Bytes b{'H','S','G','R',3,uint8_t(weapon),uint8_t(tunic),0};
        for(unsigned i=0;i<capacity;++i){b[8+4*i]=uint8_t(items[i].kind);b[9+4*i]=items[i].first;b[10+4*i]=items[i].second;b[11+4*i]=items[i].third;}
        uint32_t hash=2166136261u;for(unsigned i=0;i<byte_count-4;++i)hash=(hash^b[i])*16777619u;
        for(unsigned i=0;i<4;++i)b[byte_count-4+i]=uint8_t(hash>>(8*i));
        return b;
    }
    bool decode(std::span<const uint8_t> b) {
        if(b.size()!=byte_count&&b.size()!=legacy_byte_count)return false;
        if(b[0]!='H'||b[1]!='S'||b[2]!='G'||b[3]!='R'||b[7])return false;
        const bool legacy=b[4]==1||b[4]==2;
        if(!legacy&&b[4]!=3)return false;
        const unsigned storedCapacity=legacy?legacy_capacity:capacity;
        if(b.size()!=12+storedCapacity*4||b[5]>storedCapacity||b[6]>storedCapacity)return false;
        uint32_t hash=2166136261u;for(unsigned i=0;i<b.size()-4;++i)hash=(hash^b[i])*16777619u;
        for(unsigned i=0;i<4;++i)if(b[b.size()-4+i]!=uint8_t(hash>>(8*i)))return false;
        GearInventory next;next.weapon=b[5]==storedCapacity?none:b[5];next.tunic=b[6]==storedCapacity?none:b[6];
        for(unsigned i=0;i<storedCapacity;++i){next.items[i]={GearKind(b[8+4*i]),b[9+4*i],b[10+4*i],b[11+4*i]};if(!next.items[i].valid()||(b[4]==1&&b[8+4*i]>2))return false;}
        if((next.weapon!=none&&!next.items[next.weapon].is_weapon())||(next.tunic!=none&&next.items[next.tunic].kind!=GearKind::Tunic))return false;
        *this=next;return true;
    }
};
}

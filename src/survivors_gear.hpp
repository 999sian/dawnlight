#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace dawnlight::survivors {
enum class GearKind : uint8_t { Empty, Sword, Tunic };
struct GearItem {
    GearKind kind=GearKind::Empty;
    uint8_t first=0, second=0, third=0;
    bool operator==(const GearItem&) const = default;
    bool valid() const {
        if(kind==GearKind::Empty)return !first&&!second&&!third;
        if(kind!=GearKind::Sword&&kind!=GearKind::Tunic)return false;
        return first>=1&&first<=5&&second>=1&&second<=5&&
            (kind==GearKind::Sword?(third>=1&&third<=5):third==0);
    }
    unsigned rarity() const {
        const unsigned count=kind==GearKind::Sword?3:2, sum=first+second+third;
        return sum*2>=count*9?4:sum>=count*4?3:sum>=count*3?2:sum>=count*2?1:0;
    }
};
struct GearInventory {
    static constexpr unsigned capacity=20, none=capacity;
    std::array<GearItem,capacity> items{};
    unsigned sword=none,tunic=none;
    bool operator==(const GearInventory&) const = default;
    GearItem equipped(GearKind kind) const {
        const auto i=kind==GearKind::Sword?sword:tunic;
        return i<capacity&&items[i].kind==kind?items[i]:GearItem{};
    }
    bool equip(unsigned i) {
        if(i>=capacity||items[i].kind==GearKind::Empty)return false;
        (items[i].kind==GearKind::Sword?sword:tunic)=i;return true;
    }
    bool erase(unsigned i) {
        if(i>=capacity||items[i].kind==GearKind::Empty)return false;
        if(sword==i)sword=none;
        if(tunic==i)tunic=none;
        items[i]={};return true;
    }
    unsigned add(GearItem item) {
        if(!item.valid()||item.kind==GearKind::Empty)return none;
        for(unsigned i=0;i<capacity;++i)if(items[i].kind==GearKind::Empty){items[i]=item;return i;}
        return none;
    }
    // Each stat has independently decreasing probabilities: 50/25/15/8/2%.
    static unsigned stat(unsigned roll) {return roll<50?1:roll<75?2:roll<90?3:roll<98?4:5;}
    template<class Random> static GearItem roll(Random&& random) {
        const auto kind=random(2)==0?GearKind::Sword:GearKind::Tunic;
        GearItem item{kind,uint8_t(stat(random(100))),uint8_t(stat(random(100))),0};
        if(kind==GearKind::Sword)item.third=uint8_t(stat(random(100)));
        return item;
    }
    // Separate slot-local blob: old progression/highscore formats stay intact.
    static constexpr unsigned byte_count=12+capacity*4;
    using Bytes=std::array<uint8_t,byte_count>;
    Bytes encode() const {
        Bytes b{'H','S','G','R',1,uint8_t(sword),uint8_t(tunic),0};
        for(unsigned i=0;i<capacity;++i){b[8+4*i]=uint8_t(items[i].kind);b[9+4*i]=items[i].first;b[10+4*i]=items[i].second;b[11+4*i]=items[i].third;}
        uint32_t hash=2166136261u;for(unsigned i=0;i<byte_count-4;++i)hash=(hash^b[i])*16777619u;
        for(unsigned i=0;i<4;++i)b[byte_count-4+i]=uint8_t(hash>>(8*i));
        return b;
    }
    bool decode(std::span<const uint8_t> b) {
        if(b.size()!=byte_count||b[0]!='H'||b[1]!='S'||b[2]!='G'||b[3]!='R'||b[4]!=1||b[5]>none||b[6]>none||b[7])return false;
        uint32_t hash=2166136261u;for(unsigned i=0;i<byte_count-4;++i)hash=(hash^b[i])*16777619u;
        for(unsigned i=0;i<4;++i)if(b[byte_count-4+i]!=uint8_t(hash>>(8*i)))return false;
        GearInventory next;next.sword=b[5];next.tunic=b[6];
        for(unsigned i=0;i<capacity;++i){next.items[i]={GearKind(b[8+4*i]),b[9+4*i],b[10+4*i],b[11+4*i]};if(!next.items[i].valid())return false;}
        if((next.sword!=none&&next.items[next.sword].kind!=GearKind::Sword)||(next.tunic!=none&&next.items[next.tunic].kind!=GearKind::Tunic))return false;
        *this=next;return true;
    }
};
}

#pragma once
#include <array>
namespace dawnlight::survivors {
// One finite series per ability. Repeats never arm another series or reset the
// primary cooldown. Native body actions wait for the previous hit window/pose.
enum class StrikeSeries : unsigned { Sword, Spin, Wave, Count };
struct MultiStrikeState {
    std::array<unsigned,unsigned(StrikeSeries::Count)> remaining{};
    unsigned waveDelay=0;
    bool waveManual=false;
    unsigned& count(StrikeSeries kind) { return remaining[unsigned(kind)]; }
    void start(StrikeSeries kind,unsigned extra) { count(kind)=extra; }
    void consume(StrikeSeries kind) { if(count(kind))--count(kind); }
};
}

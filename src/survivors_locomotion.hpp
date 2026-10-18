#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace dawnlight::survivors {
// Native attention movement: positive local X is Link's left.
// Order: forward, left, backward, right. Adjacent directions blend continuously.
inline std::array<float,4> locomotion_weights(int16_t relativeAngle) {
    const float sector=float(uint16_t(relativeAngle))/16384.f;
    const unsigned first=unsigned(sector);
    const float blend=sector-first;
    std::array<float,4> weights{};
    weights[first]=1-blend;
    weights[(first+1)%4]=blend;
    return weights;
}
inline float locomotion_run_blend(float speed,float maxSpeed,float walkChange,float runChange) {
    const float ratio=speed/std::max(1.f,maxSpeed);
    return std::clamp((ratio-walkChange)/std::max(.01f,runChange-walkChange),0.f,1.f);
}
} // namespace dawnlight::survivors

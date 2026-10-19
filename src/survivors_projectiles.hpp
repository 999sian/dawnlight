#pragma once
#include <algorithm>
#include <deque>
#include "survivors_rules.hpp"

namespace dawnlight::survivors {
// Reuse inactive objects; append instead of dropping shots when all are busy.
// deque growth preserves collider/status addresses retained by the engine.
// Never erase objects while the collision server may still reference them.
template<class Attack>
Attack& acquire_projectile(std::deque<Attack>& attacks, unsigned weapon, unsigned begin, unsigned count) {
    for (unsigned i = begin; i < begin + count; ++i)
        if (!attacks[i].active) return attacks[i];
    for (unsigned i = initial_projectile_slots; i < attacks.size(); ++i)
        if (!attacks[i].active && attacks[i].weapon == weapon) return attacks[i];
    attacks.emplace_back();
    attacks.back().weapon = weapon;
    return attacks.back();
}
}

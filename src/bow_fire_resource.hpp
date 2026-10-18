#pragma once

#include <cstdint>

namespace dawnlight {

// Returns zero if the disc resource could not be loaded; no substitute effect.
std::uint16_t bow_fire_effect();
// Original Dodongo breath/charge effects, sharing the same owned resource table.
std::uint16_t dodongo_fire_effect(std::uint16_t native);
void shutdown_bow_fire_resource();

}  // namespace dawnlight

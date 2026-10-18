#pragma once
#include "mods/api.h"
#include "general_modes.hpp"

namespace dawnlight {
ModResult initialize_survivors(ModError* error);
void update_survivors();
void shutdown_survivors();
bool survivors_active();
bool survivors_mobility_override(ModeSetting setting, int64_t& value);
float survivors_sprint_multiplier();
bool survivors_dual_wield();
bool survivors_arena_active();
bool survivors_dark_link_active();
} // namespace dawnlight

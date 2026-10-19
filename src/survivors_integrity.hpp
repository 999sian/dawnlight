#pragma once
#include "mods/api.h"
namespace dawnlight {
ModResult initialize_survivors_integrity(ModError*);
void begin_survivors_integrity(bool allow_cheats, bool& modded);
void update_survivors_integrity(bool& modded);
void end_survivors_integrity();
void shutdown_survivors_integrity();
}

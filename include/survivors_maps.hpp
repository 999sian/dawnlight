#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace dawnlight::survivors {
struct Arena {
    const char* name;
    const char* stage;
    std::int8_t room;
    std::int16_t point;
    const char* description;
    const char* accent;
    std::array<std::uint8_t, 3> barrier_color;
};
// Native PLYR points, not world coordinates. F_SP121/6 is the Faron field;
// D_MN08/0 is the exterior entrance; D_MN09A/50 is the intact throne room.
inline constexpr std::array<Arena, 5> arenas{{
    {"Darknut Arena", "D_DLBR0", 51, 0,
     "The original compact arena. Keep moving as the horde closes in.", "#dfb95f", {230, 185, 75}},
    {"Hyrule Field", "F_SP121", 6, 0,
     "Faron's open field. Wide routes for circling the horde; exits are sealed.", "#8dc97b", {120, 210, 150}},
    {"Gerudo Desert", "F_SP124", 0, 0,
     "Open desert, dunes and ancient ruins. Plenty of space, little cover.", "#edc37c", {245, 190, 90}},
    {"Palace of Twilight", "D_MN08", 0, 0,
     "The exterior entrance platforms. Narrow approaches demand careful movement.", "#bf9bea", {155, 95, 240}},
    {"Hyrule Castle Throne Room", "D_MN09A", 50, 0,
     "The intact throne room. A confined royal arena with sealed doors.", "#e8a2a2", {230, 105, 120}},
}};
inline bool arena_matches(unsigned index, std::string_view stage, int room) {
    return index < arenas.size() && stage == arenas[index].stage && room == arenas[index].room;
}
} // namespace dawnlight::survivors

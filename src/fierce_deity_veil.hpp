#pragma once

class daAlink_c;
namespace dawnlight {
// Rollback switch: true restores the preserved FIRE02 body-particle effect.
// The two body effects are mutually exclusive; eye bloom is independent.
inline constexpr bool kUseLegacyDarkFlames = false;
void initialize_dark_veil();
void shutdown_dark_veil();
void tick_dark_veil(daAlink_c* link, bool enabled);
void stop_dark_veil();
}

#pragma once

class daAlink_c;
namespace dawnlight {
void initialize_dark_veil();
void shutdown_dark_veil();
void tick_dark_veil(daAlink_c* link, bool enabled);
void stop_dark_veil();
}

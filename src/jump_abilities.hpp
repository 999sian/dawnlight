#pragma once

class daAlink_c;

namespace dawnlight {
// True only while this Link holds the carrier owned by Dawnlight's glide session.
bool glide_active_for(daAlink_c* link);
// Includes the first Gale jump, its landing/charge and a pending glide attach.
bool jump_abilities_busy_for(daAlink_c* link);
}

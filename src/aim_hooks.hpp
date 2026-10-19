#pragma once

class daAlink_c;
class dCamera_c;
class cXyz;

namespace dawnlight {

bool third_person_reticle_ray(dCamera_c* camera, int offsetY, float fovy, cXyz& eye, cXyz& forward);
void third_person_aim_frame(daAlink_c* link, dCamera_c* camera, short yaw, short pitch,
    float distance, float height, cXyz& center, cXyz& eye);

void prepare_bullet_time_bow_aim(daAlink_c* link);
bool update_bullet_time_bow_aim(daAlink_c* link);

}  // namespace dawnlight

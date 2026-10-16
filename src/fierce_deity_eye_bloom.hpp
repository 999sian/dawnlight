#pragma once

namespace dawnlight {
void initialize_eye_bloom();
void shutdown_eye_bloom();
// Called around one immediate eye-primitive replay, while its skinning/UVs are live.
bool begin_eye_bloom_capture();
void end_eye_bloom_capture();
}

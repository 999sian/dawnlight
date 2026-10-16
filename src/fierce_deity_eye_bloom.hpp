#pragma once

namespace dawnlight {
void initialize_eye_bloom();
void shutdown_eye_bloom();
// Bracket the native Item3D draw lists, after world effects and before menu overlays.
void begin_collection_eye_bloom();
void end_collection_eye_bloom();
// Called around one immediate eye-primitive replay, while its skinning/UVs are live.
bool begin_eye_bloom_capture();
void end_eye_bloom_capture();
}

#pragma once

#include "f_op/f_op_actor_mng.h"
#include <cstddef>
#include <cstring>

namespace dawnlight {
// fpcMtd_Method also dispatches create_request/node_create_request callbacks.
// Those objects are smaller than an actor: fopAcM_IsActor alone reads beyond
// them. Inspect only the shallow process tag before any process/actor fields.
inline bool is_base_process(const void* process) {
    if (!process || !g_fpcBs_type) return false;
    int type;
    // Dusklight's PC base has a vtable, so type is not at byte zero. memcpy
    // permits inspecting a request's bytes without treating it as a C++ object.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
    std::memcpy(&type, static_cast<const char*>(process) + offsetof(base_process_class, type), sizeof(type));
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
    return type == g_fpcBs_type;
}

inline bool is_actor_process(void* process) {
    if (!is_base_process(process)) return false;
    const auto* base = static_cast<const base_process_class*>(process);
    if (base->methods != &g_fpcLf_Method.base) return false;
    const auto* leaf = static_cast<const leafdraw_class*>(process);
    // Scenes and other process types are not necessarily actor-sized either.
    return leaf->leaf_methods == &g_fopAc_Method.base && fopAcM_IsActor(process);
}
} // namespace dawnlight

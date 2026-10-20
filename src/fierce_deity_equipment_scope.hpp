#pragma once

namespace dawnlight::dark_link {
// World models can be submitted by a post-hook inside Link's actor draw.
// Do not mistake those submissions for attached equipment in the deferred pass.
// Nested scopes restore capture automatically before held equipment is drawn.
class WorldModelDrawScope {
    static inline unsigned depth_ = 0;
public:
    WorldModelDrawScope() { ++depth_; }
    ~WorldModelDrawScope() { --depth_; }
    WorldModelDrawScope(const WorldModelDrawScope&) = delete;
    WorldModelDrawScope& operator=(const WorldModelDrawScope&) = delete;
    static bool active() { return depth_ != 0; }
};
} // namespace dawnlight::dark_link

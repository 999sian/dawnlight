#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace dawnlight::dark_veil {
struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3 operator+(Vec3 b) const { return {x+b.x, y+b.y, z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x, y-b.y, z-b.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
};
inline float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float length(Vec3 a) { return std::sqrt(dot(a,a)); }
inline Vec3 normalized(Vec3 a) { const auto n=length(a); return n>0.0001f ? a*(1/n) : Vec3{0,1,0}; }
inline Vec3 limited(Vec3 a, float limit) { const auto n=length(a); return n>limit ? a*(limit/n) : a; }
using Pose = std::array<std::array<float,4>,3>;
inline Vec3 origin(const Pose& p) { return {p[0][3],p[1][3],p[2][3]}; }
inline Vec3 rotate(const Pose& p, Vec3 v) {
    return {p[0][0]*v.x+p[0][1]*v.y+p[0][2]*v.z,
            p[1][0]*v.x+p[1][1]*v.y+p[1][2]*v.z,
            p[2][0]*v.x+p[2][1]*v.y+p[2][2]*v.z};
}
inline bool finite(const Pose& p) {
    for (const auto& row:p) for (float value:row) if (!std::isfinite(value)) return false;
    return true;
}
inline constexpr std::array<unsigned,11> kJoints{1,2,3,7,8,12,13,18,19,23,24};
// Continuous capsule sleeves: bone length, local Y/Z radii including a soft
// outer margin. No persistent geometry or material pointers from Link's model.
inline constexpr std::array<Vec3,11> kSurfaces{{
    {20,24,20},{18,26,20},{10,14,14},
    {28,15,15},{26,14,14},{28,15,15},{26,14,14},
    {35,17,17},{40,15,15},{35,17,17},{40,15,15},
}};
inline constexpr unsigned kSides=16, kRings=8;
inline constexpr unsigned kVertexCount=kJoints.size()*(kRings-1)*kSides*6;
struct Vertex { Vec3 world, field; float opacity; };

struct Motion {
    Vec3 previous{}, lag{};
    bool ready=false;
    void reset() { *this={}; }
    void update(Vec3 root, float dt) {
        const auto delta=root-previous;
        // Never stretch a veil across warps, resumed scenes or reused actors.
        if (!ready || dt<=0 || dt>0.25f || length(delta)>120) {
            lag={}; ready=true;
        } else {
            const auto target=limited(delta*(-0.045f/dt),12.0f);
            const float blend=1-std::exp(-dt/0.09f);
            lag=lag+(target-lag)*blend;
        }
        previous=root;
    }
};

inline void build(const std::array<Pose,kJoints.size()>& poses, const Pose& root,
                  Vec3 eye, Vec3 lag, float seconds, float fade, std::vector<Vertex>& out) {
    out.clear();
    if (!finite(root) || !std::isfinite(seconds)) return;
    for (const auto& pose:poses) if (!finite(pose)) return;
    out.reserve(kVertexCount);
    constexpr float pi=3.14159265358979323846f;
    // Two hemispherical caps joined by a straight sleeve along the limb.
    constexpr std::array<float,kRings> latitude{-pi/2,-pi/3,-pi/6,0,0,pi/6,pi/3,pi/2};
    const auto rootPos=origin(root);
    const auto right=normalized(rotate(root,{1,0,0}));
    const auto forward=normalized(rotate(root,{0,0,1}));
    for (unsigned part=0;part<poses.size();++part) {
        std::array<std::array<Vertex,kSides+1>,kRings> rings{};
        const auto& pose=poses[part];
        const auto dim=kSurfaces[part];
        for (unsigned r=0;r<kRings;++r) {
            const float cap=std::cos(latitude[r]);
            const float axial=std::sin(latitude[r]);
            for (unsigned side=0;side<kSides;++side) {
                const float angle=2*pi*side/kSides;
                const float y=std::cos(angle),z=std::sin(angle);
                const Vec3 local{(r<4 ? 0:dim.x)+axial*std::min(dim.y,dim.z),cap*dim.y*y,cap*dim.z*z};
                const auto normal=normalized(rotate(pose,{axial,cap*y,cap*z}));
                const float lift=std::max(0.0f,normal.y);
                const auto base=origin(pose)+rotate(pose,local);
                // Attached bases and a short, bounded, gently billowing upper
                // edge. Only the upper-facing region receives movement lag.
                const float wave=std::sin(seconds*2.1f+dot(base-rootPos,{0.07f,0.03f,0.09f}));
                const auto position=base+Vec3{0,lift*(12+3*wave),0}+lag*(lift*lift);
                const auto relative=position-rootPos;
                // One body-local 3D field across all sleeves: no repeated sprite
                // or angular UV seam. Its Y coordinate scrolls upward at 8 units/s.
                const Vec3 field{dot(relative,right)*0.075f+0.22f*std::sin(seconds*0.43f+relative.y*0.04f),
                    (relative.y-seconds*8)*0.075f,
                    dot(relative,forward)*0.075f+0.18f*std::cos(seconds*0.31f+dot(relative,right)*0.05f)};
                const float facing=std::abs(dot(normal,normalized(eye-position)));
                // Soften the silhouette instead of exposing the capsule contour.
                const float edge=std::clamp(facing/0.28f,0.0f,1.0f);
                const float opacity=fade*edge*edge*(3-2*edge)*(0.45f+0.55f*(1-facing))*(1-0.45f*lift);
                rings[r][side]={position,field,opacity};
            }
            rings[r][kSides]=rings[r][0];
        }
        for (unsigned r=0;r+1<kRings;++r) for (unsigned s=0;s<kSides;++s) {
            const auto& a=rings[r][s]; const auto& b=rings[r+1][s];
            const auto& c=rings[r][s+1]; const auto& d=rings[r+1][s+1];
            out.insert(out.end(),{a,c,b,c,d,b});
        }
    }
}
}

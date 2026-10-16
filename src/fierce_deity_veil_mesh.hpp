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
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
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
// Strand roots are spread around each bone, close to the body surface.
// The spaces between strands contain no geometry, rather than a faint shell.
inline constexpr std::array<Vec3,11> kSurfaces{{
    {20,20,16},{18,22,16},{10,12,12},
    {28,11,11},{26,10,10},{28,11,11},{26,10,10},
    {35,13,13},{40,11,11},{35,13,13},{40,11,11},
}};
inline constexpr unsigned kStrands=4, kSegments=24;
inline constexpr unsigned kVertexCount=kJoints.size()*kStrands*kSegments*6;
inline constexpr float kRiseSpeed=20.0f;
struct Vertex { Vec3 world, field; float opacity; };

// Features at height h reappear at h + speed*dt after dt: the bends themselves
// travel upward, not just a low-contrast texture on otherwise static geometry.
inline Vec3 curl(float height, float seconds, float seed) {
    const float flow=height-kRiseSpeed*seconds;
    const float phase=seed*2.39996323f;
    return {std::sin(flow*0.18f+phase)+0.28f*std::sin(flow*0.37f+phase*1.7f),0,
            std::cos(flow*0.16f+phase*1.13f)+0.25f*std::sin(flow*0.31f+phase*0.7f)};
}
inline float smooth(float a,float b,float value) {
    const float t=std::clamp((value-a)/(b-a),0.0f,1.0f);
    return t*t*(3-2*t);
}

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
    for (unsigned part=0;part<poses.size();++part) {
        const auto& pose=poses[part];
        const auto dim=kSurfaces[part];
        for (unsigned strand=0;strand<kStrands;++strand) {
            const unsigned index=part*kStrands+strand;
            const float seed=float(index+1);
            const float variation=float((index*17+11)%29)/28;
            const float angle=2*pi*strand/kStrands+part*2.39996323f;
            const Vec3 local{dim.x*(0.18f+0.22f*((strand+part)%4)),
                             dim.y*std::cos(angle),dim.z*std::sin(angle)};
            const auto anchor=origin(pose)+rotate(pose,local);
            const auto radial=rotate(pose,{0,std::cos(angle),std::sin(angle)});
            const auto outward=limited({radial.x,0,radial.z},1);
            const float height=(part==2 ? 58.0f : part<2 ? 46.0f : 32.0f)+variation*16;
            const float halfWidth=1.0f+variation*0.8f;
            std::array<Vec3,kSegments+1> centers{};
            std::array<std::array<Vertex,2>,kSegments+1> strip{};
            for (unsigned r=0;r<=kSegments;++r) {
                const float t=float(r)/kSegments;
                const float h=height*t;
                const float growth=smooth(0,0.65f,t);
                centers[r]=anchor+Vec3{0,h,0}+
                    curl(h,seconds,seed)*((2.0f+variation*3)*growth)+
                    outward*(t*t*5)+lag*(t*t);
            }
            for (unsigned r=0;r<=kSegments;++r) {
                const float t=float(r)/kSegments;
                const auto tangent=normalized(centers[std::min(r+1,kSegments)]-centers[r ? r-1:0]);
                // A ribbon follows a real 3D curve, while its narrow cross-section
                // faces the camera so it remains visible from front and side.
                auto across=cross(tangent,normalized(eye-centers[r]));
                if (length(across)<0.001f) across=cross(tangent,{1,0,0});
                if (length(across)<0.001f) across=cross(tangent,{0,0,1});
                across=normalized(across);
                const float width=halfWidth*(0.65f+0.35f*std::sin(pi*t))*std::pow(1-t,0.65f);
                const float opacity=fade*smooth(0,0.08f,t)*(1-smooth(0.6f,1,t));
                const float flow=(height*t-kRiseSpeed*seconds)*0.13f;
                for (unsigned side=0;side<2;++side) {
                    const float u=side ? 1.0f:-1.0f;
                    strip[r][side]={centers[r]+across*(u*width),{u,flow,seed},opacity};
                }
            }
            // Each strand is an independent strip. Never connect neighbouring
            // roots or limbs into a translucent enclosing surface.
            for (unsigned r=0;r<kSegments;++r)
                out.insert(out.end(),{strip[r][0],strip[r][1],strip[r+1][0],
                                      strip[r][1],strip[r+1][1],strip[r+1][0]});
        }
    }
}
}

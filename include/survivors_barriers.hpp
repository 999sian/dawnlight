#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <vector>

namespace dawnlight::survivors::barriers {
struct Point { float x, y, z; };
struct Wall { Point a, b; };
// Weld coincident mesh vertices: room meshes can split vertex indices at material
// seams. Only the seam between playable floor and an authored exit becomes a
// curtain. Open edges, cliffs and ordinary room/material seams remain unblocked.
class Boundary {
    using Vertex = std::array<long, 3>;
    using Key = std::array<Vertex, 2>;
    struct Edge { Wall wall; unsigned floor = 0, exit = 0; };
    std::map<Key, Edge> edges;
    static Vertex vertex(Point p) {
        return {std::lround(p.x * 2), std::lround(p.y * 2), std::lround(p.z * 2)};
    }
public:
    void triangle(const std::array<Point, 3>& p, bool walkable, bool exit) {
        for (unsigned i = 0; i < 3; ++i) {
            Key key{vertex(p[i]), vertex(p[(i + 1) % 3])};
            if (key[1] < key[0]) std::swap(key[0], key[1]);
            auto& edge = edges[key];
            edge.wall = {p[i], p[(i + 1) % 3]};
            edge.floor += walkable && !exit;
            edge.exit += exit;
        }
    }
    std::vector<Wall> walls() const {
        std::vector<Wall> result;
        for (const auto& entry : edges) {
            const auto& e = entry.second;
            if (!e.floor || !e.exit) continue;
            const float dx = e.wall.a.x - e.wall.b.x, dz = e.wall.a.z - e.wall.b.z;
            if (dx * dx + dz * dz > 1) result.push_back(e.wall);
        }
        return result;
    }
};
inline float distance_squared(Point p, const Wall& w) {
    const float x = w.b.x - w.a.x, z = w.b.z - w.a.z;
    const float length = x*x + z*z;
    const float t = length > 0 ? std::clamp(((p.x-w.a.x)*x+(p.z-w.a.z)*z)/length, 0.f, 1.f) : 0;
    const float dx = p.x-w.a.x-t*x, dz = p.z-w.a.z-t*z;
    return dx*dx+dz*dz;
}
// Continuous circle/segment collision, including end caps. Slide the remaining
// input along the wall instead of cancelling movement or changing Link's speed.
// Sweeping the full movement also catches rolls, lunges and high-speed knockback.
inline Point slide(Point start, Point end, float radius, const std::vector<Wall>& walls) {
    Point p = start;
    float dx = end.x-start.x, dz = end.z-start.z;
    for (unsigned iteration = 0; iteration < 4 && dx*dx+dz*dz > 0.0001f; ++iteration) {
        float hit = 1, nx = 0, nz = 0;
        const auto candidate = [&](float t, float x, float z) {
            if (t >= 0 && t < hit && dx*x+dz*z < -0.00001f) { hit=t; nx=x; nz=z; }
        };
        for (const auto& w : walls) {
            if (end.y < std::min(w.a.y,w.b.y)-200 || start.y > std::max(w.a.y,w.b.y)+1500) continue;
            if (std::max(p.x,p.x+dx)+radius<std::min(w.a.x,w.b.x) ||
                std::min(p.x,p.x+dx)-radius>std::max(w.a.x,w.b.x) ||
                std::max(p.z,p.z+dz)+radius<std::min(w.a.z,w.b.z) ||
                std::min(p.z,p.z+dz)-radius>std::max(w.a.z,w.b.z)) continue;
            const float ex=w.b.x-w.a.x, ez=w.b.z-w.a.z;
            const float length=std::hypot(ex,ez);
            if (length < 0.01f) continue;
            const float ux=ex/length, uz=ez/length, vx=-uz, vz=ux;
            const float distance=(p.x-w.a.x)*vx+(p.z-w.a.z)*vz, delta=dx*vx+dz*vz;
            const float projection=std::clamp((p.x-w.a.x)*ux+(p.z-w.a.z)*uz,0.f,length);
            const float ox=p.x-w.a.x-projection*ux, oz=p.z-w.a.z-projection*uz;
            const float overlap=std::hypot(ox,oz);
            if (overlap<radius) {
                if (overlap>0.0001f) candidate(0,ox/overlap,oz/overlap);
                else { const float side=delta>0 ? -1.f : 1.f; candidate(0,side*vx,side*vz); }
            }
            if (std::abs(delta)>0.00001f) for (float side : {-1.f,1.f}) {
                const float t=(side*radius-distance)/delta;
                const float along=(p.x+t*dx-w.a.x)*ux+(p.z+t*dz-w.a.z)*uz;
                if (along>=0 && along<=length) candidate(t,side*vx,side*vz);
            }
            for (const auto& cap : {w.a,w.b}) {
                const float x=p.x-cap.x,z=p.z-cap.z;
                const float a=dx*dx+dz*dz,b=x*dx+z*dz,c=x*x+z*z-radius*radius;
                const float discriminant=b*b-a*c;
                if (discriminant<0) continue;
                const float t=(-b-std::sqrt(discriminant))/a;
                candidate(t,(x+t*dx)/radius,(z+t*dz)/radius);
            }
        }
        const float travel = std::max(0.f,hit-0.0001f);
        p.x+=dx*travel; p.z+=dz*travel;
        if (hit==1) { p.x+=dx*(1-travel); p.z+=dz*(1-travel); break; }
        dx*=1-hit; dz*=1-hit;
        const float into=std::min(0.f,dx*nx+dz*nz);
        dx-=into*nx; dz-=into*nz;
    }
    p.y=end.y;
    return p;
}
} // namespace dawnlight::survivors::barriers

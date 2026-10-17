#include "../include/survivors_barriers.hpp"
#include "../include/survivors_maps.hpp"
#include <cassert>
#include <iostream>
using namespace dawnlight::survivors;
using namespace dawnlight::survivors::barriers;
bool near(float a,float b) { return std::abs(a-b)<0.1f; }
int main() {
    for (unsigned i=0;i<arenas.size();++i) {
        assert(arena_matches(i,arenas[i].stage,arenas[i].room));
        assert(!arena_matches(i,"F_SP102",arenas[i].room));
        assert(!arena_matches(i,arenas[i].stage,arenas[i].room+1));
    }
    assert(!arena_matches(99,"F_SP121",6));
    // Two floor triangles: internal diagonal/material seam is not a wall.
    const Point a{0,0,0},b{100,0,0},c{100,0,100},d{0,0,100};
    Boundary mesh;
    mesh.triangle({a,b,c},true,false);
    mesh.triangle({a,c,d},true,false);
    assert(mesh.walls().size()==4);
    // A native solid wall removes the need for an additional curtain.
    mesh.triangle({a,b,Point{100,200,0}},false,false);
    assert(mesh.walls().size()==3);
    // An exit floor adjacent to playable ground produces a barrier at the seam.
    mesh.triangle({b,c,Point{200,0,100}},true,true);
    assert(mesh.walls().size()==3);
    // A slightly split vertex is welded; it cannot create a false interior wall.
    Boundary split;
    split.triangle({a,b,c},true,false);
    split.triangle({Point{0.1f,0,0},Point{100.1f,0,100},d},true,false);
    assert(split.walls().size()==4);
    const std::vector<Wall> walls{{{0,0,-1000},{0,0,1000}}};
    for (float speed:{20.f,100.f,2000.f}) {
        const auto p=slide({-100,0,0},{speed,0,100},40,walls);
        assert(p.x<=-39.9f && near(p.z,100)); // full tangential movement retained
        const auto reverse=slide({100,0,0},{-speed,0,-100},40,walls);
        assert(reverse.x>=39.9f && near(reverse.z,-100));
    }
    auto p=slide({-10,0,0},{100,0,30},40,walls); // already within collision skin
    assert(near(p.x,-10) && near(p.z,30));
    p=slide({-10,0,0},{-100,0,30},40,walls); // can retreat from the skin
    assert(near(p.x,-100) && near(p.z,30));
    p=slide({-100,0,0},{-50,0,100},40,walls); // no collision, exact movement
    assert(near(p.x,-50) && near(p.z,100));
    p=slide({-100,1800,0},{100,1800,100},40,walls); // separated upper floor
    assert(near(p.x,100) && near(p.z,100));
    p=slide({-100,0,1039},{100,0,1039},40,walls); // end cap, not just a plane
    assert(p.x<100);
    p=slide({-100,0,1041},{100,0,1041},40,walls);
    assert(near(p.x,100));
    const std::vector<Wall> corner{{{0,0,-1000},{0,0,1000}},{{-1000,0,0},{1000,0,0}}};
    p=slide({-100,0,-100},{1000,0,1000},40,corner);
    assert(p.x<=-39.9f && p.z<=-39.9f);
    std::cout<<"Arena routing, mesh seams, exit edges and swept barrier collision passed\n";
}

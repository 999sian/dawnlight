#include "../src/fierce_deity_veil_mesh.hpp"
#include <cassert>
#include <limits>
#include <iostream>
using namespace dawnlight::dark_veil;

Pose identity(Vec3 position={}) {
    return {{{1,0,0,position.x},{0,1,0,position.y},{0,0,1,position.z}}};
}
int main() {
    std::array<Pose,kJoints.size()> pose;
    for (unsigned i=0;i<pose.size();++i) pose[i]=identity({float(i*10),float(i*15),0});
    const auto root=identity();
    std::vector<Vertex> a,b;
    build(pose,root,{0,70,300},{},1,1,a);
    assert(a.size()==kVertexCount);
    for (const auto& v:a) {
        assert(std::isfinite(v.world.x)&&std::isfinite(v.world.y)&&std::isfinite(v.world.z));
        assert(v.opacity>=0&&v.opacity<=1);
    }
    // Translating the actor + camera leaves the body-local field unchanged.
    const Vec3 translation{700,0,-800};
    auto moved=pose;
    for (auto& p:moved) { p[0][3]+=translation.x; p[2][3]+=translation.z; }
    build(moved,identity(translation),Vec3{0,70,300}+translation,{},1,1,b);
    assert(a.size()==b.size());
    for (unsigned i=0;i<a.size();++i) {
        assert(length(b[i].world-a[i].world-translation)<0.001);
        assert(length(b[i].field-a[i].field)<0.001);
    }
    // A single independently animated limb cannot move any other sleeve.
    moved=pose; moved[4][0][3]+=35;
    build(moved,root,{0,70,300},{},1,1,b);
    const unsigned perPart=kVertexCount/kJoints.size();
    for (unsigned i=0;i<a.size();++i)
        if (i/perPart!=4) assert(length(a[i].world-b[i].world)==0);
    assert(length(a[4*perPart].world-b[4*perPart].world)>30);
    // Separate narrow strips, with shared segment edges and tapered ends.
    for (unsigned strand=0;strand<kJoints.size()*kStrands;++strand) {
        const auto start=strand*kSegments*6;
        assert(a[start].opacity==0);
        assert(a[start+(kSegments-1)*6+4].opacity==0);
        for (unsigned r=0;r<kSegments;++r) {
            const auto i=start+r*6;
            assert(length(a[i].world-a[i+1].world)<=3.601f);
            assert(a[i].field.x==-1 && a[i+1].field.x==1);
            if (r+1<kSegments) assert(length(a[i+2].world-a[i+6].world)==0);
        }
    }
    // Root centers stay attached while upper curve geometry visibly moves.
    build(pose,root,{0,70,300},{},1.25f,1,b);
    unsigned moving=0;
    for (unsigned strand=0;strand<kJoints.size()*kStrands;++strand) {
        const auto first=strand*kSegments*6;
        const auto anchorA=(a[first].world+a[first+1].world)*0.5f;
        const auto anchorB=(b[first].world+b[first+1].world)*0.5f;
        assert(length(anchorA-anchorB)<0.0001f);
        const auto i=first+(kSegments/2)*6;
        const auto centerA=(a[i].world+a[i+1].world)*0.5f;
        const auto centerB=(b[i].world+b[i+1].world)*0.5f;
        if (length(centerA-centerB)>0.1f) ++moving;
    }
    assert(moving>kJoints.size()*kStrands*0.9f);
    for (float seed:{1.0f,9.0f,22.0f})
        assert(length(curl(17,1,seed)-curl(17+kRiseSpeed*0.25f,1.25f,seed))<0.0001f);
    // Side and overhead views must not collapse strips or create NaNs.
    for (Vec3 eye:{Vec3{300,70,0},Vec3{0,400,0}}) {
        build(pose,root,eye,{},1,1,b);
        assert(b.size()==kVertexCount);
        for (const auto& v:b) assert(std::isfinite(v.world.x)&&std::isfinite(v.world.y)&&std::isfinite(v.world.z));
    }
    build(pose,root,{0,70,300},{},1,0,b);
    for (const auto& v:b) assert(v.opacity==0);
    moved=pose; moved[0][0][0]=std::numeric_limits<float>::quiet_NaN();
    build(moved,root,{0,70,300},{},1,1,b); assert(b.empty());
    Motion motion;
    motion.update({},1.0f/60);
    for (unsigned i=1;i<=120;++i) {
        motion.update({float(i*2),0,0},1.0f/60);
        assert(length(motion.lag)<=12.001f&&motion.lag.x<=0);
    }
    assert(motion.lag.x<-1);
    motion.update({10000,0,0},1.0f/60); assert(length(motion.lag)==0);
    motion.update({10001,0,0},2); assert(length(motion.lag)==0);
    motion.reset(); assert(!motion.ready&&length(motion.lag)==0);
    // Time-based relaxation converges similarly at different display rates.
    auto simulate=[](unsigned fps) {
        Motion m; m.update({},1.0f/fps);
        for (unsigned f=1;f<=fps;++f) m.update({60.0f*f/fps,0,0},1.0f/fps);
        return m.lag;
    };
    assert(length(simulate(30)-simulate(144))<0.01);
    std::cout<<"Dark veil: bounded mesh, separate thin strips, moving curves, upward propagation, body attachment, independent limbs, fade, invalid poses and motion reset passed\n";
}

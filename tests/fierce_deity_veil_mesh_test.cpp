#include "../src/fierce_deity_veil_mesh.hpp"
#include <cassert>
#include <limits>
#include <iostream>
using namespace dawnlight::dark_veil;

Pose identity(Vec3 position={}) {
    return {{{1,0,0,position.x},{0,1,0,position.y},{0,0,1,position.z}}};
}
float time_at(unsigned strand,float age,unsigned generation=1) {
    return lifecycle(strand,0).duration*(generation+age-variation(strand,7));
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
    for (unsigned i=0;i<a.size();++i) {
        assert(length(b[i].world-a[i].world-translation)<0.001);
        assert(length(b[i].field-a[i].field)<0.001);
    }
    // Independently animated limbs do not displace unrelated strands.
    moved=pose; moved[4][0][3]+=35;
    build(moved,root,{0,70,300},{},1,1,b);
    const unsigned perPart=kVertexCount/kJoints.size();
    for (unsigned i=0;i<a.size();++i)
        if (i/perPart!=4) assert(length(a[i].world-b[i].world)==0);
    assert(length(a[4*perPart].world-b[4*perPart].world)>30);
    float minWidth=100,maxWidth=0,minHeight=1000,maxHeight=0,minAlpha=1,maxAlpha=0;
    for (unsigned i=0;i<kJoints.size()*kStrands;++i) {
        const auto first=i*kSegments*6, mid=first+8*6, last=first+(kSegments-1)*6+4;
        const auto center=(a[first].world+a[first+1].world)*0.5f;
        assert(length(center-anchor(pose[i/kStrands],i/kStrands,i%kStrands,1))<0.0001f);
        const auto local=local_anchor(i/kStrands,i%kStrands,1);
        assert(std::hypot(local.y,local.z)<=10.001f); // buried roots
        const auto life=lifecycle(i,1);
        if(life.age>0.14f && life.age<0.62f) assert(a[first].opacity>=0.35f); // no base gap
        assert(a[last].opacity==0);
        for (unsigned r=0;r<kSegments;++r) {
            const auto v=first+r*6;
            assert(length(a[v].world-a[v+1].world)<=54.401f);
            assert(a[v].field.x==-1 && a[v+1].field.x==1);
            if (r+1<kSegments) assert(length(a[v+2].world-a[v+6].world)==0);
        }
        const float width=length(a[mid].world-a[mid+1].world);
        const float height=a[last].world.y-center.y;
        minWidth=std::min(minWidth,width); maxWidth=std::max(maxWidth,width);
        minHeight=std::min(minHeight,height); maxHeight=std::max(maxHeight,height);
        if(life.age>0.14f && life.age<0.62f) {
            minAlpha=std::min(minAlpha,a[first].opacity); maxAlpha=std::max(maxAlpha,a[first].opacity);
        }
    }
    assert(maxWidth/minWidth>3 && maxHeight/minHeight>3 && maxAlpha/minAlpha>2);
    build(pose,root,{0,70,300},{},1.25f,1,b);
    unsigned moving=0;
    for (unsigned i=0;i<kJoints.size()*kStrands;++i) {
        const auto first=i*kSegments*6;
        if(length((a[first].world+a[first+1].world-b[first].world-b[first+1].world)*0.5f)>0.1f) ++moving;
    }
    assert(moving>kJoints.size()*kStrands*0.9f); // roots wander even on a stationary pose
    for (float seed:{1.0f,9.0f,22.0f})
        assert(length(curl(17,1,seed)-curl(17+kRiseSpeed*0.25f,1.25f,seed))<0.0001f);
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
    const float attached=time_at(0,0.3f);
    motion.update(pose,{},1.0f/60,attached);
    for (unsigned f=1;f<=120;++f) {
        moved=pose;
        for (auto& p:moved) p[0][3]+=f*2;
        motion.update(moved,{float(f*2),0,0},1.0f/60,attached);
        assert(length(motion.lag[0])<=512.001f && motion.lag[0].x<0);
    }
    assert(motion.lag[0].x<-5); // visible run follow-through
    const auto lagBeforeStop=motion.lag[0];
    motion.update(moved,{240,0,0},1.0f/60,attached);
    assert(motion.lag[0].x<0 && length(motion.lag[0])<length(lagBeforeStop));
    motion.update(moved,{10000,0,0},1.0f/60,attached);
    for (auto lag:motion.lag) assert(length(lag)==0);
    motion.update(pose,{10001,0,0},2,attached);
    for (auto lag:motion.lag) assert(length(lag)==0);
    motion.reset(); assert(!motion.ready);
    // Rotating a limb in place produces local lag without actor translation.
    const float limbTime=time_at(4*kStrands,0.3f);
    motion.update(pose,{},1.0f/60,limbTime);
    moved=pose;
    moved[4][0][0]=0; moved[4][0][1]=-1;
    moved[4][1][0]=1; moved[4][1][1]=0;
    motion.update(moved,{},1.0f/60,limbTime);
    for (unsigned i=0;i<motion.lag.size();++i)
        if(i/kStrands!=4) assert(length(motion.lag[i])==0);
    const auto delta=anchor(moved[4],4,0,limbTime)-anchor(pose[4],4,0,limbTime);
    assert(dot(motion.lag[4*kStrands],delta)<0);
    build(moved,root,{0,70,300},{},limbTime,1,a);
    build(moved,root,{0,70,300},motion,limbTime,1,b);
    const unsigned first=4*kStrands*kSegments*6, mid=first+12*6;
    assert(length((a[first].world+a[first+1].world-b[first].world-b[first+1].world)*0.5f)<0.0001f);
    assert(length((a[mid].world+a[mid+1].world-b[mid].world-b[mid+1].world)*0.5f)>1);
    auto simulate=[&](unsigned fps,float speed=60) {
        Motion m;
        const float start=time_at(0,0.1f);
        m.update(pose,{},1.0f/fps,start);
        for (unsigned f=1;f<=fps;++f) {
            auto running=pose;
            const float x=speed*f/fps;
            for (auto& p:running) p[0][3]+=x;
            m.update(running,{x,0,0},1.0f/fps,start+float(f)/fps);
        }
        return m.lag[0];
    };
    assert(length(simulate(30)-simulate(144))<0.01);
    const auto walking=simulate(60,180), running=simulate(60,540), sprinting=simulate(60,1620);
    assert(std::abs(running.x/walking.x-3)<0.04f);
    assert(std::abs(sprinting.x/running.x-3)<0.04f);
    assert(length(simulate(30,1620)-simulate(144,1620))<0.02f);
    // New strands inherit the current running trail instead of starting upright.
    Motion movingSmoke;
    const float birth=time_at(0,0.001f,2);
    const float begin=birth-1;
    auto sprintPose=pose;
    for(unsigned step=0;step<=60;++step) {
        const float x=step*10.0f;
        sprintPose=pose; for(auto& p:sprintPose) p[0][3]+=x;
        movingSmoke.update(sprintPose,{x,0,0},1.0f/60,begin+step/60.0f);
    }
    assert(!movingSmoke.released[0] && movingSmoke.lag[0].x<-30);
    // A released sprint wisp keeps its tilt even while Link turns back.
    movingSmoke.reset();
    const float detach=time_at(0,0.62f)+0.001f;
    for(unsigned step=0;step<=60;++step) {
        const float x=step*10.0f;
        sprintPose=pose; for(auto& p:sprintPose) p[0][3]+=x;
        movingSmoke.update(sprintPose,{x,0,0},1.0f/60,detach-1+step/60.0f);
    }
    assert(movingSmoke.released[0]);
    const auto tilt=movingSmoke.lag[0], drift=movingSmoke.velocity[0], releasedAt=movingSmoke.base[0];
    assert(tilt.x<-30 && drift.x>30);
    for(unsigned step=1;step<=30;++step) {
        const float x=600-step*10.0f;
        sprintPose=pose; for(auto& p:sprintPose) p[0][3]+=x;
        movingSmoke.update(sprintPose,{x,0,0},1.0f/60,detach+step/60.0f);
        assert(length(movingSmoke.lag[0]-tilt)<0.001f);
    }
    assert(length(movingSmoke.base[0]-releasedAt-drift*0.5f)<0.01f);
    // Release into world space, fade out, then respawn on the current body.
    motion.reset();
    float release=time_at(0,0.62f)+0.001f;
    motion.update(pose,{},0,release-0.02f);
    assert(!motion.released[0]);
    motion.update(pose,{},0.02f,release);
    assert(motion.released[0]);
    const auto releasedBase=motion.base[0], velocity=motion.velocity[0];
    moved=pose; for(auto& p:moved) p[0][3]+=30;
    motion.update(moved,{30,0,0},0.1f,release+0.1f);
    assert(length(motion.base[0]-releasedBase-velocity*0.1f)<0.001f);
    assert(motion.base[0].y>releasedBase.y);
    assert(length(motion.base[0]-anchor(moved[0],0,0,release+0.1f))>20);
    assert(life_opacity(0)==0 && life_opacity(1)==0 && life_opacity(0.99f)<0.01f);
    build(pose,root,{0,70,300},{},time_at(0,0.8f),1,a);
    assert(a[0].opacity==0 && a[12*6].opacity>0); // free wisps feather at both ends
    const float respawn=time_at(0,0.01f,2);
    for (float t=release+0.2f;t<respawn;t+=0.1f) motion.update(moved,{30,0,0},0.1f,t);
    motion.update(moved,{30,0,0},0.1f,respawn);
    assert(!motion.released[0] && motion.generation[0]==2);
    assert(length(motion.base[0]-anchor(moved[0],0,0,respawn))<0.001f);
    unsigned freeCount=0;
    for (bool free:motion.released) freeCount+=free;
    assert(freeCount>0 && freeCount<motion.released.size()); // staggered, never a shared flash
    motion.update(moved,{10000,0,0},0.1f,respawn+0.1f);
    for (unsigned i=0;i<motion.base.size();++i) {
        assert(length(motion.lag[i])==0);
        assert(length(motion.base[i]-anchor(moved[i/kStrands],i/kStrands,i%kStrands,respawn+0.1f))<0.001f);
    }
    // Wide ribbons must not fold into reverse-facing triangle fans as curves
    // rise or the camera moves. Check perspective-facing area for every face,
    // including strong vertical lag, close views and almost end-on views.
    for (Vec3 eye:{Vec3{0,70,300},Vec3{300,70,0},Vec3{20,300,30},Vec3{0,400,0},Vec3{5,10,15}}) {
        for (unsigned frame=0;frame<30;++frame) {
            const float time=frame*0.13f;
            Motion stress;
            stress.update(pose,{},0,time);
            for (auto& lag:stress.lag) lag=frame%2 ? Vec3{15,-20,12}:Vec3{-360,-20,120};
            build(pose,root,eye,stress,time,1,b);
            for (unsigned v=0;v<b.size();v+=3) {
                const auto p=b[v].world,q=b[v+1].world,r=b[v+2].world;
                assert(dot(cross(q-p,r-p),eye-p)>=-0.001f);
            }
        }
    }
    std::cout<<"Dark veil: varied strands, wandering roots, limb/run lag, lifecycle, warp reset and fold-free wide ribbons passed\n";
}

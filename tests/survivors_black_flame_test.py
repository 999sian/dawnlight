"""Scoped black-flame rendering preserves callbacks and native blend state."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fixture=r'''
#include <array>
#include <cassert>
struct JPABaseEmitter;
struct JPAEmitterCallBack{virtual ~JPAEmitterCallBack()=default;virtual void execute(JPABaseEmitter*){}virtual void executeAfter(JPABaseEmitter*){}virtual void draw(JPABaseEmitter*){}virtual void drawAfter(JPABaseEmitter*){}};
struct Shape{int getBlendMode()const{return 11;}int getBlendSrc()const{return 12;}int getBlendDst()const{return 13;}int getLogicOp()const{return 14;}};
struct Resource{Shape shape;Shape* getBsp(){return &shape;}}resource;
struct JPABaseEmitter{Resource* pRes=&resource;JPAEmitterCallBack* callback=nullptr;JPAEmitterCallBack* getEmitterCallBackPtr(){return callback;}void setEmitterCallBackPtr(JPAEmitterCallBack* c){callback=c;}};
constexpr int GX_BM_BLEND=1,GX_BL_SRCALPHA=2,GX_BL_INVSRCALPHA=3,GX_LO_CLEAR=4;
std::array<int,4> blend{};void GXSetBlendMode(int a,int b,int c,int d){blend={a,b,c,d};}
struct NativeCallback:JPAEmitterCallBack{unsigned calls=0;void execute(JPABaseEmitter*)override{++calls;}void executeAfter(JPABaseEmitter*)override{++calls;}void draw(JPABaseEmitter*)override{++calls;}void drawAfter(JPABaseEmitter*)override{++calls;assert((blend==std::array{11,12,13,14}));}};
'''
main=r'''
int main(){JPABaseEmitter emitter;NativeCallback native;emitter.callback=&native;BlackFlameCallback black;
 black.bind(&emitter);black.bind(&emitter);assert(black.original==&native);
 emitter.callback->execute(&emitter);emitter.callback->executeAfter(&emitter);emitter.callback->draw(&emitter);
 assert((blend==std::array{1,2,3,4}));emitter.callback->drawAfter(&emitter);assert(native.calls==4);
 black.unbind(&emitter);assert(emitter.callback==&native&&!black.original);
 black.unbind(&emitter);assert(emitter.callback==&native);
 emitter.callback=nullptr;black.bind(&emitter);black.draw(&emitter);black.drawAfter(&emitter);black.unbind(&emitter);assert(!emitter.callback);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test';cpp.write_text(fixture+(root/'src/survivors_black_flame.inc').read_text()+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Survivors black flames: scoped alpha blend, restoration and native callbacks passed')

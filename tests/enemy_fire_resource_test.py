"""Shared flame imports retain both users' resources and clean up owned emitters."""
from pathlib import Path
import os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/bow_fire_resource.cpp').read_text()
source=re.sub(r'^#include "[^"\n]+"\n','',source,flags=re.M)
particle=(root/'tests/particle_resource.cpp').read_text()
particle=particle[particle.index('using namespace dawnlight::particle_resource;'):particle.index('int main()')]
hook=(root/'src/enemy_spawner.cpp').read_text()
hook=hook[hook.index('HookAction before_dodongo_fire('):hook.index('ModResult install_test_hooks()')]
fixture=r'''
#include <algorithm>
#include <any>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>
#include "particle_resource.hpp"
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s32=int32_t;
struct BE16{u8 b[2];operator u16()const{return u16((b[0]<<8)|b[1]);}
 BE16& operator=(u16 n){b[0]=n>>8;b[1]=n;return *this;}};
#define BE(T) BE16
struct JKRHeap{static JKRHeap* getSystemHeap(){return nullptr;}};
unsigned destroyed=0;bool failHeap=false;
struct JKRExpHeap{
 void* memory;std::vector<std::function<void()>> disposers;
 static JKRExpHeap* create(void* m,u32,JKRHeap*,bool){return failHeap?nullptr:new JKRExpHeap{m,{}};}
 void* alloc(u32,int){return memory;}
 void destroy(){for(auto& f:disposers)f();++destroyed;delete this;}
};
struct JPAResource{u16 id=0;const BE16* mpTDB1=nullptr;u8 texNum=0;};
struct JPATexture{};
struct JPAResourceManager{
 JPAResource** pResAry=nullptr;JPATexture** pTexAry=nullptr;
 u16 resRegNum=0,resMaxNum=0,texRegNum=0,texMaxNum=0;
 JPAResourceManager()=default;
 JPAResourceManager(u8* data,JKRExpHeap* heap){
  using namespace dawnlight::particle_resource;
  auto* r=new JPAResource{read16(data+16),nullptr,data[22]};
  unsigned offset=24;for(unsigned i=0;i<read16(data+18);++i){
   if(!std::memcmp(data+offset,"TDB1",4))r->mpTDB1=reinterpret_cast<BE16*>(data+offset+8);
   offset+=read32(data+offset+4);
  }
  pResAry=new JPAResource*[1]{r};resRegNum=resMaxNum=1;
  texRegNum=texMaxNum=read16(data+10);pTexAry=new JPATexture*[texRegNum];
  for(unsigned i=0;i<texRegNum;++i)pTexAry[i]=new JPATexture;
  auto** resources=pResAry;auto** textures=pTexAry;const unsigned count=texRegNum;
  heap->disposers.push_back([=]{delete r;delete[] resources;for(unsigned i=0;i<count;++i)delete textures[i];delete[] textures;});
 }
 JPAResource* getResource(u16 id){for(unsigned i=0;i<resRegNum;++i)if(pResAry[i]->id==id)return pResAry[i];return nullptr;}
 bool checkUserIndexDuplication(u16 id){return getResource(id);}
};
struct Emitter{JPAResource* pRes;bool deleted=false;};
struct Node{Node* next=nullptr;Emitter* emitter=nullptr;Node* getNext(){return next;}Emitter* getObject(){return emitter;}};
struct List{Node* first=nullptr;Node* getFirst(){return first;}Node* getEnd(){return nullptr;}};
struct JPAEmitterManager{
 JPAResourceManager* common=nullptr;u8 gidMax=1;List pEmtrUseList[1];
 JPAResourceManager* getResourceManager(u8){return common;}
 void forceDeleteEmitter(Emitter* e){assert(destroyed==0);e->deleted=true;}
};
struct dPa_control_c{static inline JPAEmitterManager* mEmitterMng=nullptr;};
struct DVDDir{unsigned index=0;};struct DVDDirEntry{bool isDir=false;const char* name="fixture.jpc";int entryNum=0;};
struct DVDFileInfo{u32 length=0;int index=0;};
std::vector<std::vector<u8>> disc;unsigned scans=0,reads=0;
bool DVDOpenDir(const char*,DVDDir*){++scans;return true;}
bool DVDReadDir(DVDDir* d,DVDDirEntry* e){if(d->index==disc.size())return false;e->entryNum=d->index++;return true;}
bool DVDFastOpen(int n,DVDFileInfo* f){f->index=n;f->length=disc[n].size();return true;}
s32 DVDReadPrio(DVDFileInfo* f,u8* bytes,u32 size,int,int){assert(!(uintptr_t(bytes)%32)&&!(size%32));++reads;
 std::memcpy(bytes,disc[f->index].data(),f->length);return f->length;}
void DVDClose(DVDFileInfo*){}void DVDCloseDir(DVDDir*){}
struct ModContext{};ModContext* mod_ctx=nullptr;
struct Log{unsigned errors=0;void error(ModContext*,const char*){++errors;}}logger;auto* svc_log=&logger;
namespace dawnlight{void shutdown_bow_fire_resource();}
'''
hook_fixture=r'''
using namespace dawnlight;
enum HookAction{HOOK_CONTINUE};
namespace mods{
 template<class T>T arg(void* p,unsigned n){return std::any_cast<T>(static_cast<std::any*>(p)[n]);}
 template<class T>T& arg_ref(void* p,unsigned n){return std::any_cast<T&>(static_cast<std::any*>(p)[n]);}
}
struct fopAc_ac_c{int profile;};constexpr int fpcNm_E_DD_e=1,fpcNm_B_DS_e=3,fpcNm_E_VT_e=4;
int fopAcM_GetName(fopAc_ac_c* a){return a->profile;}
std::vector<std::pair<void*,bool>> s_testProcessStack;
bool enemy_spawner_process_active(){return !s_testProcessStack.empty()&&s_testProcessStack.back().second;}
'''
main=r'''
int main(){
 JPAResource original{0x7fff,nullptr,0};JPATexture texture;
 JPAResource* originalResources[]={&original};JPATexture* originalTextures[]={&texture};
 JPAResourceManager common;common.pResAry=originalResources;common.pTexAry=originalTextures;
 common.resRegNum=common.texRegNum=1;common.resMaxNum=common.texMaxNum=9;
 JPAEmitterManager manager;manager.common=&common;dPa_control_c::mEmitterMng=&manager;
 // Separate native banks; extraction keeps only each resource's referenced textures.
 for(u16 id:{0x8113,0x846c,0x846d,0x846e,0x846f,0x8470,0x8471,0x8472,0x89ac,0x89ad,0x89ae,0x89af,0x85f6,0x89b0,0x89b1,0x8848,0x8849,0x884a}){
  auto bytes=fixture();write16(bytes.data()+40,id);disc.push_back(bytes);
 }
 for(bool bowFirst:{true,false}){
  destroyed=0;scans=reads=0;
  u16 bow=bowFirst?bow_fire_effect():0;
  std::vector<u16> imported;
  for(u16 native=0x846c;native<=0x8472;++native){
   const unsigned previous=common.texRegNum;u16 id=dodongo_fire_effect(native);
   assert(id&&id<0x7fff);imported.push_back(id);
   auto* r=common.getResource(id);assert(r&&r->texNum==3);
   assert(u16(r->mpTDB1[0])==previous&&u16(r->mpTDB1[1])==previous+1&&u16(r->mpTDB1[2])==previous);
   assert(dodongo_fire_effect(native)==id); // Cached imports never offset twice.
  }
  if(!bowFirst)bow=bow_fire_effect();
  assert(bow&&bow_fire_effect()==bow&&scans==1&&reads==18);
  assert(common.resRegNum==9&&common.texRegNum==17&&common.getResource(0x7fff)==&original);
  std::sort(imported.begin(),imported.end());assert(std::adjacent_find(imported.begin(),imported.end())==imported.end());
  assert(std::find(imported.begin(),imported.end(),bow)==imported.end());
  assert(!dodongo_fire_effect(0x84c6)&&!dodongo_fire_effect(0x8113));
  // Rewrite only a standalone Dodongo's own keyed emitter requests.
  fopAc_ac_c actor{fpcNm_E_DD_e};std::any processArgs[]={0,static_cast<void*>(&actor)};
  std::any args[]={0,0,0,u16(0x846c)};
  before_dodongo_fire(nullptr,args,nullptr,nullptr);assert(std::any_cast<u16>(args[3])==0x846c);
  s_testProcessStack={{processArgs,true}};before_dodongo_fire(nullptr,args,nullptr,nullptr);
  assert(std::any_cast<u16>(args[3])==dodongo_fire_effect(0x846c));
  args[3]=u16(0x846c);s_testProcessStack.push_back({processArgs,false});before_dodongo_fire(nullptr,args,nullptr,nullptr);
  assert(std::any_cast<u16>(args[3])==0x846c);s_testProcessStack.pop_back();
  actor.profile=2;before_dodongo_fire(nullptr,args,nullptr,nullptr);assert(std::any_cast<u16>(args[3])==0x846c);
  for(bool stallord:{true,false})for(u16 native:stallord?std::vector<u16>{0x89ac,0x89ad,0x89ae,0x89af,0x85f6,0x89b0,0x89b1}:std::vector<u16>{0x8848,0x8849,0x884a}){
   actor.profile=stallord?fpcNm_B_DS_e:fpcNm_E_VT_e;args[3]=native;
   before_dodongo_fire(nullptr,args,nullptr,nullptr);
   const auto mapped=std::any_cast<u16>(args[3]);assert(mapped!=native&&mapped==boss_attack_effect(native,stallord));
   assert(common.getResource(mapped));assert(!boss_attack_effect(native,!stallord));
   args[3]=native;s_testProcessStack.push_back({processArgs,false});
   before_dodongo_fire(nullptr,args,nullptr,nullptr);assert(std::any_cast<u16>(args[3])==native);s_testProcessStack.pop_back();
  }
  assert(common.resRegNum==19&&common.texRegNum==37&&scans==1&&reads==18);
  s_testProcessStack.clear();
  // Both users' emitters are deleted before freeing heaps; foreign effects survive.
  Emitter a{common.getResource(bow)},b{common.getResource(imported[0])},foreign{&original};
  Node n3{nullptr,&foreign},n2{&n3,&b},n1{&n2,&a};manager.pEmtrUseList[0].first=&n1;
  shutdown_bow_fire_resource();assert(a.deleted&&b.deleted&&!foreign.deleted&&destroyed==18);
  assert(common.pResAry==originalResources&&common.pTexAry==originalTextures);
  assert(common.resRegNum==1&&common.texRegNum==1&&common.resMaxNum==9&&common.texMaxNum==9);
  manager.pEmtrUseList[0].first=nullptr;
 }
 // Missing/broken allocation fails once without altering the common bank.
 failHeap=true;unsigned failures=logger.errors;assert(!dodongo_fire_effect(0x846c));assert(!dodongo_fire_effect(0x846c));
 assert(logger.errors==failures+1&&common.pResAry==originalResources);shutdown_bow_fire_resource();failHeap=false;
 disc.clear();failures=logger.errors;assert(!bow_fire_effect()&&!bow_fire_effect());assert(logger.errors==failures+1);
 shutdown_bow_fire_resource();dPa_control_c::mEmitterMng=nullptr;assert(!dodongo_fire_effect(0x846c));
}
'''
with tempfile.TemporaryDirectory() as tmp:
 cpp=Path(tmp)/'test.cpp';exe=Path(tmp)/'test'
 cpp.write_text(fixture+particle+source+hook_fixture+hook+main)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Enemy fire: shared imports, texture indices, cache, actor scope, failure and unload passed')

"""Keep native Poe requests out of replacement processing in mixed rooms."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/cave_randomizer.cpp").read_text()
state = source[source.index("struct RoomLoad {"):source.index("std::unordered_set<fpc_ProcID> s_stalhounds;")]
callback = source[source.index("void after_request("):source.index("bool find_ground(")]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>
using s16=int16_t; using s8=int8_t; using u32=uint32_t;
using fpc_ProcID=unsigned;
constexpr unsigned fpcM_ERROR_PROCESS_ID_e=~0u;
constexpr s16 fpcNm_E_PO_e=1, fpcNm_E_WW_e=2, fpcNm_E_OC_e=3;
struct ModContext {};
struct cXyz {float x,y,z;};
struct fopAcM_prm_class {
    struct {u32 parameters; cXyz position;} base;
    s8 room_no;
    unsigned parent_id=fpcM_ERROR_PROCESS_ID_e;
};
using stdCreateFunc=void(*)();
namespace mods {
template<typename T> T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}
}
bool is_enemy_profile(s16 profile){
    // Native Poes have the ENEMY group, just like ordinary combat actors.
    return profile==fpcNm_E_PO_e || profile==fpcNm_E_WW_e || profile==fpcNm_E_OC_e;
}
'''
checks = r'''
int main(){
    fopAcM_prm_class append{};append.room_no=16;
    append.base.position={41,82,123};
    auto* ptr=&append;s16 profile=fpcNm_E_PO_e;unsigned id=1;
    stdCreateFunc create=nullptr;
    void* args[]={nullptr,&profile,&create,nullptr,&ptr};
    s_roomLoads.push_back({nullptr,append.room_no,true});
    // Exemption must not depend on a room number or any particular save flag.
    // Every original Poe parameter is left for native creation to interpret.
    for(int room=0;room<64;++room) for(u32 params:{0u,0xff231104u,0xffffffffu}){
        append.room_no=static_cast<s8>(room);s_roomLoads.back().room=room;
        append.base.parameters=params;
        after_request(nullptr,args,&id,nullptr);
        assert(s_pending.empty());
        assert(profile==fpcNm_E_PO_e && append.base.parameters==params);
        assert(append.base.position.x==41 && append.base.position.y==82 && append.base.position.z==123);
        assert(append.room_no==room && append.parent_id==fpcM_ERROR_PROCESS_ID_e);
    }
    // Adjacent ordinary enemy requests in the SAME room still enter the
    // normal placement/replacement pipeline, regardless of request order.
    profile=fpcNm_E_OC_e;after_request(nullptr,args,&id,nullptr);
    assert(s_pending.size()==1 && s_pending.at(id).profile==fpcNm_E_OC_e);
    assert(s_pending.at(id).room==append.room_no && s_pending.at(id).authored.y==82);
    profile=fpcNm_E_PO_e;++id;after_request(nullptr,args,&id,nullptr);
    assert(s_pending.size()==1 && !s_pending.contains(id));
    profile=fpcNm_E_OC_e;++id;after_request(nullptr,args,&id,nullptr);
    assert(s_pending.size()==2 && s_pending.contains(id));
    s_pending.clear();
    s_roomLoads.back().enabled=false;after_request(nullptr,args,&id,nullptr);
    assert(s_pending.empty());
    s_roomLoads.clear();after_request(nullptr,args,&id,nullptr);assert(s_pending.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/"poes.cpp", Path(tmp)/"poes"
    cpp.write_text(fixture + state + callback + checks)
    subprocess.run(["c++","-std=c++20","-Wall","-Wextra","-Werror",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Cave randomizer: native Poe requests preserved, other enemies in mixed rooms still randomized")

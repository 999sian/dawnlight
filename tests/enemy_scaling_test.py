"""Exercise the production HP hook, including Diababa's scripted side heads."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "src/enemy_scaling.cpp").read_text()
callbacks = source[source.index("s16 scale_health("):source.index("}  // namespace")]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
using s16=int16_t;
struct ModContext {};
constexpr int cPhs_COMPLEATE_e=4, fopAc_ENEMY_e=2;
constexpr s16 fpcNm_B_BH_e=1, fpcNm_B_BQ_e=2, fpcNm_E_OC_e=3;
struct fopAc_ac_c {int group; s16 name,health,field_0x560;};
s16 fopAcM_GetName(void* actor){return static_cast<fopAc_ac_c*>(actor)->name;}
namespace mods {
template<typename T> T arg(void* args,int i){return *static_cast<T*>(static_cast<void**>(args)[i]);}
}
int percent=100;bool automatic=false;unsigned ngplus=0;
int health_scale_percent(){return percent;}
bool automatic_ngplus_health_scaling(){return automatic;}
unsigned new_game_plus_count(){return ngplus;}
'''
checks = r'''
int main(){
    int result=cPhs_COMPLEATE_e;
    fopAc_ac_c actor{};void* actorPtr=&actor;void* args[]={&actorPtr};
    for(int hpPercent:{1,50,100,101,200,300,9999})
    for(bool ng:{false,true}) for(unsigned cycle:{0u,1u,9u,99u}){
        percent=hpPercent;automatic=ng;ngplus=cycle;
        // Neither stored health field may change, including native counters
        // other than the initial 1 HP. Do not force a replacement constant.
        for(s16 health:{s16(-1),s16(0),s16(1),s16(3)}){
            actor={fopAc_ENEMY_e,fpcNm_B_BH_e,health,7};
            on_actor_create_post(nullptr,args,&result,nullptr);
            assert(actor.health==health && actor.field_0x560==7);
        }
        // The main boss and ordinary enemies still receive the configured
        // scaling, including the capped automatic NG+ minimum.
        for(s16 profile:{fpcNm_B_BQ_e,fpcNm_E_OC_e}){
            actor={fopAc_ENEMY_e,profile,50,100};
            on_actor_create_post(nullptr,args,&result,nullptr);
            int effective=hpPercent;
            if(ng && cycle) effective=std::max(effective,200+10*int(std::min(cycle,9u)));
            assert(actor.health==std::min((50*effective+99)/100,32767));
            assert(actor.field_0x560==std::min(effective,32767));
        }
    }
    percent=200;automatic=false;
    actor={fopAc_ENEMY_e,fpcNm_E_OC_e,5,10};result=0;
    on_actor_create_post(nullptr,args,&result,nullptr);assert(actor.health==5 && actor.field_0x560==10);
    result=cPhs_COMPLEATE_e;actor.group=0;
    on_actor_create_post(nullptr,args,&result,nullptr);assert(actor.health==5 && actor.field_0x560==10);
    on_actor_create_post(nullptr,nullptr,&result,nullptr);
    on_actor_create_post(nullptr,args,nullptr,nullptr);
    actorPtr=nullptr;on_actor_create_post(nullptr,args,&result,nullptr);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp)/"scaling.cpp", Path(tmp)/"scaling"
    cpp.write_text(fixture + callbacks + checks)
    subprocess.run(["c++","-std=c++20","-Wall","-Wextra","-Werror",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Enemy HP scaling: Diababa heads preserved; ordinary/main-boss scaling, NG+ and creation guards passed")

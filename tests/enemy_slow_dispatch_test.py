"""Exercise production profile lookup and nested Execute routing without the host."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/enemy_slow_motion.cpp').read_text()

def function(signature):
    start=source.index(signature); body=source.index('{',start); depth=0
    for end in range(body,len(source)):
        depth+=(source[end]=='{')-(source[end]=='}')
        if depth==0:return source[start:end+1]
    raise AssertionError(signature)

fixture=r'''
#include <any>
#include <array>
#include <cassert>
struct ModContext {};
enum HookAction {HOOK_CONTINUE};
using process_method_func=int(*)(void*);
int execute(void*){return 1;}int destroy(void*){return 1;}
struct process_method_class {process_method_func execute_method=execute;} methods;
struct fopAc_ac_c {int name=0;bool actor=true;void* sub_method=&methods;};
unsigned probes=0;
int fopAcM_GetName(fopAc_ac_c* a){++probes;return a->name;}
bool is_actor_process(void* a){return static_cast<fopAc_ac_c*>(a)->actor;}
constexpr unsigned fpcNm_MAX_NUM=800;
struct EnemySlowProfile {int name;bool processExecute;};
const EnemySlowProfile first{2,true},direct{7,false},duplicate{2,false},last{799,true};
const std::array<const EnemySlowProfile*,4>s_profiles{&first,&direct,&duplicate,&last};
std::array<bool,64>s_processFrames{};std::size_t s_processDepth=0;
unsigned enters=0,exits=0;
HookAction begin_enemy_slow_execute(fopAc_ac_c* actor,const EnemySlowProfile* p){
    assert(actor->name==p->name&&p->processExecute);++enters;return HOOK_CONTINUE;
}
void after_enemy_slow_execute(ModContext*,void*,void*,void*){++exits;}
namespace mods {template<class T>T arg(void* a,int n){return std::any_cast<T>(static_cast<std::any*>(a)[n]);}}
'''
cases=r'''
int main(){
    assert(!find_profile(nullptr));
    fopAc_ac_c actor;
    for(int name=-1;name<=800;++name){
        actor.name=name;const EnemySlowProfile* expected=nullptr;
        for(auto* p:s_profiles)if(p->name==name){expected=p;break;}
        assert(find_profile(&actor)==expected);
    }
    actor.name=2;std::any args[]={process_method_func(destroy),static_cast<void*>(&actor)};
    probes=0;
    before_process_method(nullptr,args,nullptr,nullptr);
    after_process_method(nullptr,args,nullptr,nullptr);
    assert(!probes&&!enters&&!exits&&!s_processDepth);
    args[0]=process_method_func(execute);
    before_process_method(nullptr,args,nullptr,nullptr);
    assert(probes==1&&enters==1&&s_processDepth==1);
    fopAc_ac_c unrelated;std::any nested[]={process_method_func(execute),static_cast<void*>(&unrelated)};
    before_process_method(nullptr,nested,nullptr,nullptr);
    after_process_method(nullptr,nested,nullptr,nullptr);
    assert(enters==1&&!exits&&s_processDepth==1);
    after_process_method(nullptr,args,nullptr,nullptr);
    assert(exits==1&&!s_processDepth);
    // Profiles using dedicated hooks must not execute twice via the dispatcher.
    actor.name=7;before_process_method(nullptr,args,nullptr,nullptr);
    after_process_method(nullptr,args,nullptr,nullptr);assert(enters==1&&exits==1);
    actor.name=799;actor.actor=false;probes=0;
    before_process_method(nullptr,args,nullptr,nullptr);
    after_process_method(nullptr,args,nullptr,nullptr);assert(!probes);
    // Overflow remains balanced and does not enter unrecordable scopes.
    actor.actor=true;
    for(unsigned i=0;i<70;++i)before_process_method(nullptr,args,nullptr,nullptr);
    for(unsigned i=0;i<70;++i)after_process_method(nullptr,args,nullptr,nullptr);
    assert(enters==65&&exits==65&&!s_processDepth);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'test.cpp';binary=Path(tmp)/'test'
    cpp.write_text(fixture+function('const EnemySlowProfile* find_profile(')+
                   function('HookAction before_process_method(')+function('void after_process_method(')+cases)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Slow dispatch: all profile IDs, method filtering, direct-hook exclusion and nested scope balance passed')

"""Reject generic creation callbacks before reading actor-sized memory."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
# Engine boundary only: the production guard is included unchanged. Keep a
# PC-style vtable and an actor tag beyond the end of smaller request objects.
engine = r'''
#pragma once
#include <cstddef>
struct process_method_class { void* create; void* destroy; void* execute; };
struct leafdraw_method_class { process_method_class base; void* draw; };
struct actor_method_class { leafdraw_method_class base; };
inline leafdraw_method_class g_fpcLf_Method{};
inline actor_method_class g_fopAc_Method{};
inline int g_fpcBs_type=0x9130001;
struct base_process_class {
    int type=g_fpcBs_type;
    unsigned id=42;
    const process_method_class* methods=&g_fpcLf_Method.base;
    virtual ~base_process_class()=default;
};
struct leafdraw_class:base_process_class {
    const leafdraw_method_class* leaf_methods=&g_fopAc_Method.base;
};
struct fopAc_ac_c:leafdraw_class {
    std::byte padding[256]{};
    int actor_type=0x9130003;
    void* sub_method=nullptr;
};
inline unsigned actorProbes=0;
inline bool fopAcM_IsActor(void* p) {
    ++actorProbes;
    return static_cast<fopAc_ac_c*>(p)->actor_type==0x9130003;
}
'''
test = r'''
#include "process_actor.hpp"
#include <cassert>
#include <new>
#include <sys/mman.h>
#include <unistd.h>
using namespace dawnlight;
int main() {
    assert(!is_base_process(nullptr)&&!is_actor_process(nullptr));
    const auto page=static_cast<size_t>(sysconf(_SC_PAGESIZE));
    auto* memory=static_cast<char*>(mmap(nullptr,page*2,PROT_READ|PROT_WRITE,
                                       MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    assert(memory!=MAP_FAILED);
    assert(mprotect(memory+page,page,PROT_NONE)==0);
    // Creation, cancellation and request deletion pass a request, not an
    // actor. A protected next page makes the old deep tag probe crash reliably.
    for(size_t size:{size_t(72),size_t(104),size_t(144)}) {
        auto* request=memory+page-size;
        std::memset(request,0,size);
        for(unsigned callback=0;callback<3;++callback) {
            assert(!is_base_process(request)&&!is_actor_process(request));
        }
    }
    // Native scene/base and non-actor leaf objects must also short-circuit
    // before the actor probe, even though their base type tag is legitimate.
    auto* base=new(memory+page-sizeof(base_process_class)) base_process_class;
    base->methods=nullptr;
    assert(is_base_process(base)&&!is_actor_process(base));
    base->~base_process_class();
    auto* leaf=new(memory+page-sizeof(leafdraw_class)) leafdraw_class;
    leaf->leaf_methods=nullptr;
    assert(is_base_process(leaf)&&!is_actor_process(leaf));
    leaf->~leafdraw_class();
    assert(actorProbes==0);
    assert(munmap(memory,page*2)==0);
    // Heap bytes beyond a real request may coincidentally match actor_type;
    // that must never authorize dereferencing the bogus actor method pointer.
    fopAc_ac_c residue;
    residue.type=0;residue.sub_method=reinterpret_cast<void*>(0x149);
    assert(!is_actor_process(&residue)&&actorProbes==0);
    fopAc_ac_c actor;
    actor.actor_type=0;
    assert(is_base_process(&actor)); // Owned pre-create admission still works.
    assert(!is_actor_process(&actor));
    actor.actor_type=0x9130003;
    assert(is_actor_process(&actor)); // Execute and delete may inspect methods.
    const auto probes=actorProbes;
    actor.type=0;g_fpcBs_type=0;
    assert(!is_base_process(&actor)&&!is_actor_process(&actor));
    assert(actorProbes==probes); // Uninitialized engine tags never accept zeros.
}
'''
with tempfile.TemporaryDirectory() as tmp:
    folder = Path(tmp)
    (folder/'f_op').mkdir()
    (folder/'f_op/f_op_actor_mng.h').write_text(engine)
    cpp, binary = folder/'test.cpp', folder/'test'
    cpp.write_text('#include <initializer_list>\n' + test)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I', tmp, '-I', str(root/'src'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Process actor guard: protected-page requests, scene/leaf rejection and actor lifecycle passed')

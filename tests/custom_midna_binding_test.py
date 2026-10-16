"""Run Dawnlight's input hooks together with Dusklight's native Midna trigger."""
from pathlib import Path
import argparse
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--dusklight-dir', type=Path, required=True)
parser.add_argument('--baseline-ref')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = (root / 'src/item_slot_hooks.cpp').read_text()
baseline = subprocess.check_output(
    ['git', 'show', args.baseline_ref + ':src/item_slot_hooks.cpp'], cwd=root, text=True
) if args.baseline_ref else source
native = (args.dusklight_dir / 'src/d/actor/d_a_alink.cpp').read_text()


def function(text, signature):
    start = text.index(signature)
    return text[start:text.index('\n}', start) + 2]


fixture = r'''
#include <cassert>
#include <string_view>
using BOOL=int; using u32=unsigned int;
constexpr BOOL TRUE=1,FALSE=0;
constexpr int PAD_1=0, BTN_Z=4, MOD_OK=0, MOD_ERROR=1;
constexpr unsigned PAD_BUTTON_LEFT=1, PAD_TRIGGER_Z=0x10,
    PAD_NATIVE_BUTTON_INVALID=0xFFFFFFFF;
struct ModContext {};
enum HookAction {HOOK_CONTINUE,HOOK_SKIP_ORIGINAL};
namespace mods {template<class T>T arg(void* args,int){return *static_cast<T*>(args);}}
namespace dusk::config {
struct ConfigVarBase {};
template<class T>struct ConfigVar:ConfigVarBase {
    T value; T getValue()const{return value;}
};
}
dusk::config::ConfigVar<int> binding{{},-1};
bool settingPresent=true,resolveFails=false,nullAddress=false;
int resolves=0;
using ZSlotGetConfigVarFn=dusk::config::ConfigVarBase*(*)(std::string_view);
ZSlotGetConfigVarFn s_midnaGetConfigVar=nullptr;
dusk::config::ConfigVarBase* getVar(std::string_view key){
    assert(key=="actionBindings.callMidna_port0");
    return settingPresent?&binding:nullptr;
}
struct HookService {int(*resolve)(ModContext*,const char*,void**,void*);};
int resolve(ModContext*,const char* name,void** address,void*){
    assert(std::string_view(name)=="dusk::config::GetConfigVar");++resolves;
    *address=nullAddress?nullptr:reinterpret_cast<void*>(&getVar);
    return resolveFails?MOD_ERROR:MOD_OK;
}
HookService hook{resolve};HookService* svc_hook=&hook;ModContext* mod_ctx=nullptr;
namespace dusk {
enum class ActionBinds {CALL_MIDNA};
bool virtualAvailable=false,physicalPressed=false,virtualPressed=false;
bool isActionBound(ActionBinds,int port){assert(port==0);return binding.value!=-1||virtualAvailable;}
bool getActionBindTrig(ActionBinds,int port){assert(port==0);return physicalPressed||virtualPressed;}
}
struct daAlink_c {int mItemTrigger=0;BOOL midnaTalkTrigger()const;};
daAlink_c link;
bool zActive=true,touchActive=false,touchPressed=false,playerPresent=true,menu=false;
bool s_dpadLeftHeld=false,s_dpadLeftTrig=false,s_touchZItemHeld=false,s_touchZItemTrig=false;
bool s_touchRingCombineRequested=false;
bool z_item_slot_active(){return zActive;}
bool dawnlight_touch_ui_active(){return touchActive;}
bool consume_midna_touch_press(){bool value=touchPressed;touchPressed=false;return value;}
bool z_item_menu_or_pause_context(){return menu;}
int dMeter2Info_getWindowStatus(){return menu?2:0;}
daAlink_c* dComIfGp_getLinkPlayer(){return playerPresent?&link:nullptr;}
daAlink_c* daAlink_getAlinkActorClass(){return dComIfGp_getLinkPlayer();}
struct interface_of_controller_pad {u32 mButtonFlags=0,mPressedButtonFlags=0;}pad;
struct mDoCPd_c {static auto& getCpadInfo(int port){assert(port==0);return pad;}};
// FUNCTIONS
bool talk(){
    const auto* p=&link;BOOL result=FALSE;
    const auto action=before_midna_talk_trigger(nullptr,&p,&result,nullptr);
    return action==HOOK_CONTINUE?link.midnaTalkTrigger():result;
}
int main(){
    // A configured controller or keyboard bind reaches the original host
    // action instead of being replaced with a false D-pad result (issue #133).
    for(int button:{0,9,44}){
        binding.value=button;dusk::physicalPressed=true;
        assert(talk());
        dusk::physicalPressed=false;s_dpadLeftTrig=true;
        assert(!talk()); // custom binding takes priority over D-pad Left
        s_dpadLeftTrig=false;link.mItemTrigger=BTN_Z;
        assert(!talk()); // no vanilla Z fallback while a custom bind exists
        link.mItemTrigger=0;
    }
    // Native edge state is respected: held/released (no new trigger) is false.
    dusk::physicalPressed=true;assert(talk());
    dusk::physicalPressed=false;assert(!talk());
    // Custom bindings release D-pad Left for other native functions.
    pad={PAD_BUTTON_LEFT,PAD_BUTTON_LEFT};s_dpadLeftHeld=s_dpadLeftTrig=true;
    after_pad_read(nullptr,nullptr,nullptr,nullptr);
    assert(!s_dpadLeftHeld&&!s_dpadLeftTrig);
    assert(pad.mButtonFlags==PAD_BUTTON_LEFT&&pad.mPressedButtonFlags==PAD_BUTTON_LEFT);
    // Unbinding at runtime restores the fallback, even if the native virtual
    // Midna touch action is available. Do not confuse that with a user bind.
    binding.value=-1;dusk::virtualAvailable=true;
    after_pad_read(nullptr,nullptr,nullptr,nullptr);
    assert(s_dpadLeftHeld&&s_dpadLeftTrig&&talk());
    assert(pad.mButtonFlags==0&&pad.mPressedButtonFlags==0);
    s_dpadLeftTrig=false;link.mItemTrigger=BTN_Z;
    dusk::virtualPressed=true;assert(!talk());
    // A dedicated Midna touch tap works with and without a custom bind.
    touchActive=true;
    for(int button:{-1,9}){
        binding.value=button;s_touchZItemHeld=true;touchPressed=false;
        assert(!talk()); // item Touch-Z must not call the virtual Midna action
        touchPressed=true;assert(talk());assert(!talk());
    }
    s_touchZItemHeld=false;dusk::virtualPressed=false;dusk::virtualAvailable=false;
    // Menus/absent player clear fallback state without consuming D-pad input.
    binding.value=-1;
    for(bool inMenu:{false,true}){
        menu=inMenu;playerPresent=inMenu;pad={PAD_BUTTON_LEFT,PAD_BUTTON_LEFT};
        s_dpadLeftHeld=s_dpadLeftTrig=true;
        after_pad_read(nullptr,nullptr,nullptr,nullptr);
        assert(!s_dpadLeftHeld&&!s_dpadLeftTrig&&pad.mPressedButtonFlags==PAD_BUTTON_LEFT);
    }
    menu=false;playerPresent=true;
    // With Z Items disabled (including provider ownership), native behavior
    // and the separate touch shortcut continue without our D-pad remap.
    zActive=false;touchActive=false;binding.value=9;dusk::physicalPressed=true;
    assert(talk());dusk::physicalPressed=false;assert(!talk());
    binding.value=-1;assert(talk()); // original vanilla Z fallback is allowed here
    // Current settings are read every time; only the symbol is cached.
    assert(resolves==1);binding.value=12;assert(custom_midna_binding_configured());
    settingPresent=false;assert(!custom_midna_binding_configured());settingPresent=true;
    s_midnaGetConfigVar=nullptr;resolveFails=true;assert(!custom_midna_binding_configured());
    resolveFails=false;nullAddress=true;assert(!custom_midna_binding_configured());
    nullAddress=false;assert(custom_midna_binding_configured());
}
'''
fixture = fixture.replace('// FUNCTIONS', '\n'.join((
    function(source, 'bool custom_midna_binding_configured('),
    function(baseline, 'void after_pad_read('),
    function(baseline, 'HookAction before_midna_talk_trigger('),
    function(native, 'BOOL daAlink_c::midnaTalkTrigger() const'),
)))
with tempfile.TemporaryDirectory() as tmp:
    cpp, exe = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-DTARGET_PC=1', '-Wall', '-Wextra', '-Werror',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Midna binding passed: custom priority, native trigger, live unbind, D-pad fallback, touch/Z separation')

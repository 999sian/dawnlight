"""Exercise production item repair and Z oil HUD code with saved combo fixtures."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
integrity = (root / 'src/item_integrity.cpp').read_text()
items = (root / 'src/item_slot_hooks.cpp').read_text()


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


fixture = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <optional>
#include <string>
using u8 = unsigned char;
using f32 = float;
constexpr int SELECT_ITEM_X = 0, SELECT_ITEM_Y = 1;
constexpr int SELECT_ITEM_NUM = 2, kZItemSlot = 2, MAX_ITEM_SLOTS = 24;
constexpr u8 dItemNo_BOW_e = 1, dItemNo_KANTERA_e = 2, dItemNo_KANTERA2_e = 3,
    dItemNo_NORMAL_BOMB_e = 4, dItemNo_WATER_BOMB_e = 5, dItemNo_POKE_BOMB_e = 6,
    dItemNo_HAWK_EYE_e = 7, dItemNo_FISHING_ROD_1_e = 8, dItemNo_BEE_CHILD_e = 9,
    dItemNo_WORM_e = 10, dItemNo_ZORAS_JEWEL_e = 11, dItemNo_NONE_e = 255;
struct dSv_player_item_c { u8 mItems[MAX_ITEM_SLOTS]{}; } inventory;
std::array<u8, 3> selected{}, mixed{};
bool fixes = true, s_repairingMixPairs = false;
int repairs = 0;
bool item_integrity_fixes_enabled() { return fixes; }
u8 dComIfGs_getSelectItemIndex(int i) { return selected.at(i); }
u8 dComIfGs_getMixItemIndex(int i) { return mixed.at(i); }
void dComIfGs_setMixItemIndex(int i, u8 slot) { mixed.at(i) = slot; }
void dComIfGp_setSelectItem(int) { ++repairs; }
// INTEGRITY
u8 dComIfGs_getItem(u8 slot, bool) {
    assert(slot < MAX_ITEM_SLOTS);
    repair_mix_pairs(&inventory);
    return inventory.mItems[slot];
}
// OIL PREDICATE

// Minimal touch surface: verify the actual RML builder can display both meters.
namespace Rml { struct Element {}; }
Rml::Element container, button;
Rml::Element* s_zTouchMeterContainer = &container;
Rml::Element* s_zTouchMeterButton = &button;
std::string s_zTouchMeterRml, rendered;
void setRml(Rml::Element*, const std::string* rml) { rendered = *rml; }
struct RmlSetInnerRMLHook { inline static auto g_orig = &setRml; };
Rml::Element* child(Rml::Element*, int) { return &container; }
auto s_rmlGetChild = &child;
void property() {}
auto s_rmlSetProperty = &property;
void configure_z_touch_meter_container(Rml::Element*) {}
bool paused = false;
bool z_item_menu_or_pause_context() { return paused; }
void* dComIfGp_getLinkPlayer() { return &button; }
struct daPy_py_c { static int checkNowWolf() { return 0; } };
u8 resolvedItem = 0;
u8 resolved_select_item(int) { return resolvedItem; }
bool z_item_ammo_values(u8 item, u8& count, u8& max) {
    count = 30; max = 60; return item == dItemNo_BOW_e;
}
int oil = 50;
int dComIfGs_getMaxOil() { return 100; }
int dComIfGs_getOil() { return oil; }
// TOUCH HUD

namespace dusk::ui { enum class Control { X, Y, Z, B }; }
struct ModContext {};
namespace mods {
template<class T> T arg(void* args, int) { return *static_cast<T*>(args); }
}
u8 dComIfGp_getSelectItem(int slot) {
    return mixed[slot] == 4 ? dItemNo_BOW_e : inventory.mItems[selected[slot]];
}
// XY OIL

int main() {
    // Vanilla stores the partner in select and the Bow in mix. The Standalone
    // menu temporarily disguises the Lantern as Hawkeye before restoring it.
    inventory.mItems[4] = dItemNo_BOW_e;
    inventory.mItems[1] = dItemNo_KANTERA_e;
    for (int slot = 0; slot < 3; ++slot) {
        selected.fill(dItemNo_NONE_e); mixed.fill(dItemNo_NONE_e);
        selected[slot] = 1; mixed[slot] = 4;
        for (auto partner : {dItemNo_HAWK_EYE_e, dItemNo_KANTERA_e}) {
            inventory.mItems[1] = partner;
            repair_mix_pairs(&inventory);  // setItem post-hook
            (void)dComIfGs_getItem(1, false);  // getItem pre-hook
            assert(selected[slot] == 1 && mixed[slot] == 4);
        }
        assert(z_item_uses_oil(dItemNo_BOW_e) == (slot == kZItemSlot));
    }
    assert(repairs == 0);
    // A reversed pair is also preserved; bare Bow and unrelated mixes use no oil.
    assert(valid_mix_pair(&inventory, 4, 1));
    assert(z_item_uses_oil(dItemNo_KANTERA_e));
    assert(z_item_uses_oil(dItemNo_KANTERA2_e));
    selected[2] = 4; mixed[2] = 1;
    assert(z_item_uses_oil(dItemNo_BOW_e));
    mixed[2] = dItemNo_NONE_e;
    assert(!z_item_uses_oil(dItemNo_BOW_e));
    selected[2] = MAX_ITEM_SLOTS; mixed[2] = 4;
    assert(!z_item_uses_oil(dItemNo_BOW_e));
    // Keep native Bow/Bomb/Hawkeye and fishing combos; still reject corrupt data.
    for (auto partner : {dItemNo_NORMAL_BOMB_e, dItemNo_WATER_BOMB_e,
            dItemNo_POKE_BOMB_e, dItemNo_HAWK_EYE_e}) {
        inventory.mItems[1] = partner;
        assert(valid_mix_pair(&inventory, 1, 4));
        selected[2] = 1; mixed[2] = 4;
        assert(!z_item_uses_oil(dItemNo_BOW_e));
    }
    inventory.mItems[3] = dItemNo_FISHING_ROD_1_e;
    for (auto partner : {dItemNo_BEE_CHILD_e, dItemNo_WORM_e, dItemNo_ZORAS_JEWEL_e}) {
        inventory.mItems[1] = partner;
        assert(valid_mix_pair(&inventory, 1, 3));
        assert(!valid_mix_pair(&inventory, 1, 4));
    }
    selected[0] = MAX_ITEM_SLOTS; mixed[0] = 4;
    repair_mix_pairs(&inventory);
    assert(mixed[0] == dItemNo_NONE_e && repairs == 1);
    // Android must show ammo AND oil for the combo, including an empty oil gauge.
    inventory.mItems[1] = dItemNo_KANTERA_e;
    selected[2] = 1; mixed[2] = 4; resolvedItem = dItemNo_BOW_e;
    for (int value : {50, 0}) {
        oil = value;
        sync_z_touch_item_meter(&button);
        assert(rendered.find("<count ") != std::string::npos);
        assert(rendered.find("<oil-meter ") != std::string::npos);
    }
    mixed[2] = dItemNo_NONE_e;
    sync_z_touch_item_meter(&button);
    assert(rendered.find("<oil-meter ") == std::string::npos);
    paused = true;
    sync_z_touch_item_meter(&button);
    assert(rendered.find("<count ") == std::string::npos);
    // Native touch X/Y oil hook works independently of custom HUD styling.
    for (auto control : {dusk::ui::Control::X, dusk::ui::Control::Y}) {
        const int slot = control == dusk::ui::Control::X ? 0 : 1;
        selected[slot] = 1; mixed[slot] = 4;
        for (int value : {0, 50, 100}) {
            oil = value;
            std::optional<float> fill;
            after_touch_item_oil(nullptr, &control, &fill, nullptr);
            assert(fill && *fill == value / 100.0f);
        }
        std::optional<float> provided = .25f;
        after_touch_item_oil(nullptr, &control, &provided, nullptr);
        assert(*provided == .25f);
        selected[slot] = 4; mixed[slot] = dItemNo_NONE_e;
        std::optional<float> plain;
        after_touch_item_oil(nullptr, &control, &plain, nullptr);
        assert(!plain);
    }
}
'''
fixture = fixture.replace('// INTEGRITY', '\n'.join(
    function(integrity, signature) for signature in
    ('bool valid_slot(', 'bool valid_mix_pair(', 'void repair_mix_pairs(')))
fixture = fixture.replace('// OIL PREDICATE',
    function(items, 'bool select_item_uses_oil(') + '\n' + function(items, 'bool z_item_uses_oil(const'))
fixture = fixture.replace('// TOUCH HUD', function(items, 'void sync_z_touch_item_meter('))
fixture = fixture.replace('// XY OIL', function(items, 'void after_touch_item_oil('))
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'test.cpp'
    exe = Path(tmp) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('Fire-arrow compatibility passed: combo restoration, native mixes, Z oil and touch HUD')

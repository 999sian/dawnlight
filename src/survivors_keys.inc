// Include pending native pickups: the meter applies these at the end of the
// frame, which can otherwise make the last key unavailable or spend it twice.
unsigned chest_keys() {
    return unsigned(std::clamp(int(dComIfGs_getKeyNum()) + int(dComIfGp_getItemKeyNumCount()), 0, 99));
}
void set_chest_keys(unsigned count) {
    dComIfGp_clearItemKeyNumCount();
    dComIfGs_setKeyNum(u8(std::min(count, 99u)));
}
bool chest_uses_key(const Chest& chest) {
    return !chest.free && chest_keys() != 0;
}
unsigned chest_rupee_price(const Chest& chest) {
    return chest_uses_key(chest) ? 0 : s_run.chest_price(chest.free, chest.fixed);
}

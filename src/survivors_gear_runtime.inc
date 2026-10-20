// Persistent equipment is independent of run rewards and high-score resets.
constexpr const char* kGearBlob="survivors-gear";
GearInventory s_gear;
bool s_gearWritable=true;
unsigned s_gearNoticeTicks=0;
const char* s_gearNotice="";
bool s_gearChestLocated=false;
cXyz s_gearChestPosition{};
s16 s_gearChestYaw=0;
UiWindowHandle s_gearWindow=0;
void close_gear_window();
void restore_gear_collection();
void open_gear_window();
bool hub_gear_interaction(daAlink_c*);
void load_gear() {
    s_gear={};s_gearWritable=true;
    GearInventory::Bytes bytes{};size_t size=bytes.size();
    const auto result=svc_save->get_blob(mod_ctx,kGearBlob,bytes.data(),&size);
    if(result==MOD_UNAVAILABLE)return; // Older slots have no equipment blob.
    if(result!=MOD_OK || !s_gear.decode(std::span(bytes.data(),std::min(size,bytes.size())))) {
        s_gearWritable=false;
        svc_log->warn(mod_ctx,"Survivors: invalid equipment inventory; preserving stored data");
    }
}
bool store_gear(const GearInventory& next) {
    if(!s_gearWritable)return false;
    const auto bytes=next.encode();
    if(svc_save->set_blob(mod_ctx,kGearBlob,bytes.data(),bytes.size())!=MOD_OK)return false;
    s_gear=next;s_progressSaved=false;return true;
}
void drop_boss_gear() {
    // Called only after the kill credit latch. Cleanup/victory-wave kills and
    // debug runs never create persistent equipment. Luck does not change 20%.
    if(s_run.debug_run || !s_run.roll_chance(2000))return;
    auto random=[](unsigned n){
        const uint32_t limit=UINT32_MAX-UINT32_MAX%n;
        uint32_t value;do{value=s_run.next_random()-1;}while(value>=limit);
        return value%n;
    };
    const auto item=GearInventory::roll(random);
    auto next=s_gear;
    if(next.add(item)==GearInventory::none)s_gearNotice="Equipment inventory full!";
    else if(!store_gear(next))s_gearNotice="Equipment save failed!";
    else s_gearNotice=item.kind==GearKind::Sword?"Weapon found!":"Tunic found!";
    s_gearNoticeTicks=4*ticks_per_second;
}

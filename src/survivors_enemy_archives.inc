// Retain only archives actually loaded by run-owned standalone actors. This
// covers every map, boss variants and descendants without a second roster of
// archive names. Native actors keep their own references and delete normally.
DEFINE_HOOK(static_cast<int (*)(request_of_phase_process_class*, const char*)>(&dComIfG_resLoad), SurvivorsArchiveLoad);
DEFINE_HOOK(static_cast<int (*)(request_of_phase_process_class*, const char*, JKRHeap*)>(&dComIfG_resLoad), SurvivorsArchiveHeapLoad);
struct EnemyArchiveHold {
    std::array<char,11> name{};
    JKRArchive* archive=nullptr;
};
// Only one map's encountered resources are retained. If full, native loading
// continues unchanged; no allocation, spawn failure or cross-map accumulation.
std::array<EnemyArchiveHold,32> s_enemyArchives{};
bool s_enemyArchivesEnabled=false;
unsigned s_enemyArchiveMap=0;

void release_enemy_archives() {
    s_enemyArchivesEnabled=false;
    for(auto& hold:s_enemyArchives) {
        if(!hold.archive)continue;
        auto* info=dComIfG_getObjectResInfo(hold.name.data());
        // Reset/shutdown may already have removed a host resource. Never drop a
        // reference on a different archive that reused the same resource slot.
        const bool owned=info && info->getArchive()==hold.archive;
        const auto name=hold.name;
        hold={};
        if(owned)dComIfG_deleteObjectResMain(name.data());
    }
}
void begin_enemy_archives() {
    release_enemy_archives();
    s_enemyArchiveMap=s_map;
    s_enemyArchivesEnabled=true;
}
void update_enemy_archives() {
    if(s_enemyArchivesEnabled && (!s_active || !s_gameStarted || s_hub ||
        s_restart || s_map!=s_enemyArchiveMap || !in_arena() || transitioning()))
        release_enemy_archives();
}
void after_enemy_archive_load(ModContext*,void* args,void* result,void*) {
    if(!s_enemyArchivesEnabled || !s_active || !s_gameStarted || s_hub ||
       s_restart || s_map!=s_enemyArchiveMap || !in_arena() || transitioning() ||
       !enemy_spawner_process_active() || !result ||
       *static_cast<int*>(result)!=cPhs_COMPLEATE_e)return;
    const auto* phase=mods::arg<request_of_phase_process_class*>(args,0);
    const auto* name=mods::arg<const char*>(args,1);
    if(!phase || phase->id!=2 || !name || !*name)return;
    auto* info=dComIfG_getObjectResInfo(name);
    if(!info || !info->getArchive() || info->getDMCommand() || info->heap || info->getCount()>=0xffff)return;
    const auto* canonical=info->getArchiveName();
    const auto length=std::strlen(canonical);
    if(length>=s_enemyArchives.front().name.size())return;
    EnemyArchiveHold* available=nullptr;
    for(auto& hold:s_enemyArchives) {
        if(hold.archive==info->getArchive() && std::strcmp(hold.name.data(),canonical)==0)return;
        if(!hold.archive && !available)available=&hold;
    }
    if(!available)return;
    // The successful native load already mounted and prepared the archive.
    // setObjectRes now only increments its reference count: no second load,
    // phase request, polling, recursion or change to the actor's phase state.
    if(!dComIfG_setObjectRes(canonical,u8(0),nullptr))return;
    std::memcpy(available->name.data(),canonical,length+1);
    available->archive=info->getArchive();
}
void after_enemy_archive_heap_load(ModContext* ctx,void* args,void* result,void* user) {
    // Never extend the lifetime of an archive placed in an actor-owned heap.
    if(mods::arg<JKRHeap*>(args,2)==nullptr)after_enemy_archive_load(ctx,args,result,user);
}

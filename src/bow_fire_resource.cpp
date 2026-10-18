#include "bow_fire_resource.hpp"
#include "particle_resource.hpp"
#include "service_imports.hpp"

#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JParticle/JPAEmitterManager.h"
#include "JSystem/JParticle/JPAResource.h"
#include "JSystem/JParticle/JPAResourceManager.h"
#include "d/d_particle.h"
#include "dolphin/dvd.h"
#include "mods/service.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace dawnlight {
namespace {

// Borrow the original resources from the disc, using private common-bank IDs.
constexpr std::array<u16,8> kSources{0x8113,0x846c,0x846d,0x846e,0x846f,0x8470,0x8471,0x8472};
struct ImportedEffect {
    JKRExpHeap* heap=nullptr;
    std::unique_ptr<u8[]> storage;
    JPAResource* resource=nullptr;
    u16 effect=0;
    bool attempted=false;
};
std::array<ImportedEffect,kSources.size()> s_imported;
std::array<std::vector<u8>,kSources.size()> s_sourceBytes;
JPAResourceManager* s_common = nullptr;
bool s_scanned=false,s_installed=false;
JPAResource** s_originalResources = nullptr;
JPATexture** s_originalTextures = nullptr;
u16 s_resourceCount = 0, s_resourceCapacity = 0;
u16 s_textureCount = 0, s_textureCapacity = 0;
std::vector<JPAResource*> s_resources;
std::vector<JPATexture*> s_textures;

JPAResourceManager* common_manager() {
    auto* manager = dPa_control_c::mEmitterMng;
    return manager != nullptr ? manager->getResourceManager(u8{0}) : nullptr;
}

void read_particle_resources() {
    DVDDir directory{};
    if (!DVDOpenDir("/res/Particle", &directory)) return;
    DVDDirEntry entry{};
    while (DVDReadDir(&directory, &entry)) {
        if (entry.isDir || entry.name == nullptr) continue;
        const std::string name(entry.name);
        if (name.size() < 4 || name.substr(name.size() - 4) != ".jpc") continue;
        DVDFileInfo file{};
        if (!DVDFastOpen(entry.entryNum, &file)) continue;
        const auto length = file.length;
        // Bound allocations for malformed files; DVD reads require 32-byte alignment.
        if (length >= 16 && length <= 16 * 1024 * 1024) {
            const auto rounded = (length + 31) & ~u32{31};
            std::vector<u8> storage(rounded + 31);
            auto* bytes = reinterpret_cast<u8*>(
                (reinterpret_cast<uintptr_t>(storage.data()) + 31) & ~uintptr_t{31});
            if (DVDReadPrio(&file, bytes, rounded, 0, 2) >= static_cast<s32>(length)) {
                for(unsigned i=0;i<kSources.size();++i)
                    if(s_sourceBytes[i].empty())s_sourceBytes[i]=particle_resource::extract({bytes,length},kSources[i]);
            }
        }
        DVDClose(&file);
        if(std::all_of(s_sourceBytes.begin(),s_sourceBytes.end(),[](const auto& bytes){return !bytes.empty();}))break;
    }
    DVDCloseDir(&directory);
}

bool install_resource(JPAResourceManager* common,unsigned index) {
    if(!s_scanned){s_scanned=true;read_particle_resources();}
    auto bytes=std::move(s_sourceBytes[index]);
    auto& imported=s_imported[index];
    if (bytes.empty() || common->resRegNum == 0xffff) return false;
    const u16 textureCount = particle_resource::read16(bytes.data() + 10);
    if (unsigned(common->texRegNum) + textureCount > 0xffff) return false;

    // A private common ID keeps native Bulblin arrows and every room's pack intact.
    u16 effect = 0x7fff;
    while (effect != 0 && common->checkUserIndexDuplication(effect)) --effect;
    if (effect == 0) return false;
    particle_resource::write16(bytes.data() + 16, effect);

    // The game's fixed system heap is already partitioned during play. The
    // size-only create overload would allocate from it and abort on exhaustion,
    // even with errorFlag=false. Back this JPA heap with mod-owned host memory;
    // the parent only registers it for JKR disposer/texture lifetime tracking.
    const u32 heapSize = static_cast<u32>(bytes.size()) + 256 * 1024;
    imported.storage.reset(new (std::nothrow) u8[heapSize + 31]);
    if (!imported.storage) return false;
    auto* heapMemory = reinterpret_cast<void*>(
        (reinterpret_cast<uintptr_t>(imported.storage.get()) + 31) & ~uintptr_t{31});
    imported.heap = JKRExpHeap::create(heapMemory, heapSize, JKRHeap::getSystemHeap(), false);
    if (imported.heap == nullptr) {
        imported.storage.reset();
        return false;
    }
    auto* data = static_cast<u8*>(imported.heap->alloc(static_cast<u32>(bytes.size()), 32));
    if (data == nullptr) {
        imported.heap->destroy();
        imported.heap = nullptr;
        imported.storage.reset();
        return false;
    }
    std::memcpy(data, bytes.data(), bytes.size());
    JPAResourceManager donor(data, imported.heap);
    imported.resource = donor.getResource(effect);
    if(!imported.resource) {
        imported.heap->destroy();imported.heap=nullptr;imported.storage.reset();return false;
    }
    auto* indices = const_cast<BE(u16)*>(imported.resource->mpTDB1);
    for (u8 i = 0; i < imported.resource->texNum; ++i) {
        indices[i] = static_cast<u16>(indices[i]) + common->texRegNum;
    }

    if(!s_installed) {
        s_originalResources = common->pResAry;
        s_originalTextures = common->pTexAry;
        s_resourceCount = common->resRegNum;
        s_resourceCapacity = common->resMaxNum;
        s_textureCount = common->texRegNum;
        s_textureCapacity = common->texMaxNum;
        s_resources.assign(common->pResAry, common->pResAry + common->resRegNum);
        s_textures.assign(common->pTexAry, common->pTexAry + common->texRegNum);
        s_installed=true;
    }
    s_resources.push_back(imported.resource);
    s_textures.insert(s_textures.end(), donor.pTexAry, donor.pTexAry + donor.texRegNum);
    common->pResAry = s_resources.data();
    common->pTexAry = s_textures.data();
    common->resMaxNum = common->resRegNum = static_cast<u16>(s_resources.size());
    common->texMaxNum = common->texRegNum = static_cast<u16>(s_textures.size());
    imported.effect = effect;
    return true;
}

}  // namespace

std::uint16_t imported_particle_effect(unsigned index) {
    auto* common=common_manager();if(!common)return 0;
    if(s_common!=common){shutdown_bow_fire_resource();s_common=common;}
    auto& imported=s_imported[index];
    if(!imported.attempted) {
        imported.attempted=true;
        if(!install_resource(common,index))svc_log->error(mod_ctx,
            index==0?"Dawnlight: could not load Bulblin flame from the disc":"Dawnlight: could not load Dodongo fire effect from the disc");
    }
    return imported.effect;
}
std::uint16_t bow_fire_effect() {return imported_particle_effect(0);}
std::uint16_t dodongo_fire_effect(std::uint16_t native) {
    for(unsigned i=1;i<kSources.size();++i)if(kSources[i]==native)return imported_particle_effect(i);
    return 0;
}

void shutdown_bow_fire_resource() {
    if (s_installed && common_manager() == s_common) {
        // Invalidated emitters can still hold pRes until the next particle update.
        // Remove them before releasing the resource/texture memory.
        auto* manager = dPa_control_c::mEmitterMng;
        for (u8 group = 0; group < manager->gidMax; ++group) {
            auto& list = manager->pEmtrUseList[group];
            for (auto* link = list.getFirst(); link != list.getEnd();) {
                auto* next = link->getNext();
                auto* emitter = link->getObject();
                if (std::any_of(s_imported.begin(),s_imported.end(),[&](const auto& entry){return entry.resource&&emitter->pRes==entry.resource;}))
                    manager->forceDeleteEmitter(emitter);
                link = next;
            }
        }
        s_common->pResAry = s_originalResources;
        s_common->pTexAry = s_originalTextures;
        s_common->resRegNum = s_resourceCount;
        s_common->resMaxNum = s_resourceCapacity;
        s_common->texRegNum = s_textureCount;
        s_common->texMaxNum = s_textureCapacity;
    }
    for(auto& imported:s_imported) {
        if(imported.heap)imported.heap->destroy();
        imported={}; // Release backing memory after texture/disposer cleanup.
    }
    s_sourceBytes={};s_common=nullptr;s_scanned=false;s_installed=false;
    s_resources.clear();
    s_textures.clear();
}

}  // namespace dawnlight

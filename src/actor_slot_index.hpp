#pragma once

#include <cstddef>
#include <unordered_map>

namespace dawnlight {
// Stores slot numbers, never actor or vector-element pointers. Add immediately
// after accepting a spawn ID; erase only once deletion/failed creation is known.
// Rebuild after compaction. Retiring and pending actors remain indexed.
template<class Id>
class ActorSlotIndex {
    std::unordered_map<Id, std::size_t> slots_;
public:
    void add(Id id, std::size_t slot) { slots_.insert_or_assign(id, slot); }
    void erase(Id id) { slots_.erase(id); }
    template<class Container>
    typename Container::value_type* find(Container& records, Id id) const {
        const auto it = slots_.find(id);
        if (it == slots_.end() || it->second >= records.size()) return nullptr;
        auto& record = records[it->second];
        // Defend against stale slots without ever dereferencing an engine actor.
        return record.id == id ? &record : nullptr;
    }
    template<class Container>
    void rebuild(const Container& records, Id none) {
        slots_.clear();
        for (std::size_t i = 0; i < records.size(); ++i)
            if (records[i].id != none) add(records[i].id, i);
    }
};
} // namespace dawnlight

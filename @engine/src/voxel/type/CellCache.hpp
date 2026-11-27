#pragma once

#include "voxel/type/Grid.hpp"

#include <array>
#include <unordered_map>

namespace derivean {

// What lookups in a map keyed by cell found, a node or its absence, remembered by where the key lies:
// four apart along each axis, so the keys around a point take different slots. It serves a
// std::unordered_map, whose nodes keep their place until erased; whoever inserts or erases a key
// forgets it here. The slots point into one map, so a copy or a move starts empty and a moved-from
// cache is emptied: neither may answer for another map's nodes.
template<typename T>
struct CellCache {
    static constexpr int Side = 4;
    struct Slot {
        Cell key;
        T* node{};
        bool held{};   // the slot answers for its key
    };
    std::array<Slot, Side * Side * Side> slots{};

    CellCache() = default;
    CellCache(const CellCache&) {}
    CellCache(CellCache&& other) noexcept { other.slots = {}; }
    CellCache& operator=(const CellCache&) {
        slots = {};
        return *this;
    }
    CellCache& operator=(CellCache&& other) noexcept {
        slots = {};
        other.slots = {};
        return *this;
    }
};

template<typename T>
typename CellCache<T>::Slot& slotOf(CellCache<T>& cache, Cell key) {
    constexpr int side = CellCache<T>::Side;
    return cache.slots[size_t(((key.y & (side - 1)) * side + (key.z & (side - 1))) * side + (key.x & (side - 1)))];
}

template<typename T, typename Hash>
T* cachedFind(CellCache<T>& cache, std::unordered_map<Cell, T, Hash>& map, Cell key) {
    auto& slot = slotOf(cache, key);
    if (slot.held && slot.key == key) return slot.node;
    const auto found = map.find(key);
    slot = {key, found == map.end() ? nullptr : &found->second, true};
    return slot.node;
}

template<typename T>
void forget(CellCache<T>& cache, Cell key) {
    auto& slot = slotOf(cache, key);
    if (slot.key == key) slot.held = false;
}

} // namespace derivean

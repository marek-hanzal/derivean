#include "groundwater/service/Aquifers.hpp"

#include "groundwater/fn/Stages.hpp"

namespace derivean::sim {

std::shared_ptr<const GroundwaterTile> Aquifers::find(TileKey local) const {
    std::lock_guard lock(mutex_);
    const auto found = slots_.find(local);
    return found == slots_.end() ? nullptr : found->second.tile;
}

std::shared_ptr<const GroundwaterTile> Aquifers::require(Atlas& atlas, TileKey local) {
    {
        std::unique_lock lock(mutex_);
        Slot& slot = slots_[local];
        if (slot.tile) return slot.tile;
        if (slot.claimed) {
            // The thread that claimed it waits for landscape tiles only, and no landscape tile
            // waits for ground water: sleeping here cannot close a circle.
            done_.wait(lock, [&] { return slots_[local].tile != nullptr; });
            return slots_[local].tile;
        }
        slot.claimed = true;
    }
    std::shared_ptr<const Tile> held[9];
    const Tile* around[9];
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) atlas.request({Level::Local, local.x + dx, local.z + dz}, 1000 - int(Level::Local) * 100);
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            const int at = (dz + 1) * 3 + dx + 1;
            held[at] = atlas.require({Level::Local, local.x + dx, local.z + dz});
            around[at] = held[at].get();
        }
    std::shared_ptr<const GroundwaterTile> tile = buildGroundwater(atlas.setting(), local, around);
    {
        std::lock_guard lock(mutex_);
        slots_[local].tile = tile;
    }
    done_.notify_all();
    return tile;
}

} // namespace derivean::sim

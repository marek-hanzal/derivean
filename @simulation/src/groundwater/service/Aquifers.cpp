#include "groundwater/service/Aquifers.hpp"

#include "groundwater/fn/Reads.hpp"
#include "groundwater/fn/Regional.hpp"
#include "groundwater/fn/Stages.hpp"

#include <stdexcept>

namespace derivean::sim {
namespace {

// The nine tiles of a level around a key, row-major with the key in the middle; asked for ahead, so
// the atlas's pool builds them side by side.
void requireAround(Atlas& atlas, TileKey key, BuildMode mode, std::shared_ptr<const Tile> held[9], const Tile* around[9]) {
    const auto reads = aquiferTerrain(key);
    if (mode == BuildMode::Demand)
        for (const TileKey tile : reads) atlas.request(tile, 1000 - int(key.level) * 100);
    for (size_t i = 0; i < reads.size(); ++i) {
        held[i] = mode == BuildMode::Prepared ? atlas.find(reads[i]) : atlas.require(reads[i]);
        if (!held[i]) throw std::logic_error("missing prepared terrain input for aquifer");
        around[i] = held[i].get();
    }
}

} // namespace

std::shared_ptr<const GroundwaterTile> Aquifers::find(TileKey local) const {
    std::lock_guard lock(mutex_);
    const auto found = slots_.find(local);
    return found == slots_.end() ? nullptr : found->second.tile;
}

std::shared_ptr<const RegionalTile> Aquifers::findRegional(TileKey region) const {
    std::lock_guard lock(mutex_);
    const auto found = regional_.find(region);
    return found == regional_.end() ? nullptr : found->second.tile;
}

template <class T, class Build>
std::shared_ptr<const T> Aquifers::once(Slots<T>& slots, TileKey key, Build&& build) {
    {
        std::unique_lock lock(mutex_);
        // A thread that claimed a local tile waits for region tiles and landscape tiles only, one
        // that claimed a region tile for landscape tiles only, and the landscape tiles ground
        // water asks the atlas for never ask for ground water: sleeping here cannot close a circle.
        // Only a ground tile asks, and nothing here waits for one.
        done_.wait(lock, [&] { return slots[key].tile != nullptr || !slots[key].claimed; });
        Slot<T>& slot = slots[key];
        if (slot.tile) return slot.tile;
        // Unclaimed: never built, or the build threw and gave the claim back. A tile is a pure
        // function of the setting and the key, so a waiter then works it out itself rather than
        // sharing a failure that may have been the machine's.
        slot.claimed = true;
    }
    // A build that throws gives the claim back, so that nobody waits for it forever.
    struct Release {
        Aquifers& owner;
        Slots<T>& slots;
        TileKey key;
        bool done{};
        ~Release() {
            if (done) return;
            {
                std::lock_guard lock(owner.mutex_);
                slots[key].claimed = false;
            }
            owner.done_.notify_all();
        }
    } release{*this, slots, key};
    std::shared_ptr<const T> tile = build();
    {
        std::lock_guard lock(mutex_);
        slots[key].tile = tile;
    }
    release.done = true;
    done_.notify_all();
    return tile;
}

std::shared_ptr<const RegionalTile> Aquifers::requireRegional(Atlas& atlas, TileKey region, BuildMode mode) {
    return once(regional_, region, [&] {
        std::shared_ptr<const Tile> held[9];
        const Tile* around[9];
        requireAround(atlas, region, mode, held, around);
        return std::shared_ptr<const RegionalTile>(buildRegional(atlas.setting(), region, around));
    });
}

std::vector<std::shared_ptr<const RegionalTile>> Aquifers::regionsUnder(Atlas& atlas, TileKey local, BuildMode mode) {
    std::vector<std::shared_ptr<const RegionalTile>> regions;
    for (const TileKey key : aquiferRegions(local)) {
        auto region = mode == BuildMode::Prepared ? findRegional(key) : requireRegional(atlas, key);
        if (!region) throw std::logic_error("missing prepared regional water input");
        regions.push_back(std::move(region));
    }
    return regions;
}

std::shared_ptr<const GroundwaterTile> Aquifers::require(Atlas& atlas, TileKey local, BuildMode mode) {
    return once(slots_, local, [&] {
        std::shared_ptr<const Tile> held[9];
        const Tile* around[9];
        requireAround(atlas, local, mode, held, around);
        const std::vector<std::shared_ptr<const RegionalTile>> regions = regionsUnder(atlas, local, mode);
        std::vector<const RegionalTile*> under;
        for (const auto& region : regions) under.push_back(region.get());
        return std::shared_ptr<const GroundwaterTile>(buildGroundwater(atlas.setting(), local, around, under));
    });
}

} // namespace derivean::sim

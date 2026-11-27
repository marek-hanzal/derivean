#include "groundwater/service/Aquifers.hpp"

#include "groundwater/fn/Reads.hpp"
#include "groundwater/fn/Regional.hpp"
#include "groundwater/fn/Stages.hpp"
#include "simulation/constant/BuildPriority.hpp"

#include <optional>
#include <stdexcept>

namespace derivean::sim {
namespace {

// The nine tiles of a level around a key, row-major with the key in the middle; asked for ahead, so
// the atlas's pool builds them side by side.
void requireAround(Atlas& atlas, TileKey key, BuildMode mode, std::shared_ptr<const Tile> held[9], const Tile* around[9]) {
    const auto reads = aquiferTerrain(key);
    if (mode == BuildMode::Demand)
        for (const TileKey tile : reads) atlas.request(tile, terrainPriority(key.level));
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
std::shared_ptr<const T> Aquifers::once(Slots<T>& slots, TileKey key, Workers* workers, bool wait, Build&& build) {
    // A thread that claimed a local tile waits for region tiles and landscape tiles only, one
    // that claimed a region tile for landscape tiles only, and the landscape tiles ground water
    // asks the atlas for never ask for ground water. Only a ground tile asks, and nothing here
    // waits for one. A waiter works the pool's queue meanwhile, though only above what it holds
    // (Workers::Claim); the claim of a tile here holds its priority.
    const int priority = Workers::above(aquiferPriority(key.level));
    const auto settled = [&] { return slots[key].tile != nullptr || !slots[key].claimed; };
    std::optional<Workers::Claim> claim;
    while (true) {
        if (!wait) {
            // Asked ahead: built or under way is enough.
            std::lock_guard lock(mutex_);
            Slot<T>& slot = slots[key];
            if (slot.tile || slot.claimed) return slot.tile;
            slot.claimed = true;
            claim.emplace(priority);
            break;
        }
        if (workers)
            workers->helpUntil([&] {
                std::lock_guard lock(mutex_);
                return settled();
            });
        std::unique_lock lock(mutex_);
        // Another waiter may have claimed it since: wait again.
        if (workers && !settled()) continue;
        done_.wait(lock, settled);
        Slot<T>& slot = slots[key];
        if (slot.tile) return slot.tile;
        // Unclaimed: never built, or the build threw and gave the claim back. A tile is a pure
        // function of the setting and the key, so a waiter then works it out itself rather than
        // sharing a failure that may have been the machine's.
        slot.claimed = true;
        claim.emplace(priority);
        break;
    }
    // A build that throws gives the claim back, so that nobody waits for it forever.
    struct Release {
        Aquifers& owner;
        Slots<T>& slots;
        TileKey key;
        Workers* workers;
        bool done{};
        ~Release() {
            if (done) return;
            {
                std::lock_guard lock(owner.mutex_);
                slots[key].claimed = false;
            }
            owner.done_.notify_all();
            if (workers) workers->notify();
        }
    } release{*this, slots, key, workers};
    std::shared_ptr<const T> tile = build();
    {
        std::lock_guard lock(mutex_);
        slots[key].tile = tile;
    }
    release.done = true;
    done_.notify_all();
    if (workers) workers->notify();
    return tile;
}

std::shared_ptr<const RegionalTile> Aquifers::requireRegional(Atlas& atlas, TileKey region, BuildMode mode) { return regional(atlas, region, mode, true); }

std::shared_ptr<const RegionalTile> Aquifers::regional(Atlas& atlas, TileKey region, BuildMode mode, bool wait) {
    return once(regional_, region, atlas.setting().workers, wait, [&] {
        std::shared_ptr<const Tile> held[9];
        const Tile* around[9];
        requireAround(atlas, region, mode, held, around);
        return std::shared_ptr<const RegionalTile>(buildRegional(atlas.setting(), region, around));
    });
}

void Aquifers::requestRegional(Atlas& atlas, TileKey region) {
    if (!atlas.setting().workers) return;
    {
        std::lock_guard lock(mutex_);
        Slot<RegionalTile>& slot = regional_[region];
        if (slot.tile || slot.claimed || slot.requested) return;
        slot.requested = true;
    }
    // The task never waits: a region someone else builds is left to them. A build that throws here
    // is left to whoever requires the tile: it builds anew and gets the exception itself, where the
    // pool has nobody to pass it to.
    atlas.setting().workers->submit([this, &atlas, region] {
        try {
            regional(atlas, region, BuildMode::Demand, false);
        } catch (...) {
        }
    }, aquiferPriority(region.level));
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
    return once(slots_, local, atlas.setting().workers, true, [&] {
        // The regional flow under the field reads no local tile: the pool builds it side by side
        // with the local tiles and with each other, and regionsUnder takes it or waits for it.
        if (mode == BuildMode::Demand && atlas.setting().workers)
            for (const TileKey region : aquiferRegions(local)) requestRegional(atlas, region);
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

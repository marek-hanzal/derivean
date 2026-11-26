#include "landscape/service/Atlas.hpp"

#include "landscape/fn/Build.hpp"
#include "simulation/type/World.hpp"

#include <algorithm>
#include <stdexcept>

namespace derivean::sim {

Atlas::Atlas(Setting setting, Workers& workers) : setting_(setting), workers_(workers) { setting_.workers = &workers; }

std::shared_ptr<const Tile> Atlas::find(TileKey key) const {
    std::lock_guard lock(mutex_);
    const auto found = slots_.find(key);
    return found == slots_.end() ? nullptr : found->second.tile;
}

size_t Atlas::finished() const {
    std::lock_guard lock(mutex_);
    return finished_;
}

void Atlas::bind(World& world) {
    std::lock_guard lock(mutex_);
    if (!setting_.world) setting_.world = &world;
}

void Atlas::request(TileKey key, int priority) {
    {
        std::lock_guard lock(mutex_);
        const auto found = slots_.find(key);
        if (found != slots_.end() && (found->second.tile || found->second.claimed)) return;
    }
    workers_.submit([this, key] { require(key); }, priority);
}

std::shared_ptr<const Tile> Atlas::require(TileKey key, BuildMode mode) {
    {
        std::unique_lock lock(mutex_);
        // A claimed tile only waits on a prerequisite. A failed build releases the claim.
        done_.wait(lock, [&] { return slots_[key].tile || !slots_[key].claimed; });
        Slot& slot = slots_[key];
        if (slot.tile) return slot.tile;
        slot.claimed = true;
    }

    try {
        const std::vector<TileKey> needed = dependencies(key);
        if (mode == BuildMode::Demand)
            for (const TileKey other : needed) request(other, 1000 - int(other.level) * 100);
        std::vector<std::shared_ptr<const Tile>> held;
        held.reserve(needed.size());
        for (const TileKey other : needed) {
            auto tile = mode == BuildMode::Prepared ? find(other) : require(other);
            if (!tile) throw std::logic_error("missing prepared terrain input");
            held.push_back(std::move(tile));
        }

        const TileLookup lookup = [&](TileKey wanted) -> const Tile* {
            for (size_t i = 0; i < needed.size(); ++i)
                if (needed[i] == wanted) return held[i].get();
            return nullptr;
        };
        Setting setting;
        {
            std::lock_guard lock(mutex_);
            setting = setting_;
        }
        const std::vector<TileKey> water = mode == BuildMode::Prepared ? groundwaterRead(key) : std::vector<TileKey>{};
        AquiferLookup aquifers;
        if (mode == BuildMode::Prepared && key.level == Level::Ground && setting.world)
            aquifers = [&](TileKey wanted) {
                if (std::find(water.begin(), water.end(), wanted) == water.end())
                    throw std::logic_error("undeclared Local aquifer input");
                return setting.world->groundwater.find(wanted);
            };
        std::shared_ptr<const Tile> tile = replayTile(setting, key, lookup, aquifers);
        {
            std::lock_guard lock(mutex_);
            slots_[key].tile = tile;
            ++finished_;
        }
        done_.notify_all();
        return tile;
    } catch (...) {
        {
            std::lock_guard lock(mutex_);
            slots_[key].claimed = false;
        }
        done_.notify_all();
        throw;
    }
}

} // namespace derivean::sim

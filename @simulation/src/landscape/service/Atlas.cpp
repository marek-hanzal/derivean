#include "landscape/service/Atlas.hpp"

#include "landscape/fn/Build.hpp"

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

std::shared_ptr<const Tile> Atlas::require(TileKey key) {
    bool waiting = false;
    {
        std::lock_guard lock(mutex_);
        Slot& slot = slots_[key];
        if (slot.tile) return slot.tile;
        waiting = slot.claimed;
        slot.claimed = true;
    }
    if (waiting) {
        // Another thread is building it. Sleeping is safe: every claimed tile has a thread working
        // on it or waiting further down the dependency order, and that order has no cycles. Running
        // unrelated queued work here instead could pick a tile that waits for this very thread.
        std::unique_lock lock(mutex_);
        done_.wait(lock, [&] { return slots_[key].tile != nullptr; });
        return slots_[key].tile;
    }

    const std::vector<TileKey> needed = dependencies(key);
    for (const TileKey& other : needed) request(other, 1000 - int(other.level) * 100);
    std::vector<std::shared_ptr<const Tile>> held;
    for (const TileKey& other : needed) held.push_back(require(other));

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
    std::shared_ptr<const Tile> tile = replayTile(setting, key, lookup);
    {
        std::lock_guard lock(mutex_);
        slots_[key].tile = tile;
        ++finished_;
    }
    done_.notify_all();
    return tile;
}

} // namespace derivean::sim

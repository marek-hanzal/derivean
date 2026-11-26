#pragma once

#include "core/service/Workers.hpp"
#include "landscape/type/Setting.hpp"
#include "landscape/type/Terrain.hpp"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace derivean::sim {

enum class BuildMode { Demand, Prepared };

// Finished tiles of every level, built on demand. A tile is a pure function of the setting and its
// key; the atlas only decides when the work happens and makes sure it happens once.
class Atlas {
public:
    Atlas(Setting setting, Workers& workers);
    Atlas(const Atlas&) = delete;
    Atlas& operator=(const Atlas&) = delete;

    // Finished tile or null; never blocks on a build.
    std::shared_ptr<const Tile> find(TileKey key) const;

    // Builds the tile and everything it depends on; blocks until it is finished.
    // Prepared takes only finished dependencies and reports a missing input to its caller.
    std::shared_ptr<const Tile> require(TileKey key, BuildMode mode = BuildMode::Demand);

    // Starts the build in the background; higher priority first.
    void request(TileKey key, int priority = 0);

    size_t finished() const;
    const Setting& setting() const { return setting_; }

    // Names the world whose groundwater ground tiles ask for their springs. It must happen before
    // the first ground tile is built, or tiles built before and after would differ; the queries do
    // it on every call that can build one. Later calls change nothing.
    void bind(World& world);

private:
    friend struct BuildGraphFixture;

    struct Slot {
        std::shared_ptr<const Tile> tile;
        bool claimed{};
    };

    Setting setting_;
    Workers& workers_;
    mutable std::mutex mutex_;
    std::condition_variable done_;
    std::unordered_map<TileKey, Slot, TileKeyHash> slots_;
    size_t finished_{};
};

} // namespace derivean::sim

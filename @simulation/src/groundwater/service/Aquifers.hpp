#pragma once

#include "groundwater/type/Groundwater.hpp"
#include "landscape/service/Atlas.hpp"

namespace derivean::sim {

// The ground water of every local tile asked for so far. A tile's ground water is a pure function
// of the setting and its key; this only makes sure it is worked out once. It is built on the
// thread that first asks, with the atlas's pool joining in for the duration of the call: nothing
// of a build is left running when `require` returns, so a world may be destroyed as soon as no
// call into it is under way. What it keeps holds no landscape tile.
class Aquifers {
public:
    Aquifers() = default;
    Aquifers(const Aquifers&) = delete;
    Aquifers& operator=(const Aquifers&) = delete;

    // Builds the nine local tiles the answer reads if they are missing; blocks until it is there.
    std::shared_ptr<const GroundwaterTile> require(Atlas& atlas, TileKey local);

    // Finished or null; never blocks on a build.
    std::shared_ptr<const GroundwaterTile> find(TileKey local) const;

private:
    struct Slot {
        std::shared_ptr<const GroundwaterTile> tile;
        bool claimed{};
    };

    mutable std::mutex mutex_;
    std::condition_variable done_;
    std::unordered_map<TileKey, Slot, TileKeyHash> slots_;
};

} // namespace derivean::sim

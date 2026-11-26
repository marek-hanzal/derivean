#pragma once

#include "groundwater/type/Groundwater.hpp"
#include "groundwater/type/Regional.hpp"
#include "landscape/service/Atlas.hpp"

namespace derivean::sim {

// The ground water of every local and region tile asked for so far. A tile's ground water is a
// pure function of the setting and its key; this only makes sure it is worked out once. It is
// built on the thread that first asks, with the atlas's pool joining in for the duration of the
// call, and holds no landscape tile once built. A world must not be destroyed while a call into it
// is under way; that includes ground tiles still building on the atlas's pool, which ask for their
// springs here.
class Aquifers {
public:
    Aquifers() = default;
    Aquifers(const Aquifers&) = delete;
    Aquifers& operator=(const Aquifers&) = delete;

    // Builds the nine local tiles the answer reads if they are missing, and the regional flow of
    // the region tiles they lie in; blocks until it is there. A build that throws passes the
    // exception to its caller; whoever waited for it builds anew.
    std::shared_ptr<const GroundwaterTile> require(Atlas& atlas, TileKey local);

    // The regional flow of a region tile, from the nine region tiles around it; as `require`.
    std::shared_ptr<const RegionalTile> requireRegional(Atlas& atlas, TileKey region);

    // The regional flow of the region tiles under the field of a local tile: what `require` hands
    // its build (fn/Inflow.hpp).
    std::vector<std::shared_ptr<const RegionalTile>> regionsUnder(Atlas& atlas, TileKey local);

    // Finished or null; never blocks on a build.
    std::shared_ptr<const GroundwaterTile> find(TileKey local) const;

private:
    template <class T>
    struct Slot {
        std::shared_ptr<const T> tile;
        bool claimed{};
    };
    template <class T>
    using Slots = std::unordered_map<TileKey, Slot<T>, TileKeyHash>;

    template <class T, class Build>
    std::shared_ptr<const T> once(Slots<T>& slots, TileKey key, Build&& build);

    mutable std::mutex mutex_;
    std::condition_variable done_;
    Slots<GroundwaterTile> slots_;
    Slots<RegionalTile> regional_;
};

} // namespace derivean::sim

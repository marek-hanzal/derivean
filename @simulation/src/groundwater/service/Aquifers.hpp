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
    std::shared_ptr<const GroundwaterTile> require(Atlas& atlas, TileKey local, BuildMode mode = BuildMode::Demand);

    // The regional flow of a region tile, from the nine region tiles around it; as `require`.
    std::shared_ptr<const RegionalTile> requireRegional(Atlas& atlas, TileKey region, BuildMode mode = BuildMode::Demand);

    // Starts the regional flow of a region tile on the atlas's pool, just below the priority of
    // the region tiles it reads and above the local ones, so that a local build finds its regions
    // under way; whoever requires it meanwhile waits for it or builds it. The task holds this and
    // the atlas: the pool must stop before either goes, as World's destructor makes sure.
    void requestRegional(Atlas& atlas, TileKey region);

    // The regional flow of the region tiles under the field of a local tile: what `require` hands
    // its build (fn/Inflow.hpp).
    std::vector<std::shared_ptr<const RegionalTile>> regionsUnder(Atlas& atlas, TileKey local, BuildMode mode = BuildMode::Demand);

    // Finished or null; never blocks on a build.
    std::shared_ptr<const GroundwaterTile> find(TileKey local) const;
    std::shared_ptr<const RegionalTile> findRegional(TileKey region) const;

private:
    friend struct BuildGraphFixture;

    template <class T>
    struct Slot {
        std::shared_ptr<const T> tile;
        bool claimed{};
        bool requested{};   // handed to the pool ahead (requestRegional)
    };
    template <class T>
    using Slots = std::unordered_map<TileKey, Slot<T>, TileKeyHash>;

    template <class T, class Build>
    std::shared_ptr<const T> once(Slots<T>& slots, TileKey key, Workers* workers, bool wait, Build&& build);
    // `wait` false returns at once, null, where someone else has claimed the region.
    std::shared_ptr<const RegionalTile> regional(Atlas& atlas, TileKey region, BuildMode mode, bool wait);

    mutable std::mutex mutex_;
    std::condition_variable done_;
    Slots<GroundwaterTile> slots_;
    Slots<RegionalTile> regional_;
};

} // namespace derivean::sim

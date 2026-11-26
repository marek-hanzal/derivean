#include "landscape/fn/Build.hpp"
#include "nature/service/Nature.hpp"
#include "simulation/fn/Query.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace derivean {
namespace {

constexpr sim::Level Levels[]{sim::Level::Region, sim::Level::Local, sim::Level::Site, sim::Level::Ground};
constexpr double ReplanDistance = 64.0;   // m
constexpr double FootingGain = 8.0;       // the ground under the feet is worth this much more: without it nobody can stand
constexpr int FirstPriority = 500;        // of the best request of a plan; below the 600 and more the atlas gives a tile's own dependencies
constexpr int OwnPriority = 550;          // of the best own request: above every other request, of this plan or one before
constexpr int ReplanCalls = 8;
constexpr double LookAhead = 20.0;        // s of travel by which the centre of interest runs ahead of the eye
// The tiles the player's own place is made of, from the region down to the ground under the feet,
// come before everything else and may use every worker. The rest of the view leaves a share of the
// workers free, because a build cannot be interrupted: without that the chain down to the ground
// would wait at every link for a far tile to finish, and a free worker also helps a build that
// splits its loops.
constexpr double OwnPlace = 150.0;        // m around the centre of interest
constexpr size_t FreeShare = 4;           // one worker in this many is kept from the rest of the view

struct Wanted {
    double gain;
    sim::TileKey key;
    bool own;           // part of the player's own place
    bool water{};       // the groundwater of local tile `key`, not a landscape tile
};

bool finished(const sim::World& world, sim::TileKey key) { return world.atlas.find(key) != nullptr; }

using Keys = std::unordered_set<sim::TileKey, sim::TileKeyHash>;

// The unfinished tiles `key` is built from, down to those whose own dependencies are all finished:
// those go to `buildable`, deepest first, with the groundwater a ground tile reads marked `water`.
// Asked for directly, a tile the atlas would otherwise build with its whole chain inline: requests
// that share a chain then park every worker but one waiting for the link it has claimed. `seen`
// holds every unfinished tile visited, `dry` every groundwater tile found missing.
void frontier(const sim::World& world, sim::TileKey key, Keys& seen, Keys& dry, std::vector<Wanted>& buildable) {
    if (!seen.insert(key).second) return;
    bool ready = true;
    for (const sim::TileKey& other : sim::dependencies(key))
        if (!finished(world, other)) {
            ready = false;
            frontier(world, other, seen, dry, buildable);
        }
    // Ground water a ground tile reads is built from tiles far from its own chain: walked beside
    // that chain, not after it, so both are built at once.
    for (const sim::TileKey& local : sim::groundwaterRead(key)) {
        if (world.groundwater.find(local)) continue;
        ready = false;
        if (!dry.insert(local).second) continue;
        // Asked for once what it reads is finished; it can still wait on a region tile's regional
        // flow that another groundwater build has claimed.
        bool readable = true;
        for (const sim::TileKey& other : sim::groundwaterSources(local))
            if (!finished(world, other)) {
                readable = false;
                frontier(world, other, seen, dry, buildable);
            }
        if (readable) buildable.push_back({0, local, false, true});
    }
    if (ready) buildable.push_back({0, key, false});
}

} // namespace

// A lone worker is never kept free: the rest of the view would then never be asked for at all.
RequestBudget requestBudget(size_t workers) {
    return {workers, workers - (workers > 1 ? std::max<size_t>(1, workers / FreeShare) : 0)};
}

void requestTiles(Nature& nature, Position eye, Position velocity, double clearance) {
    TileRequests& tiles = nature.tiles;
    const double moved = std::hypot(eye[0] - tiles.planned[0], eye[2] - tiles.planned[2]);
    if (tiles.calls++ % ReplanCalls != 0 && moved < ReplanDistance) return;
    tiles.planned = eye;

    sim::World& world = nature.world;
    std::erase_if(tiles.flying, [&](const TileRequest& request) {
        return request.water ? world.groundwater.find(request.key) != nullptr : finished(world, request.key);
    });
    std::vector<Wanted> missing;
    for (const sim::Level level : Levels) {
        tiles.ready[size_t(level)] = true;
        tiles.wanted[size_t(level)] = tiles.finished[size_t(level)] = 0;
        if (clearance > tiles.ceiling[size_t(level)]) continue;
        const double radius = tiles.reach[size_t(level)];
        const double span = sim::tileSpan(level);
        // What the eye is about to fly over matters as much as what it is over now; what lies round
        // the eye is still wanted, so the square spans both.
        const double ahead = std::min(1.0, 0.5 * radius / std::max(1.0, LookAhead * std::hypot(velocity[0], velocity[2]))) * LookAhead;
        const sim::Vec2 center{eye[0] + velocity[0] * ahead, eye[2] + velocity[2] * ahead};
        const sim::TileKey low = sim::tileAt(level, {std::min(center.x, eye[0]) - radius, std::min(center.z, eye[2]) - radius});
        const sim::TileKey high = sim::tileAt(level, {std::max(center.x, eye[0]) + radius, std::max(center.z, eye[2]) + radius});
        // The coarser data a tile replaces and how near it lies decide what the eye gains from it.
        const double replaced = level == sim::Level::Region ? sim::PlanetCell
                              : sim::specOf(sim::Level(int(level) - 1)).cell * (level == sim::Level::Ground ? FootingGain : 1.0);
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x) {
                const sim::TileKey key{level, x, z};
                ++tiles.wanted[size_t(level)];
                if (finished(world, key)) {
                    ++tiles.finished[size_t(level)];
                    continue;
                }
                tiles.ready[size_t(level)] = false;
                const double dx = std::max({x * span - center.x, center.x - (x + 1) * span, 0.0});
                const double dz = std::max({z * span - center.z, center.z - (z + 1) * span, 0.0});
                const bool own = std::max(dx, dz) < OwnPlace && clearance < tiles.ceiling[size_t(sim::Level::Ground)];
                missing.push_back({replaced / std::max(std::hypot(dx, dz, clearance), 0.5 * span), key, own});
            }
    }
    std::sort(missing.begin(), missing.end(), [](const Wanted& a, const Wanted& b) { return a.own != b.own ? a.own : a.gain > b.gain; });
    // Each wanted tile in turn hands its rank to the tiles it waits for that can be built now.
    Keys seen, dry;
    std::vector<Wanted> order;
    for (const Wanted& wanted : missing) {
        std::vector<Wanted> buildable;
        frontier(world, wanted.key, seen, dry, buildable);
        for (Wanted& next : buildable) order.push_back({wanted.gain, next.key, wanted.own, next.water});
    }
    // A request no wanted tile leads to any more, after a jump say, is called off: a build under way
    // finishes into the atlas, one still queued does nothing when its turn comes.
    std::erase_if(tiles.flying, [&](const TileRequest& request) {
        if (request.water ? dry.contains(request.key) : seen.contains(request.key)) return false;
        request.live->store(false);
        return true;
    });
    const RequestBudget budget = requestBudget(world.workers.threads());
    int priority = FirstPriority, ownPriority = OwnPriority;
    for (const Wanted& wanted : order) {
        if (tiles.flying.size() >= (wanted.own ? budget.own : budget.shared)) break;
        if (std::any_of(tiles.flying.begin(), tiles.flying.end(), [&](const TileRequest& request) { return request.key == wanted.key && request.water == wanted.water; }))
            continue;
        // Straight to the pool, not through sim::requestAround (which ranks coarse levels first
        // whatever is asked) nor Atlas::request (which cannot be called off).
        const auto live = std::make_shared<std::atomic<bool>>(true);
        world.workers.submit([&world, key = wanted.key, water = wanted.water, live] {
            if (!live->load()) return;
            if (water) world.groundwater.require(world.atlas, key);
            else world.atlas.require(key);
        }, wanted.own ? ownPriority-- : priority--);
        tiles.flying.push_back({wanted.key, wanted.water, live});
    }
}

} // namespace derivean

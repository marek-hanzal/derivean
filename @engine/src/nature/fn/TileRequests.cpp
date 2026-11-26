#include "nature/service/Nature.hpp"
#include "simulation/fn/Query.hpp"

#include <algorithm>
#include <cmath>

namespace derivean {
namespace {

struct Reach {
    sim::Level level;
    double radius;      // m around the eye
    double ceiling;     // m of clearance above which the level is not worth building
};

constexpr Reach Reaches[]{
    {sim::Level::Region, RegionReach, 1e9},
    {sim::Level::Local, LocalReach, 90e3},
    {sim::Level::Site, SiteReach, 3000.0},
    {sim::Level::Ground, GroundReach, 150.0},
};
constexpr double ReplanDistance = 64.0;   // m
constexpr double FootingGain = 8.0;       // the ground under the feet is worth this much more: without it nobody can stand
constexpr int FirstPriority = 500;        // of the best request of a plan; below the 600 and more the atlas gives a tile's own dependencies
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
    bool own;   // part of the player's own place
};

bool finished(const sim::World& world, sim::TileKey key) { return world.atlas.find(key) != nullptr; }

sim::Vec2 middle(sim::TileKey key) {
    const double span = sim::tileSpan(key.level);
    return {(key.x + 0.5) * span, (key.z + 0.5) * span};
}

// A tile is integrated from its parents under its own cells, its halo and the margin beyond.
bool parentsFinished(const sim::World& world, sim::TileKey key) {
    if (key.level == sim::Level::Region) return true;
    const sim::Level parent = sim::Level(int(key.level) - 1);
    const double reach = 0.5 * sim::tileSpan(key.level) + sim::Halo * sim::specOf(key.level).cell
                       + sim::parentMargin(key.level) * sim::specOf(parent).cell;
    return sim::readyAround(world, middle(key), reach, parent);
}

} // namespace

void requestTiles(Nature& nature, Position eye, Position velocity, double clearance) {
    TileRequests& tiles = nature.tiles;
    const double moved = std::hypot(eye[0] - tiles.planned[0], eye[2] - tiles.planned[2]);
    if (tiles.calls++ % ReplanCalls != 0 && moved < ReplanDistance) return;
    tiles.planned = eye;

    sim::World& world = nature.world;
    std::erase_if(tiles.flying, [&](sim::TileKey key) { return finished(world, key); });
    std::vector<Wanted> missing;
    for (const Reach& reach : Reaches) {
        tiles.ready[size_t(reach.level)] = true;
        tiles.wanted[size_t(reach.level)] = tiles.finished[size_t(reach.level)] = 0;
        if (clearance > reach.ceiling) continue;
        const double span = sim::tileSpan(reach.level);
        // What the eye is about to fly over matters as much as what it is over now.
        const double ahead = std::min(1.0, 0.5 * reach.radius / std::max(1.0, LookAhead * std::hypot(velocity[0], velocity[2]))) * LookAhead;
        const sim::Vec2 center{eye[0] + velocity[0] * ahead, eye[2] + velocity[2] * ahead};
        const sim::TileKey low = sim::tileAt(reach.level, {center.x - reach.radius, center.z - reach.radius});
        const sim::TileKey high = sim::tileAt(reach.level, {center.x + reach.radius, center.z + reach.radius});
        // The coarser data a tile replaces and how near it lies decide what the eye gains from it.
        const double replaced = reach.level == sim::Level::Region ? sim::PlanetCell
                              : sim::specOf(sim::Level(int(reach.level) - 1)).cell * (reach.level == sim::Level::Ground ? FootingGain : 1.0);
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x) {
                const sim::TileKey key{reach.level, x, z};
                ++tiles.wanted[size_t(reach.level)];
                if (finished(world, key)) {
                    ++tiles.finished[size_t(reach.level)];
                    continue;
                }
                tiles.ready[size_t(reach.level)] = false;
                const double dx = std::max({x * span - center.x, center.x - (x + 1) * span, 0.0});
                const double dz = std::max({z * span - center.z, center.z - (z + 1) * span, 0.0});
                const bool own = clearance <= reach.ceiling && std::max(dx, dz) < OwnPlace && clearance < Reaches[3].ceiling;
                missing.push_back({replaced / std::max(std::hypot(dx, dz, clearance), 0.5 * span), key, own});
            }
    }
    std::sort(missing.begin(), missing.end(), [](const Wanted& a, const Wanted& b) { return a.own != b.own ? a.own : a.gain > b.gain; });
    const size_t capacity = world.workers.threads(), shared = capacity - std::max<size_t>(1, capacity / FreeShare);
    int priority = FirstPriority;
    for (const Wanted& wanted : missing) {
        if (tiles.flying.size() >= (wanted.own ? capacity : shared)) break;
        if (std::find(tiles.flying.begin(), tiles.flying.end(), wanted.key) != tiles.flying.end()) continue;
        if (!parentsFinished(world, wanted.key)) continue;
        // sim::requestAround ranks coarse levels first whatever is asked; the atlas takes the order worked out here.
        world.atlas.request(wanted.key, priority--);
        tiles.flying.push_back(wanted.key);
    }
}

} // namespace derivean

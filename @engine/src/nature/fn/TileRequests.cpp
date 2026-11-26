#include "nature/service/Nature.hpp"
#include "simulation/fn/BuildGraph.hpp"
#include "simulation/fn/Query.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <exception>
#include <memory>
#include <utility>

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
    sim::BuildKey key;
    bool own;
};

void collect(std::vector<TileRequest>& requests, std::exception_ptr& failure) {
    for (TileRequest& request : requests) {
        if (request.completion.wait_for(std::chrono::seconds(0)) != std::future_status::ready) continue;
        try {
            request.completion.get();
        } catch (...) {
            if (!failure) failure = std::current_exception();
        }
    }
    std::erase_if(requests, [](const TileRequest& request) { return !request.completion.valid(); });
}

void reconcileRequests(TileRequests& tiles, const sim::BuildKeys& wanted, sim::Workers& workers) {
    auto request = tiles.flying.begin();
    while (request != tiles.flying.end()) {
        if (wanted.contains(request->key)) {
            ++request;
            continue;
        }
        if (!workers.cancelQueued(request->ticket)) tiles.retired.push_back(std::move(*request));
        request = tiles.flying.erase(request);
    }
    auto retired = tiles.retired.begin();
    while (retired != tiles.retired.end()) {
        if (!wanted.contains(retired->key)) {
            ++retired;
            continue;
        }
        tiles.flying.push_back(std::move(*retired));
        retired = tiles.retired.erase(retired);
    }
}

} // namespace

// A lone worker is never kept free: the rest of the view would then never be asked for at all.
RequestBudget requestBudget(size_t workers) {
    return {workers, workers - (workers > 1 ? std::max<size_t>(1, workers / FreeShare) : 0)};
}

void requestTiles(Nature& nature, Position eye, Position velocity, double clearance) {
    TileRequests& tiles = nature.tiles;
    // Drain both sets and rethrow the first build failure.
    std::exception_ptr failure;
    collect(tiles.flying, failure);
    collect(tiles.retired, failure);
    if (failure) std::rethrow_exception(failure);

    const double moved = std::hypot(eye[0] - tiles.planned[0], eye[2] - tiles.planned[2]);
    if (tiles.calls++ % ReplanCalls != 0 && moved < ReplanDistance) return;
    tiles.planned = eye;

    sim::World& world = nature.world;
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
                const sim::BuildKey terrain{sim::BuildProduct::Terrain, key};
                if (sim::built(world, terrain)) {
                    ++tiles.finished[size_t(level)];
                    continue;
                }
                tiles.ready[size_t(level)] = false;
                const double dx = std::max({x * span - center.x, center.x - (x + 1) * span, 0.0});
                const double dz = std::max({z * span - center.z, center.z - (z + 1) * span, 0.0});
                const bool own = std::max(dx, dz) < OwnPlace && clearance < tiles.ceiling[size_t(sim::Level::Ground)];
                missing.push_back({replaced / std::max(std::hypot(dx, dz, clearance), 0.5 * span), terrain, own});
            }
    }
    std::sort(missing.begin(), missing.end(), [](const Wanted& a, const Wanted& b) { return a.own != b.own ? a.own : a.gain > b.gain; });
    // Each wanted tile in turn hands its rank to the tiles it waits for that can be built now.
    sim::BuildKeys seen;
    std::vector<Wanted> order;
    for (const Wanted& wanted : missing) {
        std::vector<sim::BuildKey> ready;
        sim::buildFrontier(world, wanted.key, seen, ready);
        for (const sim::BuildKey node : ready) order.push_back({wanted.gain, node, wanted.own});
    }
    reconcileRequests(tiles, seen, world.workers);

    const RequestBudget budget = requestBudget(world.workers.threads());
    tiles.flying.reserve(budget.own);
    int priority = FirstPriority, ownPriority = OwnPriority;
    for (const Wanted& wanted : order) {
        if (tiles.flying.size() >= (wanted.own ? budget.own : budget.shared)) break;
        const auto sameKey = [&](const TileRequest& request) { return request.key == wanted.key; };
        if (std::ranges::any_of(tiles.flying, sameKey)) continue;
        const auto task = std::make_shared<std::packaged_task<void()>>([&world, key = wanted.key] { sim::buildReady(world, key); });
        auto completion = task->get_future();
        const uint64_t ticket = world.workers.submit([task] { (*task)(); }, wanted.own ? ownPriority-- : priority--);
        tiles.flying.push_back({wanted.key, ticket, std::move(completion)});
    }
}

} // namespace derivean

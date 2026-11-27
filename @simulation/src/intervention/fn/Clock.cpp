#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Ocean.hpp"
#include "landscape/fn/Forcing.hpp"

namespace derivean::sim {
namespace {

// Moves the calendar and the sea to second `to`. The groups follow separately: an edit brings only
// the groups it touches to its own date, so a group's steps never depend on when something was
// done elsewhere.
void pass(World& world, double to) {
    Interventions& state = world.interventions;
    drainOcean(world.ocean, to - state.calendar.seconds);
    state.calendar.seconds = to;
}

// A strait acts on the planet's sea. A stroke makes the tiles around it live, brings the groups
// it changes or joins to the present so that they meet on one date, and changes their columns.
void apply(World& world, const Edit& edit) {
    Overlay& overlay = world.interventions.overlay;
    if (edit.kind == EditKind::Strait) {
        const auto cellAt = [&](Vec2 at) {
            const Vec2 point = planetPoint(world.options.seed, at);
            return wrapIndex(floorToInt(point.z / PlanetCell), PlanetCells) * PlanetCells + wrapIndex(floorToInt(point.x / PlanetCell), PlanetCells);
        };
        openStrait(world.ocean, {cellAt(edit.from), cellAt(edit.to), edit.area, edit.heightFrom});
        return;
    }
    Vec2 low, high;
    strokeBounds(edit, low, high);
    const std::vector<TileKey> keys = tilesWithin(low - Vec2{EditReach, EditReach}, high + Vec2{EditReach, EditReach});
    for (LiveSet& set : overlay.sets) {
        bool near = false;
        for (const EditedTile* tile : set.tiles)
            for (const TileKey& key : keys) near = near || (std::abs(tile->key.x - key.x) <= 1 && std::abs(tile->key.z - key.z) <= 1);
        if (!near) continue;
        evolve(world, set, world.interventions.calendar.seconds, true);
        shapeSet(world, set);
        storeSet(set);
    }
    for (const TileKey& key : keys) activate(world, key);
    rasterise(world, edit);
    regroup(world);
}

} // namespace

void advance(World& world, double seconds) {
    Interventions& state = world.interventions;
    const double target = state.calendar.seconds + std::max(0.0, seconds);
    while (state.applied < state.history.size() && state.history[state.applied].time <= target) {
        const Edit edit = state.history[state.applied++];
        pass(world, std::max(edit.time, state.calendar.seconds));
        apply(world, edit);
    }
    pass(world, target);
    for (LiveSet& set : state.overlay.sets) evolve(world, set, target, false);
}

void tick(World& world, double realSeconds) {
    if (!world.interventions.paused) advance(world, realSeconds * world.interventions.rate);
}

void intervene(World& world, Edit edit) {
    Interventions& state = world.interventions;
    edit.time = state.calendar.seconds;
    state.history.insert(state.history.begin() + std::ptrdiff_t(state.applied++), edit);
    apply(world, edit);
}

void syncLive(World& world) {
    settleOcean(world.ocean);
    for (LiveSet& set : world.interventions.overlay.sets) evolve(world, set, world.interventions.calendar.seconds, true);
}

void jumpTo(World& world, double seconds) {
    Interventions& state = world.interventions;
    if (seconds < state.calendar.seconds) {
        state.overlay = Overlay{};
        state.applied = 0;
        state.calendar.seconds = 0;
        const std::shared_ptr<const Ocean> start = world.ocean.start;
        world.ocean = start ? Ocean(*start) : fillOcean(world.planet, world.options.age);
        world.ocean.start = start;
    }
    advance(world, seconds - state.calendar.seconds);
    syncLive(world);
}

} // namespace derivean::sim

#include "game/type/Player.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

#include <algorithm>
#include <vector>

namespace derivean {
namespace {

constexpr double FloatingFeet = 1.3;   // m under the water surface the feet start at where there is no ground to stand on
constexpr double StandOff = 0.1;       // m above the ground
constexpr double FirstRing = 250.0, LastRing = 200e3;   // m from the landing the search for dry ground runs between
constexpr double Diagonal = 0.70710678118654752;
// The eight directions of a ring as exact constants, so the place found is the same on every machine.
constexpr sim::Vec2 Turns[]{{1, 0}, {Diagonal, Diagonal}, {0, 1}, {-Diagonal, Diagonal}, {-1, 0}, {-Diagonal, -Diagonal}, {0, -1}, {Diagonal, -Diagonal}};

// Dry land reports a water surface under its ground (sim::SurfaceSample::water).
bool dry(const sim::SurfaceSample& ground) { return ground.found && ground.ground > ground.water; }

// The places of a ring still in the running after `level`: the tiles under all of them are queued
// at once so the pool builds them side by side, then each is surveyed, which waits for its own.
void narrow(sim::World& world, std::vector<sim::Vec2>& places, sim::Level level) {
    for (const sim::Vec2 place : places) sim::requestAround(world, place, 0.0, level);
    std::erase_if(places, [&](sim::Vec2 place) { return !dry(sim::surveyAt(world, place, level)); });
}

// The simulation lands a new player at a shore. Where its pick lies under water, the player is put
// on the first dry ground a widening search finds: rings of eight places, each twice as far out,
// surveyed from the local level down, a finer level only where the coarser one stands clear of the
// water. Surveys build what they read and the first place of a ring in a fixed order wins, so the
// place depends on the world alone.
sim::Vec2 dryLanding(sim::World& world, sim::Vec2 landing) {
    if (dry(sim::surveyAt(world, landing, sim::Level::Ground))) return landing;
    for (double reach = FirstRing; reach < LastRing; reach *= 2) {
        std::vector<sim::Vec2> places;
        for (const sim::Vec2 turn : Turns) places.push_back({landing.x + reach * turn.x, landing.z + reach * turn.z});
        narrow(world, places, sim::Level::Local);
        narrow(world, places, sim::Level::Site);
        narrow(world, places, sim::Level::Ground);
        if (!places.empty()) return places.front();
    }
    return landing;
}

} // namespace

Position findSpawn(sim::World& world, const std::optional<std::array<double, 2>>& wish) {
    const sim::Vec2 place = wish ? sim::Vec2{(*wish)[0], (*wish)[1]} : dryLanding(world, sim::findLanding(world, {0, 0}));
    // The ground level is what the voxels are cut from; a coarser surface can lie a metre off it.
    const sim::SurfaceSample ground = sim::surveyAt(world, place, sim::Level::Ground);
    const double feet = ground.water > ground.ground ? std::max(double(ground.ground), ground.water - FloatingFeet) : ground.ground;
    return {place.x, feet + StandOff, place.z};
}

} // namespace derivean

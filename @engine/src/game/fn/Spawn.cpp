#include "game/type/Player.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Spawn.hpp"

namespace derivean {
namespace {

constexpr double FloatingFeet = 1.3;   // m under the water surface the feet start at where there is no ground to stand on
constexpr double StandOff = 0.1;       // m above the ground

} // namespace

Position findSpawn(sim::World& world, const std::optional<std::array<double, 2>>& wish) {
    const sim::Vec2 place = wish ? sim::Vec2{(*wish)[0], (*wish)[1]} : sim::findLanding(world, {0, 0});
    const sim::SurfaceSample ground = sim::surveyAt(world, place);
    const double feet = ground.water > ground.ground ? std::max(double(ground.ground), ground.water - FloatingFeet) : ground.ground;
    return {place.x, feet + StandOff, place.z};
}

} // namespace derivean

#include "horizon/type/Beds.hpp"
#include "landscape/fn/Forcing.hpp"

#include <algorithm>

namespace derivean {

Beds bedsAt(const sim::World& world, sim::Vec2 position, float datum) {
    Beds beds;
    beds.seed = world.planet.seed;
    beds.column = sim::strataAt(world.planet, sim::planetPoint(world.planet.seed, position));
    beds.fold = sim::foldAt(beds.seed, beds.column, beds.column.material);
    beds.datum = datum;
    return beds;
}

sim::Material rockAt(Beds& beds, double y) {
    const float depth = std::max(0.0f, beds.datum - float(y));
    if (depth < beds.bed.top || depth >= beds.bed.bottom)
        beds.bed = sim::bedAt(beds.seed, beds.column, sim::unitAt(beds.column, depth, beds.fold), depth, beds.column.material);
    return beds.bed.rock;
}

} // namespace derivean

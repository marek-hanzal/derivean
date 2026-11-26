#include "horizon/type/StandingWater.hpp"
#include "nature/type/SurfaceReader.hpp"

#include <cmath>
#include <limits>

namespace derivean {
namespace {

constexpr float LevelTolerance = 1e-3f;   // m within which two water levels are one flood level
constexpr int Shore = WaterRing - 1;      // points a level is carried at most
constexpr int Around[8][2]{{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

bool wet(const sim::SurfaceSample& sample) { return sample.water > sample.ground + WetDepth; }

} // namespace

std::vector<float> standingWater(const sim::World& world, WaterLattice lattice, const std::function<float(int, int)>& ground) {
    constexpr float none = std::numeric_limits<float>::quiet_NaN();
    const int side = lattice.side + 2 * WaterRing;
    SurfaceReader reader = surfaceReader(world, sim::Level::Site);
    std::vector<float> found(size_t(side) * side, none), bed(found.size());
    std::vector<uint8_t> sparse(found.size(), 0), reach(found.size(), 0);
    for (int j = 0; j < side; ++j)
        for (int i = 0; i < side; ++i) {
            const size_t at = size_t(j) * side + size_t(i);
            const sim::Vec2 place{lattice.x + (i - WaterRing) * lattice.cell, lattice.z + (j - WaterRing) * lattice.cell};
            const sim::SurfaceSample here = surfaceOrPlanet(reader, place);
            bed[at] = ground(i - WaterRing, j - WaterRing);
            if (!wet(here)) continue;
            const double data = here.level == PlanetLevel ? sim::PlanetCell : sim::specOf(here.level).cell;
            for (const auto& ray : Around) {
                if (ray[0] && ray[1]) continue;
                const sim::SurfaceSample partner = surfaceOrPlanet(reader, {place.x + ray[0] * data, place.z + ray[1] * data});
                if (!wet(partner) || std::abs(partner.water - here.water) >= LevelTolerance) continue;
                found[at] = here.water;
                sparse[at] = lattice.cell > data;
                reach[at] = uint8_t(std::min(Shore, 1 + int(std::ceil(data / lattice.cell))));
                break;
            }
        }
    // Where the points stand further apart than the data's cells, a water body must also span two
    // of them: one point alone cannot say where a pond smaller than a cell of the lattice lies. The
    // outermost ring only bears witness; nothing is known about what lies beyond it.
    std::vector<float> level(found.size(), none);
    std::vector<uint8_t> carries(found.size(), 0);
    for (int j = 1; j < side - 1; ++j)
        for (int i = 1; i < side - 1; ++i) {
            const size_t at = size_t(j) * side + i;
            if (std::isnan(found[at])) continue;
            bool spans = !sparse[at];
            for (const auto& ray : Around) spans |= std::abs(found[size_t(j + ray[1]) * side + size_t(i + ray[0])] - found[at]) < LevelTolerance;
            if (!spans) continue;
            level[at] = found[at];
            carries[at] = reach[at];
        }
    // `carries` is how many more points a level may travel from here. Each round hands every level
    // one point on; only ground under the level passes it further: a sill holds the lake back.
    for (int round = 0; round < Shore; ++round) {
        std::vector<float> next = level;
        std::vector<uint8_t> left(found.size(), 0);
        for (int j = 0; j < side; ++j)
            for (int i = 0; i < side; ++i) {
                const size_t at = size_t(j) * side + i;
                if (!std::isnan(level[at])) continue;
                for (const auto& ray : Around) {
                    const int a = i + ray[0], b = j + ray[1];
                    if (a < 0 || b < 0 || a >= side || b >= side) continue;
                    const size_t beside = size_t(b) * side + a;
                    if (!carries[beside] || level[beside] < next[at]) continue;
                    if (level[beside] > next[at] || std::isnan(next[at])) left[at] = 0;
                    next[at] = level[beside];
                    left[at] = std::max<uint8_t>(left[at], carries[beside] - 1);
                }
                if (!(bed[at] < next[at] - WetDepth)) left[at] = 0;
            }
        level = std::move(next);
        carries = std::move(left);
    }
    std::vector<float> water(size_t(lattice.side) * lattice.side);
    for (int j = 0; j < lattice.side; ++j)
        for (int i = 0; i < lattice.side; ++i) water[size_t(j) * lattice.side + i] = level[size_t(j + WaterRing) * side + size_t(i + WaterRing)];
    return water;
}

} // namespace derivean

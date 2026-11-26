#include "Groundwater.hpp"

#include "groundwater/fn/Conductivity.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Heat.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/type/World.hpp"
#include "strata/type/Column.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr double Lattice = 16384.0;   // m between the places looked at
constexpr float Looked = 150.0f;      // m of rock under the surface that counts

// Share of the top of a column that is limestone or chalk of the dated cover: beds, not the marble
// of a basement. Nothing where the cover was folded: what is sought lies flat.
float carbonateShare(const Planet& planet, Vec2 point) {
    const StrataColumn column = strataAt(planet, point);
    if (column.foldSpan > QuietFoldSpan) return 0;
    float soluble = 0, at = 0;
    while (at < Looked) {
        const StrataUnit unit = unitAt(column, at, 0);
        const Bed bed = bedAt(planet.seed, column, unit, at, column.material);
        const float bottom = std::min(std::max(bed.bottom, at + 0.5f), Looked);
        if (unit.slot >= 0 && fissuredBySolution(definitionOf(bed.rock))) soluble += bottom - at;
        at = bottom + 0.01f;
    }
    return soluble / Looked;
}

} // namespace

Vec2 seekWaterPlace(World& world, const std::string& kind) {
    const Planet& planet = world.planet;
    const int side = int(PlanetSpan / Lattice);
    const bool hot = kind == "hot";
    Vec2 best{};
    double most = -1;
    std::string why;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const Vec2 at{(x + 0.5) * Lattice, (z + 0.5) * Lattice}, point = planetPoint(planet.seed, at);
            const double above = samplePlanet(planet.elevation, point, Kernel::Linear) - planet.seaLevel, rain = samplePlanet(planet.precipitation, point, Kernel::Linear);
            const double warmth = samplePlanet(planet.temperature, point, Kernel::Linear);
            if (above < 80 || rain < (hot ? 0.3 : 0.6) || warmth < 3) continue;
            char reason[256];
            double score;
            if (hot) {
                const Geotherm heat = geothermAt(planet, point);
                score = heat.gradient;
                std::snprintf(reason, sizeof reason, "%.0f K/km from below, %.0f %% of it magmatic, melt %.2f Myr ago; rain %.2f m/yr, %.0f m above the sea", 1000.0 * heat.gradient, 100.0 * heat.magmatic,
                              heat.sinceMelt / 1e6, rain, above);
            } else {
                // Thick carbonate standing high in the rain: where a water table can lie deep in it.
                const float share = carbonateShare(planet, point);
                if (share < 0.6f) continue;
                score = share * std::min(rain, 2.0) * std::clamp(above / 800.0, 0.2, 1.5);
                std::snprintf(reason, sizeof reason, "%.0f %% of the top %.0f m is flat-lying limestone or chalk; rain %.2f m/yr, %.0f m above the sea, %.0f C", 100.0 * share, double(Looked), rain, above, warmth);
            }
            if (score > most) most = score, best = at, why = reason;
        }
    std::printf("sought %s over the planet on a lattice of %.0f km: X %.0f Z %.0f | %s\n", hot ? "the hottest ground with rain" : "flat-lying limestone standing in the rain", Lattice / 1000, best.x, best.z,
                most < 0 ? "none found" : why.c_str());
    return best;
}

#include "Measure.hpp"

#include "inspect/fn/Inspect.hpp"
#include "landscape/fn/Forcing.hpp"
#include "material/constant/Materials.hpp"
#include "measure/constant/Angle.hpp"
#include "simulation/fn/Reader.hpp"
#include "simulation/fn/Spawn.hpp"
#include "strata/type/Column.hpp"

#include <cstdio>

using namespace derivean::sim;

// terra measure history --x --z [--ages 0,1e7,...] [--region 1]: one coordinate through the ages.
// The coordinate is fixed; the crust under it is whatever the plates have carried there.
int measureHistory(const Arguments& arguments) {
    const uint64_t seed = arguments.seed();
    Vec2 focus{arguments.number("x", 0), arguments.number("z", 0)};
    if (arguments.text("at", "") == "shore") {
        Workers workers;
        focus = findShore(evolvePlanet(seed, 4.6e9, workers), focus);
    }
    const bool region = arguments.number("region", 1) > 0;
    const std::string ages = arguments.text("ages", "0,1e7,1e8,5e8,1e9,2e9,4.6e9");
    std::printf("history  seed %llu  at %.0f %.0f\n", static_cast<unsigned long long>(seed), focus.x, focus.z);
    std::printf("      age | planet m   sea m  state | crust km  stock m  cover m  basement                | plate  mm/yr  deg | contact: km  regime       mm/yr | surface rock"
                " | region: ground m  water  cover m  bed\n");
    for (size_t start = 0; start < ages.size();) {
        const size_t end = std::min(ages.find(',', start), ages.size());
        const double age = std::strtod(ages.substr(start, end - start).c_str(), nullptr);
        start = end + 1;
        World world({seed, age, unsigned(arguments.number("threads", 0))});
        const InspectCrust crust = inspectCrust(world.planet, seed, focus);
        // The rock at the top of the layering as the planet left it, before any landscape cut into it.
        const StrataColumn strata = strataAt(world.planet, planetPoint(seed, focus));
        const Bed top = bedAt(seed, strata, unitAt(strata, 0, foldAt(seed, strata, strata.material)), 0, strata.material);
        const double heading = std::atan2(crust.velocity.x, -crust.velocity.z) * DegreesPerRadian;
        const float strongest = std::max({crust.compression, crust.extension, crust.shear});
        std::printf("%6.0f Myr | %8.0f %7.0f  %-5s | %8.1f %8.0f %8.0f  %-23s | %5d %6.1f %4.0f | %11.0f  %-11s %6.1f | %-12s", age / 1e6, crust.elevation,
                    crust.seaLevel, crust.elevation > crust.seaLevel ? "land" : "sea", crust.thickness / 1000, crust.stock, crust.cover,
                    nameOf(crust.basement), crust.plate, 1000 * length(crust.velocity), heading < 0 ? heading + 360 : heading, crust.contactDistance / 1000,
                    nameOf(crust.regime), 1000 * strongest, definitionOf(top.rock).name.data());
        if (region) {
            world.atlas.require(tileAt(Level::Region, focus));
            TileReader reader{world.atlas, Level::Region};
            SurfaceSample sample;
            readSurface(reader, focus, sample);
            std::printf(" | %15.0f  %-5s %8.1f  %s", sample.ground, sample.water > sample.ground ? (sample.water > world.planet.seaLevel + 0.5 ? "lake" : "sea") : "dry",
                        sample.ground - sample.rock, definitionOf(sample.bedrock).name.data());
        }
        std::printf("\n");
    }
    return 0;
}

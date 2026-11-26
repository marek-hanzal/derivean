#include "Arguments.hpp"

#include "atmosphere/type/Atmosphere.hpp"
#include "climate/type/Climate.hpp"
#include "simulation/fn/Spawn.hpp"
#include "simulation/type/World.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr int Dates = 2;
constexpr double Days[Dates] = {15, 197};   // northern midwinter and midsummer
constexpr int Belts = 8;
constexpr double Latitudes[Belts] = {75, 55, 35, 15, -15, -35, -55, -75};

// Mean snow line of the rows within 2.5° of a latitude, on both sides of the poles; cells without
// snow anywhere on their ladder count with its top.
double snowLineNear(const Atmosphere& air, double latitude) {
    double sum = 0, cells = 0;
    for (int z = 0; z < AirCells; ++z)
        for (int x = 0; std::abs(air.latitude[size_t(z)] - latitude) < 2.5 && x < AirCells; ++x) sum += snowLineOf(air, x, z), cells += 1;
    return cells > 0 ? sum / cells : 0.0;
}

} // namespace

// terra snow --seed n --age years [--threads n]: where the snow lies in midwinter and midsummer, by latitude and
// over the default landing.
int snowCommand(const Arguments& arguments) {
    const double age = arguments.number("age", 4.6e9);
    World world({arguments.seed(), age, unsigned(arguments.number("threads", 0))});
    const Vec2 landing = findLanding(world, {0, 0});
    const int x = wrapIndex(floorToInt(landing.x / AirCell), AirCells), z = wrapIndex(floorToInt(landing.z / AirCell), AirCells);
    Atmosphere air = startAtmosphere(world.planet, Calendar{age, 0}, world.workers);

    std::printf("landing at x %.0f z %.0f: weather cell %d %d, latitude %.1f, mean land height %.0f m, land share %.2f\n", landing.x, landing.z, x,
                z, air.latitude[size_t(z)], air.openWater.at(x, z) < 1 ? air.ground.at(x, z) / (1 - air.openWater.at(x, z)) : 0.0, 1 - air.openWater.at(x, z));
    std::printf("share of the cell's land per rung:");
    for (size_t rung = 0; rung < SnowRungs; ++rung) std::printf(" %.0fm %.2f", SnowLadder[rung], air.snowArea.at(x, z)[rung]);
    std::printf("\n");

    double line[Dates][Belts];
    for (int date = 0; date < Dates; ++date) {
        advanceAtmosphere(air, Calendar{age, Days[date] * DaySeconds}, world.workers);
        for (int band = 0; band < Belts; ++band) line[date][band] = snowLineNear(air, Latitudes[band]);
        std::printf("day %.0f over the landing: air %.1f C at sea level, snow line %.0f m, cell snow %.0f kg/m2 over %.0f %% of it; kg/m2 per rung:", Days[date],
                    air.airTemperature.at(x, z), snowLineOf(air, x, z), air.snow.at(x, z), 100 * air.snowCover.at(x, z));
        for (size_t rung = 0; rung < SnowRungs; ++rung) std::printf(" %.0f", air.snowLadder.at(x, z)[rung]);
        std::printf("\n  snowAt the landing, by height of the ground:");
        for (double height : {0.0, 500.0, 1000.0, 2000.0, 3000.0, 4500.0, 6000.0, 9000.0}) std::printf(" %.0fm %.0f", height, snowAt(air, landing, world.planet.seaLevel + height));
        std::printf("\n");
    }
    std::printf("snow line, m above the sea (9000 = none on the ladder)\nlatitude   day %.0f   day %.0f\n", Days[0], Days[1]);
    for (int band = 0; band < Belts; ++band) std::printf("%8.0f %8.0f %8.0f\n", Latitudes[band], line[0][band], line[1][band]);
    return 0;
}

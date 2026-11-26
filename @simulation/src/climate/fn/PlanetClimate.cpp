#include "climate/type/Climate.hpp"

#include "chronology/type/Calendar.hpp"
#include "climate/constant/Moisture.hpp"
#include "climate/type/MoistureBalance.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {

void solvePlanetClimate(Planet& planet, Workers& workers) {
    const MoistureBalance balance = solveMoisture(planet, workers);
    const float sea = float(planet.seaLevel);
    const float metresPerYear = float(YearSeconds / WaterDensity);
    workers.parallelFor(PlanetCells, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            const ZonalClimate zone = zonalClimate(planet.seed, planetCellCenter(0, z).z, planet.time);
            for (int x = 0; x < PlanetCells; ++x) {
                const float height = planet.elevation.at(x, z);
                planet.precipitation.at(x, z) = balance.precipitation.at(x, z) * metresPerYear;
                planet.temperature.at(x, z) = float(zone.temperature - LapseRate * std::max(0.0f, height - sea));
                planet.windX.at(x, z) = float(zone.wind.x);
                planet.windZ.at(x, z) = float(zone.wind.z);
            }
        }
    }, 32);
}

} // namespace derivean::sim

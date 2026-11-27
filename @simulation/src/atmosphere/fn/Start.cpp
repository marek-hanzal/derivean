#include "atmosphere/fn/Kernels.hpp"

#include "climate/fn/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {
namespace {

constexpr int InsolationSamples = 73;   // days of the year averaged for the annual-mean sunshine of a row
constexpr double HollowDepth = 300.0;   // m below its neighbours at which a cell pools cold air fully

void allocate(Atmosphere& atmosphere) {
    const AirField blank(AirCells, AirCells);
    for (AirField* field : {&atmosphere.ground, &atmosphere.climb, &atmosphere.hollow, &atmosphere.openWater, &atmosphere.surfaceGain,
                            &atmosphere.dragKeep, &atmosphere.absolute, &atmosphere.vorticity, &atmosphere.vorticityBefore,
                            &atmosphere.stream, &atmosphere.eddy, &atmosphere.faceX, &atmosphere.faceZ, &atmosphere.windX,
                            &atmosphere.windZ, &atmosphere.surfaceTemperature, &atmosphere.airTemperature,
                            &atmosphere.upperTemperature, &atmosphere.shade, &atmosphere.soil, &atmosphere.snow, &atmosphere.snowCover, &atmosphere.snowUsual,
                            &atmosphere.rainRate, &atmosphere.snowRate, &atmosphere.lift, &atmosphere.glide,
                            &atmosphere.work.potential, &atmosphere.work.bend, &atmosphere.work.before, &atmosphere.work.courantX,
                            &atmosphere.work.courantZ})
        *field = blank;
    for (AirLayers* layers : {&atmosphere.vapour, &atmosphere.liquid, &atmosphere.ice, &atmosphere.falling, &atmosphere.work.slack})
        for (AirField& field : *layers) field = blank;
    atmosphere.snowLadder = atmosphere.snowArea = Field<SnowColumn>(AirCells, AirCells);
    for (AirField& field : atmosphere.work.carried) field = blank;
    for (AirField& field : atmosphere.work.slope) field = blank;
    atmosphere.poisson = makePoissonSolver(AirCells, AirCell);
    shapePlume(atmosphere);
}

void describeRows(Atmosphere& atmosphere) {
    for (int z = 0; z < AirCells; ++z) {
        const ZonalClimate zone = zonalClimate(atmosphere.seed, (z + 0.5) * AirCell, atmosphere.era);
        atmosphere.latitude[size_t(z)] = zone.latitude;
        atmosphere.baseTemperature[size_t(z)] = zone.temperature;
        double sunshine = 0;
        for (int sample = 0; sample < InsolationSamples; ++sample)
            sunshine += dailyInsolation(zone.latitude, declinationOn((sample + 0.5) * YearDays / InsolationSamples));
        atmosphere.meanInsolation[size_t(z)] = sunshine / InsolationSamples;
        atmosphere.vertexLatitude[size_t(z)] = latitudeAt(z * AirCell);
        atmosphere.vertexNorth[size_t(z)] = northwardZ(z * AirCell);
    }
}

} // namespace

void setSurface(Atmosphere& atmosphere, const Field<float>& elevation, double seaLevel) {
    // The height field has to cover the planet with a whole number of its cells per air cell.
    const int perCell = elevation.width / AirCells;
    if (perCell < 1 || elevation.width != perCell * AirCells || elevation.height != elevation.width) return;
    atmosphere.seaLevel = seaLevel;
    double open = 0;
    for (int z = 0; z < AirCells; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            double height = 0;
            int wet = 0;
            for (int dz = 0; dz < perCell; ++dz) {
                for (int dx = 0; dx < perCell; ++dx) {
                    const double above = elevation.at(x * perCell + dx, z * perCell + dz) - seaLevel;
                    if (above > 0) height += above;
                    else ++wet;
                }
            }
            atmosphere.ground.at(x, z) = height / (perCell * perCell);
            atmosphere.climb.at(x, z) = terrainLift(atmosphere.ground.at(x, z));
            atmosphere.openWater.at(x, z) = double(wet) / (perCell * perCell);
            open += atmosphere.openWater.at(x, z);
        }
    }
    atmosphere.openArea = open * AirCellArea;
    for (int z = 0; z < AirCells; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            const double around = 0.25 * (atmosphere.ground.at(wrapAir(x - 1), z) + atmosphere.ground.at(wrapAir(x + 1), z) +
                                          atmosphere.ground.at(x, wrapAir(z - 1)) + atmosphere.ground.at(x, wrapAir(z + 1)));
            atmosphere.hollow.at(x, z) = saturate((around - atmosphere.ground.at(x, z)) / HollowDepth);
        }
    }
    shapeSnow(atmosphere, elevation, seaLevel);
    shapeCirculation(atmosphere);
    shapeHeat(atmosphere);
}

Atmosphere startAtmosphere(const Planet& planet, const Calendar& calendar, Workers& workers) {
    Atmosphere atmosphere;
    atmosphere.seed = planet.seed;
    atmosphere.era = calendar.era;
    atmosphere.step = int64_t(std::floor(calendar.seconds / AirStep)) - AirSpinUp;
    allocate(atmosphere);
    describeRows(atmosphere);
    setSurface(atmosphere, planet.elevation, planet.seaLevel);
    shapeHaze(atmosphere, planet);
    settleHeat(atmosphere);
    settleWater(atmosphere, planet);
    atmosphere.season = seasonOf(atmosphere);
    settleCirculation(atmosphere);
    for (int step = 0; step < AirSpinUp; ++step) stepAtmosphere(atmosphere, workers);
    return atmosphere;
}

} // namespace derivean::sim

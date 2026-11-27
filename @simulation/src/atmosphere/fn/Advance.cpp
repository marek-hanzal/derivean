#include "atmosphere/fn/Kernels.hpp"

#include "core/service/Workers.hpp"

namespace derivean::sim {
namespace {

constexpr int RowGrain = 16;    // rows per task of the column physics
constexpr int TaskGrain = 1;    // lets every task have a part of its own when threads are free: the circulation, task 0, is the longest
constexpr int EddyEvery = 4;    // wind steps between refreshes of the weather-system vorticity

} // namespace

// One AirStep. In each wind step the circulation and the carried fields only read the winds the
// wind step starts with, and each writes its own fields, so they run side by side; the column
// physics that follows writes row by row. No task reads what another one writes.
void stepAtmosphere(Atmosphere& atmosphere, Workers& workers) {
    if (atmosphere.step % AirStepsPerDay == 0) atmosphere.season = seasonOf(atmosphere);
    prepareWater(atmosphere);

    AirWork& work = atmosphere.work;
    work.before.cells = atmosphere.airTemperature.cells;
    // Task 0 is the circulation; the others carry one field each: the two temperatures, then
    // vapour, liquid and ice of every band, then every kind of haze in the low band.
    constexpr int WeatherFields = 2 + 3 * AirBands;
    AirField* carried[WeatherFields] = {&atmosphere.airTemperature, &atmosphere.upperTemperature};
    Haze& haze = atmosphere.haze;
    AirLayers* const water[3] = {&atmosphere.vapour, &atmosphere.liquid, &atmosphere.ice};
    for (int kind = 0; kind < 3; ++kind)
        for (int band = 0; band < AirBands; ++band) carried[2 + kind * AirBands + band] = &(*water[kind])[size_t(band)];

    for (int part = 0; part < WindSteps; ++part) {
        prepareTransport(atmosphere);
        const bool eddies = (atmosphere.step * WindSteps + part + 1) % EddyEvery == 0;
        workers.parallelFor(1 + WeatherFields + HazeKinds, [&](int begin, int end) {
            for (int task = begin; task < end; ++task) {
                if (task == 0) {
                    stepCirculation(atmosphere);
                    refreshWind(atmosphere, eddies);
                    continue;
                }
                const int field = task - 1;
                if (field >= WeatherFields) {
                    const size_t kind = size_t(field - WeatherFields);
                    advect(atmosphere, haze.mass[kind], haze.work[2 * kind], haze.work[2 * kind + 1], Low, Carried::Vapour);
                    continue;
                }
                const int band = field == 0 ? Low : field == 1 ? Middle : (field - 2) % AirBands;
                const Carried kind = field < 2 ? Carried::Temperature : field < 2 + AirBands ? Carried::Vapour : Carried::Cloud;
                advect(atmosphere, *carried[field], work.carried[size_t(field)], work.slope[size_t(field)], band, kind);
            }
        }, TaskGrain);
        for (int field = 0; field < WeatherFields; ++field) std::swap(carried[field]->cells, work.carried[size_t(field)].cells);
        for (size_t kind = 0; kind < HazeKinds; ++kind) std::swap(haze.mass[kind].cells, haze.work[2 * kind].cells);
    }

    // Row by row: a row's water reads what its heat has just written while it is still at hand.
    // Each cell reads only its own column of what the other writes.
    workers.parallelFor(AirCells, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            stepHeat(atmosphere, z, z + 1);
            stepWater(atmosphere, z, z + 1);
        }
    }, RowGrain);
    closeWater(atmosphere);
    stepPlume(atmosphere, workers);
    stepHaze(atmosphere, workers);
    ++atmosphere.step;
}

void advanceAtmosphere(Atmosphere& atmosphere, const Calendar& now, Workers& workers) {
    const int64_t target = int64_t(std::floor(now.seconds / AirStep));
    while (atmosphere.step < target) stepAtmosphere(atmosphere, workers);
}

} // namespace derivean::sim

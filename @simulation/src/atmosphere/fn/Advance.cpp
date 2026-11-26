#include "atmosphere/fn/Kernels.hpp"

#include "core/service/Workers.hpp"

namespace derivean::sim {
namespace {

constexpr int RowGrain = 16;    // rows per task of the column physics
constexpr int TaskGrain = 3;    // carried fields per task: few larger tasks cost less than many small ones
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
    // vapour, liquid and ice of every band.
    AirField* carried[2 + 3 * AirBands] = {&atmosphere.airTemperature, &atmosphere.upperTemperature};
    AirLayers* const water[3] = {&atmosphere.vapour, &atmosphere.liquid, &atmosphere.ice};
    for (int kind = 0; kind < 3; ++kind)
        for (int band = 0; band < AirBands; ++band) carried[2 + kind * AirBands + band] = &(*water[kind])[size_t(band)];

    for (int part = 0; part < WindSteps; ++part) {
        prepareTransport(atmosphere);
        const bool eddies = (atmosphere.step * WindSteps + part + 1) % EddyEvery == 0;
        workers.parallelFor(3 + 3 * AirBands, [&](int begin, int end) {
            for (int task = begin; task < end; ++task) {
                if (task == 0) {
                    stepCirculation(atmosphere);
                    refreshWind(atmosphere, eddies);
                    continue;
                }
                const int field = task - 1;
                const int band = field == 0 ? Low : field == 1 ? Middle : (field - 2) % AirBands;
                const Carried kind = field < 2 ? Carried::Temperature : field < 2 + AirBands ? Carried::Vapour : Carried::Cloud;
                advect(atmosphere, *carried[field], work.carried[size_t(field)], work.slope[size_t(field)], band, kind);
            }
        }, TaskGrain);
        for (int field = 0; field < 2 + 3 * AirBands; ++field) std::swap(carried[field]->cells, work.carried[size_t(field)].cells);
    }

    workers.parallelFor(AirCells, [&](int begin, int end) {
        stepHeat(atmosphere, begin, end);
        stepWater(atmosphere, begin, end);
    }, RowGrain);
    closeWater(atmosphere);
    stepPlume(atmosphere, workers);
    ++atmosphere.step;
}

void advanceAtmosphere(Atmosphere& atmosphere, const Calendar& now, Workers& workers) {
    const int64_t target = int64_t(std::floor(now.seconds / AirStep));
    while (atmosphere.step < target) stepAtmosphere(atmosphere, workers);
}

} // namespace derivean::sim

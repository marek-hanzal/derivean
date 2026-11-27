#include "planet/type/Planet.hpp"

#include "climate/type/Climate.hpp"
#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t WaterStream = 0x57415452;
constexpr int SeaLevelEvery = 4;     // planet steps
constexpr int ClimateEvery = 16;

void allocate(Planet& planet) {
    const int n = PlanetCells;
    planet.columns = Field<CrustColumn>(n, n);
    planet.records = Field<ColumnRecord>(n, n);
    planet.elevation = Field<float>(n, n);
    planet.swell = Field<float>(n, n);
    planet.erosion = Field<float>(n, n);
    planet.deposition = Field<float>(n, n);
    planet.temperature = Field<float>(n, n);
    planet.precipitation = Field<float>(n, n, 0.8f);
    planet.windX = Field<float>(n, n);
    planet.windZ = Field<float>(n, n);
    planet.owner = Field<uint16_t>(n, n);
    planet.contact = Field<ContactCell>(n, n);
}

// The cooling body has a closed-form past, so its marks are evaluated rather than remembered.
void fillPrimordial(Planet& planet, double time, Workers& workers) {
    const double newest = std::floor(time / RegionAnchor) * RegionAnchor;
    workers.parallelFor(PlanetCells, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < PlanetCells; ++x) {
                const Vec2 center = planetCellCenter(x, z);
                planet.columns.at(x, z) = primordialColumn(planet.seed, center, time);
                for (int back = 0; back < PastMarks; ++back) {
                    const double anchor = std::max(0.0, newest - back * RegionAnchor);
                    const CrustColumn then = primordialColumn(planet.seed, center, anchor);
                    planet.records.at(x, z).past[size_t(pastMarkIndex(anchor))] = {columnElevation(then), then.crust, 0.0f, 0.0f, 0.0f};
                }
            }
        }
    }, 8);
}

void setWater(Planet& planet) {
    planet.surfaceKelvin = coolingSurface(planet.time);
    planet.surfaceWater = std::max(0.0, planet.ledger.waterInventory * condensedShare(planet.time));
}

} // namespace

Planet evolvePlanet(uint64_t seed, double time, Workers& workers) {
    Planet planet;
    planet.seed = seed;
    allocate(planet);
    planet.ledger.waterInventory = PlanetSpan * PlanetSpan * 4200.0 * ranged(hash(stream(seed, WaterStream), 1), 0.95, 1.05);

    planet.time = std::min(time, TectonicsStart);
    fillPrimordial(planet, planet.time, workers);
    planet.epoch = epochAt(seed, TectonicsStart);
    enterEpoch(planet, workers);
    setWater(planet);
    refreshElevation(planet);
    solveSeaLevel(planet);

    const int64_t steps = time < TectonicsStart ? 0 : int64_t((time - TectonicsStart) / PlanetStep);
    planet.seaLevels.reserve(size_t(steps) + 1);
    for (int64_t step = 0; step < steps; ++step) {
        const double before = planet.time;
        if (before >= planet.epoch.end) enterEpoch(planet, workers);
        if (strataSlot(before) != strataSlot(before - PlanetStep)) {
            for (ColumnRecord& record : planet.records.cells) {
                retireOldest(record.cover, before);
                buryCover(record.cover, before);
            }
        }

        setWater(planet);
        movePlates(planet, PlanetStep);
        actOnContacts(planet, PlanetStep);
        relaxThickCrust(planet, PlanetStep);
        for (CrustColumn& column : planet.columns.cells) column.oceanAge += float(PlanetStep);

        refreshElevation(planet);
        if (step % SeaLevelEvery == 0) solveSeaLevel(planet);
        if (step % ClimateEvery == 0) solvePlanetClimate(planet, workers);
        shapeSurface(planet, PlanetStep, workers);

        planet.time = TectonicsStart + double(step + 1) * PlanetStep;
        planet.seaLevels.push_back(float(planet.seaLevel));
        if (std::fmod(planet.time, RegionAnchor) == 0) {
            refreshElevation(planet);
            stampMarks(planet);
        }
    }

    setWater(planet);
    refreshElevation(planet);
    solveSeaLevel(planet);
    solvePlanetClimate(planet, workers);
    refreshMeltHeat(planet);
    for (ColumnRecord& record : planet.records.cells) buryCover(record.cover, planet.time);
    return planet;
}

} // namespace derivean::sim

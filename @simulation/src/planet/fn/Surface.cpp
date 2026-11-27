#include "planet/type/Planet.hpp"

#include "climate/type/Climate.hpp"

#include <array>
#include <vector>

namespace derivean::sim {
namespace {

constexpr double Denudation = 9 * Myr;         // e-folding of crust removal per metre above the sea
constexpr double SteepHeight = 2500.0;         // m above the sea at which removal has doubled
constexpr int Hops = 3;                        // cells a load may travel in one planet step
constexpr float WaveBase = 25.0f;              // m of water a shelf keeps above it
constexpr float ShelfDepth = 250.0f;
constexpr float SoluteShare = 0.07f;           // share of eroded rock that leaves in solution instead of as grains
constexpr float CoolShare = 0.35f;             // pull of a cold shelf on the solute against a warm one's
constexpr float PlatformDepth = 70.0f;         // m of water a carbonate platform keeps above it
constexpr double WarmLatitude = 45.0;          // degrees from the equator within which shelf water is warm
constexpr float StepFill = 1500.0f;            // m one cell may take in a single hop

constexpr int Offsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}};

float depthOfLid(float ageYears) {
    return -2600.0f - 350.0f * std::sqrt(std::min(ageYears, float(100 * Myr)) / float(Myr));
}

// Deposits by environment. Clastic supply dilutes the carbonate a warm shelf would otherwise grow.
Facies faciesOf(float waterDepth, float slope, float supply, float rain) {
    if (waterDepth <= 0) {
        if (rain < 0.3f && slope < 0.0008f) return Facies{5, 110, 80, 0, 0, 60};
        const uint8_t gravel = uint8_t(std::clamp(40.0f + 30000.0f * slope, 20.0f, 150.0f));
        return Facies{gravel, uint8_t(165 - gravel / 2), uint8_t(90 - gravel / 2), 0, 0, 0};
    }
    if (waterDepth > ShelfDepth) return Facies{0, 30, 210, 15, 0, 0};
    // Waves winnow the mud out of a well-supplied shoreface: sheets of clean, massive sand.
    if (waterDepth < 60.0f && supply >= 25.0f) return Facies{6, 236, 13, 0, 0, 0};
    if (supply < 150.0f) return Facies{0, 125, 130, 0, 0, 0};
    return Facies{15, 150, 90, 0, 0, 0};
}

} // namespace

float columnElevation(const CrustColumn& column) {
    const float share = float(smoothstep(OceanicLimit, ContinentalLimit, column.crust));
    const float continental = float(IsostaticRise * (double(column.crust) - CrustAtDatum));
    return float(mix(depthOfLid(column.oceanAge), continental, share));
}

void refreshElevation(Planet& planet) {
    const float support = float(epochActivity(planet.epoch, planet.time));
    for (size_t i = 0; i < planet.columns.size(); ++i)
        planet.elevation.cells[i] = columnElevation(planet.columns.cells[i]) + planet.swell.cells[i] * support;
}

void solveSeaLevel(Planet& planet) {
    // Histogram of basin shape: find the level whose volume below it equals the surface water.
    constexpr int Bins = 2400;
    constexpr float Low = -9000.0f, Width = 7.5f;
    std::array<uint32_t, Bins> histogram{};
    for (float height : planet.elevation.cells) ++histogram[size_t(std::clamp(int((height - Low) / Width), 0, Bins - 1))];
    double volume = 0, cellsBelow = 0;
    planet.seaLevel = Low;
    for (int bin = 0; bin < Bins; ++bin) {
        const double slab = cellsBelow * PlanetCellArea * Width;
        if (volume + slab >= planet.surfaceWater) {
            const double rise = cellsBelow > 0 ? (planet.surfaceWater - volume) / (cellsBelow * PlanetCellArea) : 0.0;
            planet.seaLevel = Low + bin * Width + rise;
            return;
        }
        volume += slab;
        cellsBelow += histogram[size_t(bin)];
        planet.seaLevel = Low + (bin + 1) * Width;
    }
}

void shapeSurface(Planet& planet, double dt, Workers& workers) {
    const float sea = float(planet.seaLevel);
    const float memory = float(std::exp(-dt / (10 * Myr)));
    const bool rains = planet.surfaceWater > 0;
    Field<int32_t> target(PlanetCells, PlanetCells, -1);
    Field<float> slope(PlanetCells, PlanetCells, 0.0f);
    std::array<bool, PlanetCells> warmRow{};
    for (int z = 0; z < PlanetCells; ++z) warmRow[size_t(z)] = std::abs(latitudeAt(planetCellCenter(0, z).z)) < WarmLatitude;

    // Running water removes crust faster the higher and wetter the ground; a dry body keeps its relief.
    // Each row keeps its own sums and the rows are added in order afterwards, so the totals do not
    // depend on how the rows were shared among threads or which finished first.
    std::vector<double> rowEroded(PlanetCells, 0.0), rowSolute(PlanetCells, 0.0);
    workers.parallelFor(PlanetCells, [&](int begin, int end) {
        const int n = PlanetCells;
        for (int z = begin; z < end; ++z) {
            double partial = 0, dissolved = 0;
            for (int x = 0; x < n; ++x) {
                const size_t cell = size_t(z) * n + size_t(x);
                CrustColumn& column = planet.columns.cells[cell];
                const float height = planet.elevation.cells[cell];
                float lowest = height;
                for (const auto& offset : Offsets) {
                    int nx = x + offset[0], nz = z + offset[1];
                    nx = nx < 0 ? nx + n : nx >= n ? nx - n : nx;
                    nz = nz < 0 ? nz + n : nz >= n ? nz - n : nz;
                    const size_t neighbor = size_t(nz) * n + size_t(nx);
                    const float other = planet.elevation.cells[neighbor];
                    if (other < lowest) {
                        lowest = other;
                        target.cells[cell] = int32_t(neighbor);
                    }
                }
                slope.cells[cell] = (height - lowest) / float(PlanetCell);
                float removed = 0;
                if (rains && height > sea) {
                    const double above = height - sea;
                    const double wetness = 0.35 + 0.65 * std::min(1.5f, planet.precipitation.cells[cell]);
                    removed = std::min(column.crust, float(above * (1 + above / SteepHeight) * wetness * dt / Denudation));
                    const float fromCover = stripCover(planet.records.cells[cell].cover, planet.time, removed);
                    column.exhumed += removed - fromCover;
                    column.crust -= removed;
                    column.eroded += removed;
                    column.load += removed * (1.0f - SoluteShare);
                    dissolved += removed * SoluteShare;
                    partial += removed;
                }
                planet.erosion.cells[cell] = planet.erosion.cells[cell] * memory + (1 - memory) * removed / float(dt);
            }
            rowEroded[size_t(z)] = partial;
            rowSolute[size_t(z)] = dissolved;
        }
    }, 16);
    double eroded = 0, solute = 0;
    for (int z = 0; z < PlanetCells; ++z) eroded += rowEroded[size_t(z)], solute += rowSolute[size_t(z)];
    planet.ledger.eroded += eroded * PlanetCellArea;

    // Loads walk downhill a few cells per step, settling in basins, on plains and under the sea.
    Field<float> laid(PlanetCells, PlanetCells, 0.0f);
    Field<float> arriving(PlanetCells, PlanetCells, 0.0f);
    double deposited = 0;
    for (int hop = 0; hop <= Hops; ++hop) {
        std::fill(arriving.cells.begin(), arriving.cells.end(), 0.0f);
        for (size_t cell = 0; cell < planet.columns.size(); ++cell) {
            CrustColumn& column = planet.columns.cells[cell];
            if (column.load <= 0) continue;
            const float height = planet.elevation.cells[cell] + laid.cells[cell] * float(IsostaticRise);
            const float water = sea - height;
            const int32_t next = target.cells[cell];
            float settle;
            if (hop == Hops || next < 0) settle = column.load;
            else if (water > 0) settle = std::min({column.load, std::max(0.0f, (water - WaveBase) / float(IsostaticRise)), StepFill});
            else settle = column.load * 0.3f * float(smoothstep(0.004, 0.0005, slope.cells[cell]));
            if (settle > 0) {
                addDeposit(planet.records.cells[cell].cover, planet.time, settle,
                           faciesOf(water, slope.cells[cell], column.load, planet.precipitation.cells[cell]));
                column.crust += settle;
                column.deposited += settle;
                laid.cells[cell] += settle;
                deposited += settle;
            }
            if (next >= 0) arriving.cells[size_t(next)] += column.load - settle;
            column.load = 0;
        }
        for (size_t cell = 0; cell < arriving.size(); ++cell) planet.columns.cells[cell].load += arriving.cells[cell];
    }
    planet.ledger.deposited += deposited * PlanetCellArea;

    // What rivers brought in solution comes out again as carbonate within the step. It is shared
    // among the shelves by the room their water leaves and by warmth (cold water holds its
    // carbonate); only what the shelves cannot take settles as calcareous mud on the deep floor.
    // Clastic deposits of the same step share the slot, so a muddy shelf gets marl and a clear one limestone.
    const bool temperate = planet.surfaceKelvin < 330;
    const auto room = [&](size_t cell) {
        const float water = sea - planet.elevation.cells[cell];
        return !temperate || water <= PlatformDepth || water >= ShelfDepth ? 0.0f : (water - PlatformDepth) / float(IsostaticRise);
    };
    const auto pull = [&](size_t cell) { return room(cell) * (warmRow[cell / size_t(PlanetCells)] ? 1.0f : CoolShare); };
    double pulling = 0, shelfRoom = 0;
    size_t deepCells = 0;
    for (size_t cell = 0; cell < planet.columns.size(); ++cell) {
        pulling += pull(cell);
        shelfRoom += room(cell);
        deepCells += sea - planet.elevation.cells[cell] >= ShelfDepth;
    }
    const double perPull = pulling > 0 ? std::min(solute, shelfRoom) / pulling : 0.0;
    double onShelves = 0;
    for (size_t cell = 0; cell < planet.columns.size(); ++cell) onShelves += std::min(double(room(cell)), perPull * pull(cell));
    const float deepLayer = float(std::max(0.0, solute - onShelves) / double(std::max<size_t>(1, deepCells)));
    double grownTotal = 0;
    for (size_t cell = 0; cell < planet.columns.size(); ++cell) {
        const bool deep = sea - planet.elevation.cells[cell] >= ShelfDepth;
        const float grown = deep ? deepLayer : std::min(room(cell), float(perPull * pull(cell)));
        if (grown > 0) {
            CrustColumn& column = planet.columns.cells[cell];
            addDeposit(planet.records.cells[cell].cover, planet.time, grown, deep ? Facies{0, 0, 60, 195, 0, 0} : Facies{0, 5, 20, 230, 0, 0});
            column.crust += grown;
            column.deposited += grown;
            laid.cells[cell] += grown;
            grownTotal += grown;
        }
        planet.deposition.cells[cell] = planet.deposition.cells[cell] * memory + (1 - memory) * laid.cells[cell] / float(dt);
    }
    planet.ledger.precipitated += grownTotal * PlanetCellArea;
    planet.ledger.deposited += grownTotal * PlanetCellArea;
}

void stampMarks(Planet& planet) {
    const size_t slot = size_t(pastMarkIndex(planet.time));
    for (size_t cell = 0; cell < planet.columns.size(); ++cell) {
        const CrustColumn& column = planet.columns.cells[cell];
        planet.records.cells[cell].past[slot] = {planet.elevation.cells[cell], column.crust, column.tectonicGain, column.eroded, column.deposited, column.melt, column.erupted};
    }
}

} // namespace derivean::sim

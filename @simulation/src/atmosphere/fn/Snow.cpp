#include "atmosphere/fn/Cover.hpp"
#include "atmosphere/fn/Kernels.hpp"

#include "climate/fn/Seasons.hpp"
#include "climate/fn/Snowpack.hpp"
#include "planet/type/Planet.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

// Hypsometry of a weather cell, from the planet alone. The planet knows the mean height of 65 km
// cells; ground inside them reaches further both ways. The cell's land is taken to be spread as a
// triangle whose mean is the mean height of the cell's own land. Its floor and ceiling are
// stretched beyond the lowest and highest ground the planet shows in the cell and at its borders
// (halfway to each neighbouring planet cell); where the mean cannot be kept between them, the far
// end comes in.
constexpr double PeakStretch = 2.0;      // ceiling over the mean, in units of the highest such ground over the mean
constexpr double ValleyStretch = 1.5;    // floor under the mean, in units of the mean over the lowest such ground
constexpr double LeastRelief = 200.0;    // m every cell has on either side of its mean

constexpr double PackLimit = 3000.0;     // kg/m² a pack holds; more than that leaves as ice, and ice bodies are the landscape's
constexpr double Sublimation = 0.3;      // snow gives up vapour at this share of a wet surface's rate
constexpr double BareMass = 1.0;         // kg/m² below which ground counts as bare for the snow line
constexpr int SettlingSteps = 73;        // five-day steps of the year that lays the first pack

// Share of a triangular spread between `low` and `high`, peaked at `mode`, that lies below `height`.
double shareBelow(double height, double low, double mode, double high) {
    if (height <= low) return 0.0;
    if (height >= high) return 1.0;
    return height <= mode ? (height - low) * (height - low) / ((high - low) * (mode - low))
                          : 1.0 - (high - height) * (high - height) / ((high - low) * (high - mode));
}

// m above the sea at which a ladder's snow begins. Between the highest bare rung and the first
// one under snow, the pack is taken to thin out at the rate it does between the two rungs above.
double lineOf(const SnowColumn& packs) {
    size_t first = 0;
    while (first < SnowRungs && packs[first] <= BareMass) ++first;
    if (first == SnowRungs) return SnowLadder.back();
    if (first == 0) return SnowLadder.front();
    const double top = SnowLadder[first], gap = top - SnowLadder[first - 1];
    if (first + 1 == SnowRungs || packs[first + 1] <= packs[first]) return top - 0.5 * gap;
    return top - std::min(gap, packs[first] * (SnowLadder[first + 1] - top) / (packs[first + 1] - packs[first]));
}

// kg/m² on ground `above` metres over the sea in one cell: nothing below its snow line, linear
// from there to the first rung under snow and between rungs above it.
double packAt(const SnowColumn& packs, double above) {
    const double line = lineOf(packs);
    if (above < line || line >= SnowLadder.back()) return above >= SnowLadder.back() ? packs[SnowRungs - 1] : 0.0;
    size_t upper = 1;
    while (upper + 1 < SnowRungs && SnowLadder[upper] < above) ++upper;
    const double low = SnowLadder[upper - 1], high = SnowLadder[upper];
    if (line > low) return high > line ? packs[upper] * (above - line) / (high - line) : packs[upper];
    return mix(packs[upper - 1], packs[upper], (above - low) / (high - low));
}

} // namespace

void weighSnow(Atmosphere& atmosphere, size_t cell) {
    const SnowColumn& packs = atmosphere.snowLadder.cells[cell];
    const SnowColumn& areas = atmosphere.snowArea.cells[cell];
    double stored = 0, covered = 0;
    for (size_t rung = 0; rung < SnowRungs; ++rung) {
        stored += areas[rung] * packs[rung];
        covered += areas[rung] * std::min(1.0, packs[rung] * (1.0 / SnowCoverMass));
    }
    const double land = 1.0 - atmosphere.openWater.cells[cell];
    atmosphere.snow.cells[cell] = land * stored;
    atmosphere.snowCover.cells[cell] = land * covered;
}

void shapeSnow(Atmosphere& atmosphere, const Field<float>& elevation, double seaLevel) {
    const int perCell = elevation.width / AirCells, n = elevation.width;
    const auto above = [&](int x, int z) { return std::max(0.0, double(elevation.at(wrapIndex(x, n), wrapIndex(z, n))) - seaLevel); };
    constexpr int Around[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    double credit = 0;
    for (int z = 0; z < AirCells; ++z) {
        const ZonalClimate zone = zonalClimate(atmosphere.seed, (z + 0.5) * AirCell, atmosphere.era);
        const double warmest = zone.temperature + zone.seasonal;   // °C at sea level in the warmest month
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = atmosphere.ground.index(x, z);
            SnowColumn& areas = atmosphere.snowArea.cells[cell];
            areas.fill(0.0);
            double lowest = 1e9, highest = 0, sum = 0;
            int land = 0;
            for (int dz = 0; dz < perCell; ++dz) {
                for (int dx = 0; dx < perCell; ++dx) {
                    const int px = x * perCell + dx, pz = z * perCell + dz;
                    const double own = above(px, pz);
                    if (own > 0) sum += own, ++land;
                    for (const auto& step : Around) {
                        const double edge = 0.5 * (own + above(px + step[0], pz + step[1]));
                        lowest = std::min(lowest, edge);
                        highest = std::max(highest, edge);
                    }
                }
            }
            double perennial = 0;
            if (land > 0) {
                const double mean = sum / land;
                double low = std::max(0.0, mean - std::max(LeastRelief, ValleyStretch * (mean - lowest)));
                double high = mean + std::max(LeastRelief, PeakStretch * (highest - mean));
                double mode = 3 * mean - low - high;
                if (mode < low) high = 3 * mean - 2 * low, mode = low;
                else if (mode > high) low = 3 * mean - 2 * high, mode = high;
                double under = 0;
                for (size_t rung = 0; rung < SnowRungs; ++rung) {
                    // A rung stands for the ground from halfway down to the rung below to halfway up to the next.
                    const double upTo = rung + 1 < SnowRungs ? shareBelow(0.5 * (SnowLadder[rung] + SnowLadder[rung + 1]), low, mode, high) : 1.0;
                    areas[rung] = upTo - under;
                    under = upTo;
                    perennial += areas[rung] * saturate(0.5 - 0.25 * (warmest - LapseRate * SnowLadder[rung]));
                }
            }
            atmosphere.snowUsual.cells[cell] = (1.0 - atmosphere.openWater.cells[cell]) * perennial;

            // The same packs weighed by new areas add up differently. Ground that newly counts brings
            // no water with it: its packs are thinned until the cell holds what it held. What a cell
            // holds less than before goes to the ocean.
            const double before = atmosphere.snow.cells[cell];
            weighSnow(atmosphere, cell);
            const double after = atmosphere.snow.cells[cell];
            if (after > before) {
                SnowColumn& packs = atmosphere.snowLadder.cells[cell];
                for (size_t rung = 0; rung < SnowRungs; ++rung)
                    if (areas[rung] > 0) packs[rung] *= before / after;
                weighSnow(atmosphere, cell);
            }
            credit += before - atmosphere.snow.cells[cell];
        }
    }
    atmosphere.ocean += credit * AirCellArea;
}

void settleSnow(Atmosphere& atmosphere, const Planet& planet) {
    constexpr int PerCell = PlanetCells / AirCells;
    const double startDay = double(atmosphere.step) * AirStep / DaySeconds - YearDays, span = YearDays / SettlingSteps;
    for (int z = 0; z < AirCells; ++z) {
        const ZonalClimate zone = zonalClimate(atmosphere.seed, (z + 0.5) * AirCell, atmosphere.era);
        double seaLevelAir[SettlingSteps], coldest = 1e9;
        for (int step = 0; step < SettlingSteps; ++step) {
            seaLevelAir[step] = seasonalTemperature(zone, 0.0, (startDay + (step + 0.5) * span) / YearDays);
            coldest = std::min(coldest, seaLevelAir[step]);
        }
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = atmosphere.ground.index(x, z);
            double yearly = 0;   // kg/m²
            for (int dz = 0; dz < PerCell; ++dz)
                for (int dx = 0; dx < PerCell; ++dx) yearly += WaterDensity * planet.precipitation.at(PerCell * x + dx, PerCell * z + dz) / (PerCell * PerCell);
            for (size_t rung = 0; rung < SnowRungs; ++rung) {
                const double cooler = LapseRate * SnowLadder[rung];
                const bool snows = yearly > 0 && coldest - cooler < SnowLine + SnowBlend;   // else no day of the year is cold enough
                double pack = 0;
                for (int step = 0; snows && step < SettlingSteps; ++step)
                    pack = std::min(PackLimit, stepSnowpack(pack, seaLevelAir[step] - cooler, yearly / SettlingSteps, span).swe);
                atmosphere.snowLadder.cells[cell][rung] = pack;
            }
            weighSnow(atmosphere, cell);
        }
    }
}

SnowYield stepSnow(Atmosphere& atmosphere, size_t cell, double seaLevelAir, double falling, double thirst) {
    SnowYield yield;
    SnowColumn& packs = atmosphere.snowLadder.cells[cell];
    const SnowColumn& areas = atmosphere.snowArea.cells[cell];
    const double days = AirStep / DaySeconds, perCover = 1.0 / SnowCoverMass, reach = thirst * Sublimation;
    double stored = 0, covered = 0;
    for (size_t rung = 0; rung < SnowRungs; ++rung) {
        // A bare rung that nothing falls on stays bare and adds ±0 to sums that start at +0, which
        // leaves them as they are, for air that is not NaN and finite areas.
        if (falling == 0 && packs[rung] == 0 && !std::signbit(packs[rung])) continue;
        const SnowpackChange change = stepSnowpack(packs[rung], seaLevelAir - LapseRate * SnowLadder[rung], falling, days);
        const double sublimed = std::min(change.swe, reach * std::min(1.0, change.swe * perCover));
        const double kept = std::min(change.swe - sublimed, PackLimit);
        const double area = areas[rung];
        packs[rung] = kept;
        yield.snowfall += area * change.snowfall;
        yield.melt += area * change.melt;
        yield.sublimed += area * sublimed;
        yield.shed += area * (change.swe - sublimed - kept);
        stored += area * kept;
        covered += area * std::min(1.0, kept * perCover);
    }
    const double land = 1.0 - atmosphere.openWater.cells[cell];
    atmosphere.snow.cells[cell] = land * stored;
    atmosphere.snowCover.cells[cell] = land * covered;
    return yield;
}

double snowAt(const Atmosphere& atmosphere, Vec2 position, double elevation) {
    const double above = std::clamp(elevation - atmosphere.seaLevel, SnowLadder.front(), SnowLadder.back());
    const double x = position.x / AirCell - 0.5, z = position.z / AirCell - 0.5;
    const double floorX = std::floor(x), floorZ = std::floor(z);
    const int west = wrapAir(int(floorX)), east = wrapAir(west + 1), north = wrapAir(int(floorZ)), south = wrapAir(north + 1);
    const auto pack = [&](int cellX, int cellZ) { return packAt(atmosphere.snowLadder.at(cellX, cellZ), above); };
    return mix(mix(pack(west, north), pack(east, north), x - floorX), mix(pack(west, south), pack(east, south), x - floorX), z - floorZ);
}

double snowLineOf(const Atmosphere& atmosphere, int x, int z) { return lineOf(atmosphere.snowLadder.at(x, z)); }

} // namespace derivean::sim

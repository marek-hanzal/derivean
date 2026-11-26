#include "climate/type/MoistureBalance.hpp"

#include "chronology/type/Calendar.hpp"
#include "climate/fn/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "planet/type/Planet.hpp"

// 265 lines: one solver. The grid description, its coarsening and the sweep share the Grid layout
// and the source law, and none of them means anything without the other two.
namespace derivean::sim {
namespace {

constexpr double EddyMixing = 1.0e5;   // m²/s; what weather systems do to vapour in the annual mean
constexpr double EddyWind = 6.0;       // m/s the systems add to the speed of the belts
constexpr int Levels = 3;              // grids from a quarter of the planet's resolution up to its own
constexpr int Sweeps[Levels] = {12, 4, 2};   // coarsest first
constexpr double StartHumidity = 0.7;
constexpr double WettestYear = 4000.0 / YearSeconds;   // kg/m²/s that the yearly mean rain of a column approaches but never reaches

// Where the stepped atmosphere lifts a column for a while, the steady budget gives the column a
// share of the year in each kind of lifting and averages the same condensation law over them.
constexpr int Liftings = 3;
constexpr double QuietSpell = DaySeconds;   // s a column takes to shed its excess when nothing lifts it
constexpr double StormShare = 0.3;     // share of the year a storm-track column spends inside a weather system
constexpr double StormLift = 700.0;    // m the system lifts it by
constexpr double ShowerShare = 0.02;   // share of the year a column over water spends under deep convection
constexpr double LandShowers = 0.06;   // the same over land, which the sun heats into afternoon showers
constexpr double ShowerLift = 1500.0;  // m
constexpr double PerEpisode[Liftings] = {1.0 / QuietSpell, 1.0 / LiftTime, 1.0 / CondensationTime};   // 1/s: one visit of each kind

struct Lifting {
    double share[Liftings];         // of the year, over water
    double perCapacity[Liftings];   // 1 / share of the unlifted capacity the column keeps
};

// The budget on one grid. Coarse grids only exist to bring the fine one close to its steady state.
struct Grid {
    int size{};
    double cell{};
    std::vector<double> open, air, capacity, perCapacity;    // per cell: water share, kg/m³, kg/m², m²/kg
    std::vector<double> exchange, skin, thaw, east, north;   // per row; `thaw` is the ice-free share of open water, `north` the wind through the row's north face
    std::vector<Lifting> lifting;                            // per row
    std::vector<double> vapour, rain;

    size_t at(int x, int z) const { return size_t(z) * size_t(size) + size_t(x); }
};

struct Source {
    double evaporation{}, rain{}, slope{};   // kg/m²/s, and the change of (evaporation − rain) per kg/m² of vapour
};

// Evaporation and time-mean condensation of one column; `wetting` is the rain that keeps its ground wet.
Source sourceOf(const Grid& grid, size_t cell, int z, double vapour, double wetting) {
    Source source;
    const double air = grid.air[cell], open = grid.open[cell];
    const double held = surfaceVapour(vapour, air);
    const double reach = grid.exchange[size_t(z)];
    if (open > 0) {
        const double deficit = grid.skin[size_t(z)] - held;
        if (deficit > 0) {
            const double thawed = open * grid.thaw[size_t(z)];
            source.evaporation += reach * thawed * deficit;
            if (held < air) source.slope -= reach * thawed / VapourDepth;
        }
    }
    if (open < 1 && wetting > 0) {
        // Ground gives back the lesser of its rain and the air's thirst, smoothly.
        const double thirst = reach * (air - held);
        source.evaporation += (1.0 - open) * wetting * thirst / std::sqrt(wetting * wetting + thirst * thirst + 1e-30);
    }
    const Lifting& lifting = grid.lifting[size_t(z)];
    const double capacity = grid.capacity[cell], perCapacity = grid.perCapacity[cell];
    double falling = 0, steepness = 0;
    const double moreShowers = (LandShowers - ShowerShare) * (1.0 - open);
    const double shares[Liftings] = {lifting.share[0] - moreShowers, lifting.share[1], lifting.share[2] + moreShowers};
    for (int kind = 0; kind < Liftings; ++kind) {
        const double excess = vapour * lifting.perCapacity[kind] - CondensationOnset * capacity;
        if (excess <= 0) continue;
        // In units of the unlifted column: both the excess and the capacity carry the same factor.
        // A passing system can take no more than the excess it finds, once per visit.
        const double condensing = condensationOfExcess(excess, perCapacity), available = excess * PerEpisode[kind];
        const double rate = shares[kind] * std::min(condensing, available) / lifting.perCapacity[kind];
        falling += rate;
        steepness += (condensing < available ? 2.0 : 1.0) * rate / (excess / lifting.perCapacity[kind]);
    }
    // Even a slope that faces the wind all year is rained on only while weather passes over it.
    const double room = 1.0 / (1.0 + falling / WettestYear);
    source.rain = falling * room;
    source.slope -= steepness * room * room;
    return source;
}

// One Gauss–Seidel pass. Every row is solved along its wind; even rows go first and odd rows
// second, each half against the other half's settled values, so the result does not depend on
// how rows are shared out between threads. Only the planet's own grid is worth sharing out.
void sweep(Grid& grid, Workers& workers) {
    const int n = grid.size;
    const double perCell = 1.0 / grid.cell, mixing = EddyMixing * perCell * perCell;
    for (int parity = 0; parity < 2; ++parity) {
        const auto rows = [&](int begin, int end) {
            for (int z = 2 * begin + parity; z < 2 * end; z += 2) {
                const int up = wrapIndex(z - 1, n), down = wrapIndex(z + 1, n);
                const double east = grid.east[size_t(z)], north = grid.north[size_t(z)], south = grid.north[size_t(down)];
                const double fromUpwind = std::abs(east) * perCell + mixing;
                const double fromNorth = std::max(north, 0.0) * perCell + mixing, fromSouth = -std::min(south, 0.0) * perCell + mixing;
                const double leaving = std::abs(east) * perCell + (std::max(south, 0.0) - std::min(north, 0.0)) * perCell + 4 * mixing;
                const int step = east >= 0 ? 1 : -1;
                for (int count = 0, x = east >= 0 ? 0 : n - 1; count < n; ++count, x += step) {
                    const size_t cell = grid.at(x, z);
                    const double arriving = fromUpwind * grid.vapour[grid.at(wrapIndex(x - step, n), z)] +
                                            mixing * grid.vapour[grid.at(wrapIndex(x + step, n), z)] +
                                            fromNorth * grid.vapour[grid.at(x, up)] + fromSouth * grid.vapour[grid.at(x, down)];
                    const double vapour = grid.vapour[cell];
                    const Source source = sourceOf(grid, cell, z, vapour, grid.rain[cell]);
                    const double residual = leaving * vapour - arriving - source.evaporation + source.rain;
                    grid.vapour[cell] = std::max(0.0, vapour - residual / (leaving - source.slope));
                    grid.rain[cell] = source.rain;
                }
            }
        };
        if (n == PlanetCells) workers.parallelFor(n / 2, rows, 16);
        else rows(0, n / 2);
    }
}

Grid describe(const Planet& planet, Workers& workers) {
    Grid grid;
    const int n = PlanetCells;
    grid.size = n;
    grid.cell = PlanetCell;
    for (auto* cells : {&grid.open, &grid.air, &grid.capacity, &grid.perCapacity, &grid.vapour, &grid.rain}) cells->assign(size_t(n) * n, 0.0);
    for (auto* rows : {&grid.exchange, &grid.skin, &grid.thaw, &grid.east, &grid.north}) rows->assign(size_t(n), 0.0);
    grid.lifting.resize(size_t(n));

    std::vector<ZonalClimate> zones;
    for (int z = 0; z < n; ++z) zones.push_back(zonalClimate(planet.seed, planetCellCenter(0, z).z, planet.time));
    const double rowDegrees = 360.0 / n;
    workers.parallelFor(n, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            const ZonalClimate& zone = zones[size_t(z)];
            const ZonalClimate& before = zones[size_t(wrapIndex(z - 1, n))];
            const ZonalClimate& after = zones[size_t(wrapIndex(z + 1, n))];
            const double speed = std::sqrt(dot(zone.wind, zone.wind) + EddyWind * EddyWind);
            grid.exchange[size_t(z)] = ExchangeCoefficient * (speed + Gustiness);
            grid.skin[size_t(z)] = saturationDensity(zone.temperature);
            grid.thaw[size_t(z)] = 1.0 - frozenShare(zone.temperature);
            grid.east[size_t(z)] = zone.wind.x;
            grid.north[size_t(z)] = 0.5 * (zone.wind.z + before.wind.z);

            // Lift by the overturning cells, the same measure the stepped atmosphere takes of its drift.
            const double spreading = (0.5 * (after.wind.z + zone.wind.z) - grid.north[size_t(z)]) / PlanetCell;
            const double overturning = -MoistDepth * spreading * LiftTime;
            const double stormy = StormShare * storminess((after.temperature - before.temperature) / (2 * rowDegrees), zone.latitude);
            Lifting& lifting = grid.lifting[size_t(z)];
            const double shares[Liftings] = {1.0 - stormy - ShowerShare, stormy, ShowerShare};
            const double lifts[Liftings] = {overturning, overturning + StormLift, overturning + ShowerLift};
            for (int kind = 0; kind < Liftings; ++kind) {
                lifting.share[kind] = shares[kind];
                lifting.perCapacity[kind] = 1.0 / liftedCapacity(std::min(lifts[kind], LiftDepth));
            }
            for (int x = 0; x < n; ++x) {
                const size_t cell = grid.at(x, z);
                const double height = planet.elevation.at(x, z) - planet.seaLevel;
                grid.open[cell] = height > 0 ? 0.0 : 1.0;
                grid.air[cell] = saturationDensity(zone.temperature - LapseRate * std::max(0.0, height));
                grid.capacity[cell] = columnCapacity(zone.temperature - LapseRate * terrainLift(height));
                grid.perCapacity[cell] = 1.0 / grid.capacity[cell];
            }
        }
    }, 32);
    return grid;
}

Grid coarsen(const Grid& fine) {
    Grid grid;
    const int n = fine.size / 2;
    grid.size = n;
    grid.cell = fine.cell * 2;
    const auto cells = [&](const std::vector<double>& from) {
        std::vector<double> to(size_t(n) * n);
        for (int z = 0; z < n; ++z)
            for (int x = 0; x < n; ++x)
                to[grid.at(x, z)] = 0.25 * (from[fine.at(2 * x, 2 * z)] + from[fine.at(2 * x + 1, 2 * z)] +
                                            from[fine.at(2 * x, 2 * z + 1)] + from[fine.at(2 * x + 1, 2 * z + 1)]);
        return to;
    };
    grid.open = cells(fine.open);
    grid.air = cells(fine.air);
    grid.capacity = cells(fine.capacity);
    grid.perCapacity.resize(grid.capacity.size());
    for (size_t cell = 0; cell < grid.capacity.size(); ++cell) grid.perCapacity[cell] = 1.0 / grid.capacity[cell];
    for (auto* rows : {&grid.exchange, &grid.skin, &grid.thaw, &grid.east, &grid.north}) rows->resize(size_t(n));
    grid.lifting.resize(size_t(n));
    for (int z = 0; z < n; ++z) {
        const size_t first = size_t(2 * z), second = first + 1;
        grid.exchange[size_t(z)] = 0.5 * (fine.exchange[first] + fine.exchange[second]);
        grid.skin[size_t(z)] = 0.5 * (fine.skin[first] + fine.skin[second]);
        grid.thaw[size_t(z)] = 0.5 * (fine.thaw[first] + fine.thaw[second]);
        grid.east[size_t(z)] = 0.5 * (fine.east[first] + fine.east[second]);
        grid.north[size_t(z)] = fine.north[first];
        grid.lifting[size_t(z)] = fine.lifting[first];
    }
    grid.vapour.assign(size_t(n) * n, 0.0);
    grid.rain = grid.vapour;
    return grid;
}

} // namespace

MoistureBalance solveMoisture(const Planet& planet, Workers& workers) {
    const int n = PlanetCells;
    MoistureBalance balance;
    balance.vapour = balance.evaporation = balance.precipitation = Field<float>(n, n, 0.0f);
    if (planet.surfaceWater <= 0) return balance;

    std::vector<Grid> grids;
    grids.push_back(describe(planet, workers));
    for (int level = 1; level < Levels; ++level) grids.push_back(coarsen(grids.back()));

    // Coarse to fine. Humidity rather than vapour is handed down, so that every fine cell starts
    // with what the air over its own ground can hold.
    for (int level = Levels - 1; level >= 0; --level) {
        Grid& grid = grids[size_t(level)];
        for (int z = 0; z < grid.size; ++z) {
            for (int x = 0; x < grid.size; ++x) {
                const size_t cell = grid.at(x, z);
                if (level == Levels - 1) {
                    grid.vapour[cell] = StartHumidity * grid.capacity[cell];
                } else {
                    const Grid& coarse = grids[size_t(level) + 1];
                    const size_t parent = coarse.at(x / 2, z / 2);
                    grid.vapour[cell] = coarse.vapour[parent] * coarse.perCapacity[parent] * grid.capacity[cell];
                    grid.rain[cell] = coarse.rain[parent];
                }
            }
        }
        for (int pass = 0; pass < Sweeps[Levels - 1 - level]; ++pass) sweep(grid, workers);
    }

    const Grid& grid = grids.front();
    std::vector<double> rowEvaporation(static_cast<size_t>(n)), rowRain(static_cast<size_t>(n));
    workers.parallelFor(n, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            double evaporation = 0, rain = 0;
            for (int x = 0; x < n; ++x) {
                const size_t cell = grid.at(x, z);
                const Source source = sourceOf(grid, cell, z, grid.vapour[cell], grid.rain[cell]);
                balance.vapour.cells[cell] = float(grid.vapour[cell]);
                balance.evaporation.cells[cell] = float(source.evaporation);
                balance.precipitation.cells[cell] = float(source.rain);
                evaporation += source.evaporation;
                rain += source.rain;
            }
            rowEvaporation[size_t(z)] = evaporation;
            rowRain[size_t(z)] = rain;
        }
    }, 32);
    for (int z = 0; z < n; ++z) {
        balance.evaporated += rowEvaporation[size_t(z)] * PlanetCellArea;
        balance.precipitated += rowRain[size_t(z)] * PlanetCellArea;
    }
    return balance;
}

} // namespace derivean::sim

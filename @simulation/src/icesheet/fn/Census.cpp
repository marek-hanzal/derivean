#include "icesheet/fn/Census.hpp"

#include "icesheet/constant/Ice.hpp"
#include "icesheet/type/Sheet.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/type/Planet.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr float Lies = 0.5f;   // m of a deposit from which a cell counts as holding it
constexpr float Thick = 200.0f;   // m of ice at the last maximum from which a planet cell is worth a look

struct Kind {
    double cells{}, depth{};
    void add(float metres) {
        if (metres >= Lies) ++cells, depth += metres;
    }
};

} // namespace

SheetCensus censusOf(const std::vector<const Tile*>& tiles) {
    SheetCensus census;
    Kind till, moraine, outwash, esker;
    double freed = 0, lakes = 0, sea = 0, worn = 0, wornCells = 0, slide = 0, slid = 0;
    census.deepestBed = 1e30;
    for (const Tile* tile : tiles) {
        const SheetRecord* record = tile->sheet.get();
        for (size_t i = 0; i < tile->ice.size(); ++i) {
            ++census.cells;
            const float ground = tile->now.rock[i] + tile->now.sediment[0][i] + tile->now.sediment[1][i] + tile->now.sediment[2][i];
            const bool now = tile->ice[i] >= LeastSheet, then = tile->iceAge[i] >= LeastSheet;
            census.iceNow += now, census.iceThen += then;
            census.thickestNow = std::max(census.thickestNow, double(tile->ice[i])), census.thickestThen = std::max(census.thickestThen, double(tile->iceAge[i]));
            if (then) census.deepestBed = std::min(census.deepestBed, double(ground) - tile->seaLevel);
            if (then && !now) {
                ++freed;
                const bool wet = tile->water[i] > ground;
                lakes += wet && tile->water[i] > tile->seaLevel + 0.5, sea += wet && tile->water[i] <= tile->seaLevel + 0.5;
            }
            if (!record) continue;
            if (record->worn[i] > 0) worn += record->worn[i], ++wornCells, census.wornMost = std::max(census.wornMost, double(record->worn[i]));
            till.add(record->till[i]), moraine.add(record->moraine[i]), outwash.add(record->outwash[i]), esker.add(record->esker[i]);
            const double speed = std::hypot(record->slideX[i], record->slideZ[i]);
            if (speed > 0) slide += speed, ++slid, census.fastest = std::max(census.fastest, speed);
        }
    }
    const double all = double(std::max<size_t>(1, census.cells));
    census.iceNow /= all, census.iceThen /= all, census.freed = freed / all;
    census.freedLakes = freed > 0 ? lakes / freed : 0, census.freedSea = freed > 0 ? sea / freed : 0;
    if (census.deepestBed > 1e29) census.deepestBed = 0;
    census.wornMean = wornCells > 0 ? worn / wornCells : 0;
    census.slide = slid > 0 ? slide / slid : 0;
    const auto close = [&](const Kind& kind, double& share, double& depth) { share = kind.cells / all, depth = kind.cells > 0 ? kind.depth / kind.cells : 0; };
    close(till, census.till, census.tillDepth), close(moraine, census.moraine, census.moraineDepth);
    close(outwash, census.outwash, census.outwashDepth), close(esker, census.esker, census.eskerDepth);
    return census;
}

std::vector<IcePlace> freedPlaces(const Planet& planet, const ContinentalIce& ice, PlaceKind kind, int limit) {
    const bool coastal = kind == PlaceKind::Coastal, margin = kind == PlaceKind::Margin;
    const Field<float>& now = ice.stages[IceNow].thickness;
    const Field<float>& then = ice.stages[IceLastMaximum].thickness;
    struct Found {
        int x, z;
        double thickness, height;
    };
    std::vector<Found> found;
    const auto at = [](int index) { return wrapIndex(index, PlanetCells); };
    for (int z = 0; z < PlanetCells; ++z) {
        for (int x = 0; x < PlanetCells; ++x) {
            if ((margin ? then.at(x, z) < LeastSheet || then.at(x, z) >= Thick : then.at(x, z) < Thick) || now.at(x, z) > 0 || planet.elevation.at(x, z) <= planet.seaLevel) continue;
            // Open sea beside the cell makes it coastal; any sea within two cells keeps it from counting as inland.
            bool beside = false, near = false, beyond = false;
            for (int dz = -2; dz <= 2; ++dz)
                for (int dx = -2; dx <= 2; ++dx) {
                    const int nx = at(x + dx), nz = at(z + dz);
                    const bool sea = planet.elevation.at(nx, nz) <= planet.seaLevel;
                    near = near || sea;
                    beyond = beyond || (!sea && then.at(nx, nz) < LeastSheet);
                    beside = beside || (sea && then.at(nx, nz) <= 0 && std::abs(dx) <= 1 && std::abs(dz) <= 1);
                }
            if (coastal ? beside : !near && (!margin || beyond)) found.push_back({x, z, then.at(x, z), planet.elevation.at(x, z) - planet.seaLevel});
        }
    }
    // Inland the lowest ground first: lowland is where a sheet lays what it carries. On the coast
    // the highest: an outlet cuts deepest where it falls far to the sea.
    std::sort(found.begin(), found.end(), [&](const Found& a, const Found& b) {
        if (a.height != b.height) return coastal ? a.height > b.height : a.height < b.height;
        return a.z != b.z ? a.z < b.z : a.x < b.x;
    });
    std::vector<IcePlace> places;
    for (const Found& cell : found) {
        const bool near = std::any_of(places.begin(), places.end(), [&](const IcePlace& other) {
            const Vec2 centre = planetCellCenter(cell.x, cell.z), there = planetPoint(planet.seed, other.at);
            return std::abs(wrapDelta(centre.x - there.x, PlanetSpan)) < 4 * PlanetCell && std::abs(wrapDelta(centre.z - there.z, PlanetSpan)) < 4 * PlanetCell;
        });
        if (near) continue;
        places.push_back({worldPoint(planet.seed, planetCellCenter(cell.x, cell.z)), cell.height, cell.thickness});
        if (int(places.size()) >= limit) break;
    }
    return places;
}

} // namespace derivean::sim

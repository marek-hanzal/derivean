#include "coast/fn/Coast.hpp"

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {
namespace {

constexpr double FetchStride = 32e3;      // m between the looks at the planet's sea along a fetch
constexpr double LongestFetch = 2400e3;   // m beyond which a sea is fully developed under any storm
constexpr int EdgeStride = 16;            // rim cells between the points a region asks the planet at

// m of open planet sea upwind of a point for a train travelling towards neighbour `k`.
double fetchOver(const Setting& setting, Vec2 from, int k) {
    const Vec2 upwind = normalized({double(-NeighborX[k]), double(-NeighborZ[k])});
    double fetch = 0;
    for (double way = 0.5 * FetchStride; way < LongestFetch; way += FetchStride, fetch += FetchStride)
        if (samplePlanet(setting.planet->elevation, planetPoint(setting.seed, from + upwind * way), Kernel::Linear) >= setting.planet->seaLevel) break;
    return fetch;
}

// A region's rim, from the planet: each train as high as the storm wind of its quarter raises it
// over the open sea upwind. Asked at every EdgeStride-th rim cell and carried along the rim between.
void seasOfPlanet(Domain& domain, Shore& shore, const Setting& setting) {
    const int size = domain.size, last = size - 1;
    const auto swell = [&](int x, int z, int k) {
        const float gale = stormWind(domain, domain.index(x, z), k);
        const float full = FullSea * gale * gale;
        return std::min(full * full, FetchGrowth * gale * gale * float(fetchOver(setting, domain.center(x, z), k)));
    };
    for (int edge = 0; edge < 4; ++edge) {
        const auto place = [&](int along, int& x, int& z) { x = edge == 0 ? along : edge == 1 ? last : edge == 2 ? along : 0, z = edge == 0 ? 0 : edge == 1 ? along : edge == 2 ? last : along; };
        for (int first = 0; first < last; first += EdgeStride) {
            const int second = std::min(last, first + EdgeStride);
            int ax, az, bx, bz;
            place(first, ax, az), place(second, bx, bz);
            for (int k = 0; k < WaveDirections; ++k) {
                const float low = swell(ax, az, k), high = swell(bx, bz, k);
                for (int along = first; along <= second; ++along) {
                    int x, z;
                    place(along, x, z);
                    shore.beyond[domain.index(x, z) * WaveDirections + size_t(k)] = low + (high - low) * float(along - first) / float(second - first);
                }
            }
        }
    }
}

// A finer level's rim, from the trains its parent recorded at the same place.
void seasOfParent(Domain& domain, Shore& shore, const TileLookup& lookup) {
    const Level parent = Level(int(domain.key.level) - 1);
    const double wide = specOf(parent).cell;
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z) {
        for (int x = 0; x <= last; ++x) {
            if (x > 0 && z > 0 && x < last && z < last) continue;
            const Vec2 here = domain.center(x, z);
            const double fx = here.x / wide - 0.5, fz = here.z / wide - 0.5;
            const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
            float height[WaveDirections] = {};
            for (int corner = 0; corner < 4; ++corner) {
                const int64_t gx = ix + (corner & 1), gz = iz + (corner >> 1);
                const TileKey key{parent, int32_t(std::floor(double(gx) / TileCells)), int32_t(std::floor(double(gz) / TileCells))};
                const Tile* tile = lookup(key);
                if (!tile || !tile->shore) continue;
                const size_t cell = size_t(gz - int64_t(key.z) * TileCells) * TileCells + size_t(gx - int64_t(key.x) * TileCells);
                const float weight = float((corner & 1 ? fx - double(ix) : 1 - (fx - double(ix))) * (corner >> 1 ? fz - double(iz) : 1 - (fz - double(iz))));
                for (int k = 0; k < WaveDirections; ++k) height[k] += weight * WaveQuantum * float(tile->shore->height[cell * WaveDirections + size_t(k)]);
            }
            for (int k = 0; k < WaveDirections; ++k) shore.beyond[domain.index(x, z) * WaveDirections + size_t(k)] = height[k] * height[k];
        }
    }
}

} // namespace

void readSeas(Domain& domain, const Setting& setting, const TileLookup& lookup) {
    if (domain.key.level == Level::Ground) return;
    const bool region = domain.key.level == Level::Region;
    if (region && (!setting.planet || setting.planet->surfaceWater <= 0 || setting.planet->elevation.cells.empty())) return;
    if (!domain.shore) domain.shore = std::make_shared<Shore>();
    Shore& shore = *domain.shore;
    shore.beyond.assign(domain.cells() * WaveDirections, 0.0f);
    if (region) seasOfPlanet(domain, shore, setting);
    else seasOfParent(domain, shore, lookup);
    // A domain no sea sends anything into keeps nothing: its own water, if it has any, is found by the first step.
    bool any = false;
    for (const float energy : shore.beyond) any = any || energy > 0;
    if (!any) domain.shore.reset();
}

std::shared_ptr<const ShoreRecord> recordShore(const Domain& domain) {
    if (!domain.shore || domain.shore->top.size() != domain.cells()) return nullptr;
    Shore& shore = *domain.shore;
    const float sea = float(domain.seaLevel) + SeaBand;
    bool any = false;
    for (size_t i = 0; i < domain.cells(); ++i) {
        shore.top[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        const bool stands = domain.level[i] > shore.top[i] + 1e-3f && (domain.level[i] <= sea || domain.discharge[i] > 0);
        shore.stood[i] = stands ? domain.level[i] : NoWater;
        any = any || stands;
    }
    if (!any || !spreadWater(domain, shore)) return nullptr;
    raiseWaves(domain, shore);
    auto record = std::make_shared<ShoreRecord>();
    const size_t cells = size_t(TileCells) * TileCells;
    record->height.resize(cells * WaveDirections);
    record->notch.resize(cells), record->cut.resize(cells), record->laid.resize(cells);
    record->ledger = shore.ledger;
    for (int z = 0; z < TileCells; ++z) {
        for (int x = 0; x < TileCells; ++x) {
            const size_t from = domain.index(x + Halo, z + Halo), to = size_t(z) * TileCells + size_t(x);
            record->notch[to] = shore.notch[from], record->cut[to] = shore.cut[from], record->laid[to] = shore.laid[from];
            for (int k = 0; k < WaveDirections; ++k) {
                const size_t at = from * WaveDirections + size_t(k);
                // Water keeps the train that leaves it; land, which lets none through, the one that strikes it.
                const float energy = shore.wave[at] > 0 ? shore.wave[at] : shore.arrive[at];
                record->height[to * WaveDirections + size_t(k)] = uint8_t(std::min(255.0f, std::sqrt(energy) / WaveQuantum + 0.5f));
            }
        }
    }
    return record;
}

} // namespace derivean::sim

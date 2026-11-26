#include "measure/fn/Scale.hpp"

#include "landscape/type/Terrain.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr float Unknown = -1e30f, Pending = -2e30f;

// Water level every dry cell drains to, by following the main flow direction. `met` is set where
// the path reached water inside the mosaic.
void traceBase(const Mosaic& mosaic, double river, std::vector<float>& base, std::vector<uint8_t>& met) {
    const int side = mosaic.side();
    const size_t cells = size_t(side) * size_t(side);
    base.assign(cells, Unknown);
    met.assign(cells, 0);
    std::vector<int32_t> path;
    for (size_t start = 0; start < cells; ++start) {
        if (base[start] != Unknown) continue;
        path.clear();
        int32_t cell = int32_t(start);
        float level = Unknown;
        uint8_t reached = 0;
        while (true) {
            const size_t i = size_t(cell);
            // Tiles are routed one by one, so a path may turn back across an edge; it ends there.
            if (base[i] == Pending) {
                level = mosaic.ground.cells[i];
                break;
            }
            if (base[i] != Unknown) {
                level = base[i], reached = met[i];
                break;
            }
            const int x = cell % side, z = cell / side;
            path.push_back(cell);
            base[i] = Pending;
            if (mosaic.flooded(x, z)) {
                level = mosaic.water.cells[i], reached = 1;
                break;
            }
            if (mosaic.discharge.cells[i] >= river) {
                level = mosaic.ground.cells[i], reached = 1;
                break;
            }
            const uint8_t to = mosaic.flowTo.cells[i];
            const int nx = to > 7 ? -1 : x + NeighborX[to], nz = to > 7 ? -1 : z + NeighborZ[to];
            if (nx < 0 || nz < 0 || nx >= side || nz >= side) {
                level = mosaic.ground.cells[i];
                break;
            }
            cell = nz * side + nx;
        }
        for (const int32_t visited : path) base[size_t(visited)] = level, met[size_t(visited)] = reached;
    }
}

} // namespace

LandHeights landHeights(const Mosaic& mosaic, double river) {
    LandHeights heights;
    const int side = mosaic.side();
    std::vector<float> base;
    std::vector<uint8_t> met;
    traceBase(mosaic, river, base, met);
    size_t lakes = 0, sea = 0, traced = 0;
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            const size_t i = mosaic.ground.index(x, z);
            if (mosaic.flooded(x, z)) {
                ++(mosaic.lake(x, z) ? lakes : sea);
                continue;
            }
            if (mosaic.ground.cells[i] < mosaic.seaLevel) {
                ++sea;
                continue;
            }
            heights.aboveSea.push_back(float(mosaic.ground.cells[i] - mosaic.seaLevel));
            heights.aboveWater.push_back(mosaic.ground.cells[i] - base[i]);
            traced += met[i];
        }
    }
    const double cells = double(side) * side;
    heights.dryShare = double(heights.aboveSea.size()) / cells;
    heights.lakeShare = double(lakes) / cells;
    heights.seaShare = double(sea) / cells;
    heights.traced = heights.aboveSea.empty() ? 0.0 : double(traced) / double(heights.aboveSea.size());
    return heights;
}

TransectStats transectsOf(const Mosaic& mosaic, double length) {
    TransectStats stats;
    stats.length = length;
    stats.spacing = mosaic.cell;
    const int side = mosaic.side();
    const int steps = int(std::lround(length / mosaic.cell));
    if (steps < 4 || steps >= side) return stats;
    constexpr int DirX[4] = {1, 0, 1, 1}, DirZ[4] = {0, 1, 1, -1};
    std::vector<float> relief, climb;
    for (int direction = 0; direction < 4; ++direction) {
        const int dx = DirX[direction], dz = DirZ[direction];
        const int count = direction < 2 ? steps : int(std::lround(steps / 1.41421356));
        // Starts half a line apart, thinned so the largest mosaic stays at a few thousand lines.
        const int stride = std::max(std::max(1, count / 2), side / 48);
        for (int z0 = 0; z0 < side; z0 += stride) {
            for (int x0 = 0; x0 < side; x0 += stride) {
                const int x1 = x0 + dx * count, z1 = z0 + dz * count;
                if (x1 >= side || z1 < 0 || z1 >= side) continue;
                float low = 1e30f, high = -1e30f, rise = 0, before = 0;
                int drowned = 0;
                for (int k = 0; k <= count; ++k) {
                    const int x = x0 + dx * k, z = z0 + dz * k;
                    const float here = mosaic.top(x, z);
                    drowned += mosaic.ground.at(x, z) < mosaic.seaLevel && !mosaic.lake(x, z);
                    low = std::min(low, here), high = std::max(high, here);
                    if (k > 0) rise += std::max(0.0f, here - before);
                    before = here;
                }
                if (drowned * 4 > count) continue;
                if (relief.empty() || high - low > *std::max_element(relief.begin(), relief.end())) stats.steepest = cellCentre(mosaic, x0, z0);
                relief.push_back(high - low);
                climb.push_back(rise);
            }
        }
    }
    stats.relief = distributionOf(std::move(relief));
    stats.climb = distributionOf(std::move(climb));
    return stats;
}

ReliefBand earthRelief(double length) {
    const double km = length / 1000.0;
    return {15.0 * std::pow(km, 0.65), 150.0 * std::pow(km, 0.43), 1500.0 * std::pow(km, 0.37)};
}

const char* reliefClass(double relief, double length) {
    const ReliefBand band = earthRelief(length);
    return relief <= band.plain ? "plain" : relief <= band.hills ? "hills" : relief <= band.mountains ? "mountains" : "beyond Earth";
}

} // namespace derivean::sim

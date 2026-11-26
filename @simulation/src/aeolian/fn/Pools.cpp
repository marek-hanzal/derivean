#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"
#include "hydrology/constant/Water.hpp"

#include <algorithm>
#include <vector>

namespace derivean::sim {

// The flood fills a hollow to its spill with the year's mean flow. A stream of dry country runs
// only a share of the year; the pool it feeds is full while it runs and then sinks at the rate the
// air takes water, one flood season a year. Its level is above any height for the share
// `running + (spill − height) / demand` of the year, so the level it holds half the year or more
// stands `demand · (½ − running)` below the spill. The demand is gross: in dry country the rain
// comes with the floods, not between them. Not counted: a water table that holds a pool up (oases,
// spring-fed pans), seepage through its floor, and several short floods a year, which keep a
// semi-arid pool fuller than one season does.
//
// The routing is left as it is: its discharge and `Tile::water` describe the mean flow that finer
// levels and the live water start from. A pool larger than the halo can be judged differently by
// two neighbouring tiles where only one of them sees the river through it.
std::vector<float> poolEbb(const Domain& domain) {
    if (domain.key.level == Level::Region || domain.key.level == Level::Ground) return {};
    const size_t cells = domain.cells();
    const int size = domain.size;
    const float sea = float(domain.seaLevel) + 0.5f;   // as `extractTile` tells the sea from a lake
    const auto pooled = [&](size_t i) {
        return domain.level[i] > domain.surface[i] + 1e-3f && domain.level[i] > sea && (domain.ice.empty() || domain.ice[i] <= 0);
    };
    std::vector<float> ebb(cells, 0.0f);
    std::vector<uint8_t> seen(cells, 0);
    std::vector<size_t> pool, stack;
    for (size_t start = 0; start < cells; ++start) {
        if (seen[start] || !pooled(start)) continue;
        pool.clear(), stack.assign(1, start), seen[start] = 1;
        float flow = 0, rain = 0, warmth = 0;
        while (!stack.empty()) {
            const size_t i = stack.back();
            stack.pop_back();
            pool.push_back(i);
            flow = std::max(flow, domain.discharge[i]), rain += domain.rain[i], warmth += domain.warmthNow[i];
            const int x = int(i % size_t(size)), z = int(i / size_t(size));
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                const size_t next = domain.index(nx, nz);
                if (!seen[next] && pooled(next)) seen[next] = 1, stack.push_back(next);
            }
        }
        rain /= float(pool.size()), warmth /= float(pool.size());
        // A stream from wetter ground keeps running whatever the pool's own climate (as in `Budget.cpp`).
        const float running = std::max(runningShareOf(rain, warmth), flow / (flow + PerennialFlow));
        const float drop = evaporationDemand(warmth) * std::max(0.0f, 0.5f - running);
        for (const size_t i : pool) ebb[i] = drop;
    }
    return ebb;
}

} // namespace derivean::sim

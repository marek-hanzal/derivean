#include "landscape/fn/Process.hpp"

#include <limits>

namespace derivean::sim {

// Priority flood with a bucket queue. Cells are taken in rising order of water level starting from
// the open rim; a cell reached from a higher level is standing water and inherits that level plus
// the smallest representable step, which keeps every lake draining towards its outlet.
// Buckets make the order exact only to one bucket width (millimetres to decimetres depending on
// the relief of the domain), which is far below what a cell of the level can express.
void floodLevels(Domain& domain, FlowScratch& scratch) {
    const size_t cells = domain.cells();
    const int size = domain.size;
    const float sea = float(domain.seaLevel);
    // A ground cell on a river's bed lies a fraction of a millimetre under the next one up: the
    // ground level needs finer buckets than the levels whose cells are metres wide.
    const bool fine = domain.key.level == Level::Ground;
    const int Buckets = fine ? 1 << 20 : 1 << 16;

    float low = std::numeric_limits<float>::max(), high = std::numeric_limits<float>::lowest();
    for (size_t i = 0; i < cells; ++i) {
        const float ground = domain.ground.rock[i] + domain.sedimentAt(i);
        domain.surface[i] = ground;
        const float open = std::max(ground, sea);
        low = std::min(low, open);
        high = std::max(high, open);
    }
    const float width = std::max((high - low) / float(Buckets - 2), fine ? 2e-5f : 1e-3f);
    const auto bucketOf = [&](float level) { return std::min(Buckets - 1, int((level - low) / width)); };

    scratch.head.assign(size_t(Buckets), -1);
    scratch.link.assign(cells, -1);
    scratch.visited.assign(cells, 0);
    scratch.pit.clear();
    domain.order.clear();
    std::fill(domain.spill.begin(), domain.spill.end(), -1);

    int current = 0;
    const auto push = [&](int32_t cell, float level) {
        const int bucket = bucketOf(level);
        if (bucket <= current) {
            scratch.pit.push_back(cell);
            return;
        }
        scratch.link[size_t(cell)] = scratch.head[size_t(bucket)];
        scratch.head[size_t(bucket)] = cell;
    };

    // Water leaves through the rim cells the coarser level marks as outlets and through the sea.
    // If a domain has neither, its lowest rim cell is the way out.
    current = -1;
    size_t lowest = cells;
    bool open = false;
    for (size_t i = 0; i < cells; ++i) {
        if (domain.hold[i] != Rim) continue;
        if (lowest == cells || domain.surface[i] < domain.surface[lowest]) lowest = i;
        if (!domain.outlet[i] && domain.surface[i] > sea) continue;
        open = true;
        scratch.visited[i] = 1;
        domain.level[i] = std::max(domain.surface[i], sea);
        push(int32_t(i), domain.level[i]);
    }
    if (!open && lowest < cells) {
        scratch.visited[lowest] = 1;
        domain.level[lowest] = std::max(domain.surface[lowest], sea);
        push(int32_t(lowest), domain.level[lowest]);
    }

    while (true) {
        int32_t cell;
        if (!scratch.pit.empty()) {
            cell = scratch.pit.back();
            scratch.pit.pop_back();
        } else {
            while (current < Buckets - 1 && (current < 0 || scratch.head[size_t(current)] < 0)) ++current;
            cell = scratch.head[size_t(current)];
            if (cell < 0) break;
            scratch.head[size_t(current)] = scratch.link[size_t(cell)];
        }
        domain.rank[size_t(cell)] = int32_t(domain.order.size());
        domain.order.push_back(cell);
        const int x = cell % size, z = cell / size;
        const float here = domain.level[size_t(cell)];
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = domain.index(nx, nz);
            if (scratch.visited[other]) continue;
            scratch.visited[other] = 1;
            float level = std::max(domain.surface[other], sea);
            if (level <= here) {
                level = std::nextafter(here, std::numeric_limits<float>::max());
                domain.spill[other] = cell;
            }
            domain.level[other] = level;
            push(int32_t(other), level);
        }
    }
}

} // namespace derivean::sim

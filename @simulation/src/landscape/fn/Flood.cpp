#include "landscape/fn/Process.hpp"

#include "groundwater/fn/Sinking.hpp"

#include <bit>
#include <limits>
#include <utility>
#include <vector>

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

    // The buckets are left empty by every flood, so only their size is set here. A bit per bucket
    // and a bit per word of those say which hold cells: the next one is found without walking the
    // empty ones, which on the ground level are nearly all of a million.
    if (scratch.head.size() != size_t(Buckets)) scratch.head.assign(size_t(Buckets), -1);
    std::vector<uint64_t>& filled = scratch.filled;
    std::vector<uint64_t>& words = scratch.filledWords;
    filled.assign(size_t(Buckets) / 64, 0);
    words.assign(size_t(Buckets) / 4096, 0);
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
        if (scratch.head[size_t(bucket)] < 0) {
            filled[size_t(bucket) >> 6] |= uint64_t(1) << (bucket & 63);
            words[size_t(bucket) >> 12] |= uint64_t(1) << ((bucket >> 6) & 63);
        }
        scratch.link[size_t(cell)] = scratch.head[size_t(bucket)];
        scratch.head[size_t(bucket)] = cell;
    };
    // The lowest bucket at or above `from` that holds a cell; the last bucket when none does, where
    // a walk over the empty ones would have stopped.
    const auto nextFilled = [&](int from) {
        size_t word = size_t(from) >> 6;
        if (const uint64_t bits = filled[word] & (~uint64_t(0) << (from & 63))) return int(word * 64 + size_t(std::countr_zero(bits)));
        for (size_t group = word >> 6, first = (word & 63) + 1; group < words.size(); ++group, first = 0) {
            const uint64_t mask = first >= 64 ? 0 : words[group] & (~uint64_t(0) << first);
            if (!mask) continue;
            word = group * 64 + size_t(std::countr_zero(mask));
            return int(word * 64 + size_t(std::countr_zero(filled[word])));
        }
        return Buckets - 1;
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
    // A hollow in soluble rock whose floor takes in all that reaches it holds no lake: its water
    // goes down there (groundwater/fn/Sinking.hpp).
    for (size_t i = 0; domain.sinking.active() && i < cells; ++i) {
        if (scratch.visited[i] || !drainsDown(domain, i)) continue;
        scratch.visited[i] = 1;
        domain.level[i] = domain.surface[i];
        push(int32_t(i), domain.level[i]);
    }

    // A cell into which a finished neighbour hands its river (Domain::handsOn) is not reached from
    // that neighbour's cell while any other way to it is left, so it never drains back against the river.
    std::vector<std::pair<int32_t, int32_t>> deferred;
    while (true) {
        int32_t cell;
        if (!scratch.pit.empty()) {
            cell = scratch.pit.back();
            scratch.pit.pop_back();
        } else {
            current = nextFilled(std::max(current, 0));
            cell = scratch.head[size_t(current)];
            if (cell < 0) {
                // Only cells reached solely across a handed-on step are left: they are flooded on from
                // there in rising order like the rest.
                if (deferred.empty()) break;
                current = -1;
                for (const auto& [from, to] : deferred) {
                    if (scratch.visited[size_t(to)]) continue;
                    scratch.visited[size_t(to)] = 1;
                    float level = std::max(domain.surface[size_t(to)], sea);
                    if (level <= domain.level[size_t(from)]) {
                        level = std::nextafter(domain.level[size_t(from)], std::numeric_limits<float>::max());
                        domain.spill[size_t(to)] = from;
                    }
                    domain.level[size_t(to)] = level;
                    push(to, level);
                }
                deferred.clear();
                continue;
            }
            scratch.head[size_t(current)] = scratch.link[size_t(cell)];
            if (scratch.head[size_t(current)] < 0) {
                filled[size_t(current) >> 6] &= ~(uint64_t(1) << (current & 63));
                if (!filled[size_t(current) >> 6]) words[size_t(current) >> 12] &= ~(uint64_t(1) << ((current >> 6) & 63));
            }
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
            if (domain.handsOn(size_t(cell), other)) {
                deferred.emplace_back(cell, int32_t(other));
                continue;
            }
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

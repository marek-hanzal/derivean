#include "icesheet/fn/Meltwater.hpp"

#include "icesheet/constant/Ice.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace derivean::sim {
namespace {

float groundOf(const Domain& domain, size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); }

std::vector<int32_t> descending(const std::vector<float>& height) {
    std::vector<int32_t> order(height.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int32_t a, int32_t b) { return height[size_t(a)] != height[size_t(b)] ? height[size_t(a)] > height[size_t(b)] : a < b; });
    return order;
}

} // namespace

void routeMeltwater(const Domain& domain, const SheetField& field, std::vector<float>& flow, std::vector<uint8_t>& to) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const float area = float(domain.cell * domain.cell);
    flow.assign(cells, 0.0f);
    to.assign(cells, 255);
    // Head of the water under the ice, in metres of water; bare ground lies below every iced
    // neighbour's head by the weight of that neighbour's ice.
    std::vector<float> head(cells);
    for (size_t i = 0; i < cells; ++i) head[i] = groundOf(domain, i) + float(IceToWater) * field.thickness[i];
    for (const int32_t cell : descending(head)) {
        const size_t i = size_t(cell);
        if (field.thickness[i] <= 0) continue;
        const int x = cell % size, z = cell / size;
        flow[i] += field.melt[i] * area;
        int best = -1;
        float steepest = 0;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const float fall = (head[i] - head[domain.index(nx, nz)]) / NeighborReach[k];
            if (fall > steepest) steepest = fall, best = k;
        }
        if (best < 0) continue;
        to[i] = uint8_t(best);
        const size_t n = domain.index(x + NeighborX[best], z + NeighborZ[best]);
        if (field.thickness[n] > 0) flow[n] += flow[i];
    }
}

void spreadOutwash(Domain& domain, const SheetField& field, const std::vector<float>& flow, const std::vector<uint8_t>& to, SheetRecord& left, WearTotals& totals) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const float area = float(domain.cell * domain.cell);
    // What each stream takes from the ground it comes out onto: gravel and sand to be spread, mud to be lost.
    std::vector<float> load[2];
    for (auto& part : load) part.assign(cells, 0.0f);
    for (size_t i = 0; i < cells; ++i) {
        if (to[i] > 7 || field.thickness[i] <= 0) continue;
        const size_t portal = domain.index(int(i % size_t(size)) + NeighborX[to[i]], int(i / size_t(size)) + NeighborZ[to[i]]);
        if (field.thickness[portal] > 0 || domain.hold[portal] != Free || left.moraine[portal] <= 0) continue;
        const float share = std::min(0.9f, flow[i] / (flow[i] + PortalFlow));
        const float taken = std::min(share * left.moraine[portal], domain.sedimentAt(portal));
        if (taken <= 0) continue;
        const float cover = domain.sedimentAt(portal);
        float moved[SedimentClasses];
        for (int c = 0; c < SedimentClasses; ++c) {
            moved[c] = taken * domain.ground.sediment[c][portal] / cover;
            domain.ground.sediment[c][portal] -= moved[c];
        }
        left.moraine[portal] -= taken;
        load[Coarse][portal] += moved[Coarse] * area, load[Sand][portal] += moved[Sand] * area;
        totals.dropped -= double(taken) * area;
        totals.left += double(moved[Fines]) * area;
        if (domain.owned(int(portal % size_t(size)), int(portal / size_t(size)))) {
            domain.ledger.sedimentOut += double(moved[Fines]) * area;
            domain.ledger.deposited -= double(taken) * area;
        }
        domain.deposited[portal] -= taken;
    }

    std::vector<float> ground(cells);
    for (size_t i = 0; i < cells; ++i) ground[i] = field.thickness[i] > 0 ? 1e30f : groundOf(domain, i);
    const OutwashTotals spread = washBeyond(domain, ground, load, left.outwash);
    totals.dropped += spread.settled, totals.left += spread.carriedOff;
}

OutwashTotals washBeyond(Domain& domain, const std::vector<float>& ground, std::vector<float> load[2], std::vector<float>& outwash) {
    const int size = domain.size;
    const float area = float(domain.cell * domain.cell);
    OutwashTotals totals;
    // Beyond the ice the water runs down the ground and divides over every lower neighbour, as a
    // river with more gravel than it can carry does.
    const float settling[2] = {1.0f - std::exp(-float(domain.cell) / OutwashReach[Coarse]), 1.0f - std::exp(-float(domain.cell) / OutwashReach[Sand])};
    for (const int32_t cell : descending(ground)) {
        const size_t i = size_t(cell);
        if (ground[i] >= 1e30f || load[Coarse][i] + load[Sand][i] <= 0) continue;
        const int x = cell % size, z = cell / size;
        float fall[8], total = 0;
        for (int k = 0; k < 8; ++k) {
            fall[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t n = domain.index(nx, nz);
            if (ground[n] < ground[i]) total += fall[k] = (ground[i] - ground[n]) / NeighborReach[k];
        }
        const bool free = domain.hold[i] == Free;
        // A hollow, a lake floor or the sea keeps everything that reaches it.
        const bool held = total <= 0 || ground[i] < float(domain.seaLevel);
        for (int c = 0; c < 2; ++c) {
            const float settled = free ? load[c][i] * (held ? 1.0f : settling[c]) : 0.0f;
            if (settled > 0) {
                domain.ground.sediment[c][i] += settled / area;
                domain.deposited[i] += settled / area;
                outwash[i] += settled / area;
                if (domain.owned(x, z)) domain.ledger.deposited += settled;
                totals.settled += settled;
            }
            const float onward = load[c][i] - settled;
            if (onward <= 0) continue;
            if (!free || total <= 0) {
                totals.carriedOff += onward;
                if (domain.owned(x, z)) domain.ledger.sedimentOut += onward;
                continue;
            }
            for (int k = 0; k < 8; ++k) {
                if (fall[k] <= 0) continue;
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                const float share = onward * fall[k] / total;
                load[c][domain.index(nx, nz)] += share;
                // What crosses between the tile's cells and its halo crosses its edge.
                const bool from = domain.owned(x, z), into = domain.owned(nx, nz);
                if (from && !into) domain.ledger.sedimentOut += share;
                if (!from && into) domain.ledger.sedimentIn += share;
            }
        }
    }
    return totals;
}

} // namespace derivean::sim

#include "icesheet/fn/Meltwater.hpp"

#include "icesheet/constant/Ice.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>

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
    const size_t cells = domain.cells();
    const float area = float(domain.cell * domain.cell);
    const float sea = float(domain.seaLevel);
    OutwashTotals totals;
    // The level water stands at on each cell: the lowest it can spill from towards the domain's
    // edge, the sea or the ice, under which it drains. Cells it stands over form lakes. The cells in
    // the order the water reached them from its outlets, and the cell each was reached from.
    std::vector<float> level(cells, 0.0f);
    std::vector<int32_t> reachedFrom(cells, -1), order;
    order.reserve(cells);
    {
        using Entry = std::pair<float, int32_t>;
        std::vector<Entry> heap;
        heap.reserve(cells);   // every cell enters once
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open(std::greater<Entry>{}, std::move(heap));
        std::vector<bool> reached(cells, false);
        for (int z = 0; z < size; ++z)
            for (int x = 0; x < size; ++x) {
                const size_t i = domain.index(x, z);
                const bool iced = ground[i] >= 1e30f;
                if (iced || x == 0 || z == 0 || x == size - 1 || z == size - 1 || ground[i] < sea) {
                    reached[i] = true, level[i] = iced ? -1e30f : ground[i];
                    open.push({level[i], int32_t(i)});
                }
            }
        while (!open.empty()) {
            const auto [stand, cell] = open.top();
            open.pop();
            order.push_back(cell);
            const int x = cell % size, z = cell / size;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                const size_t n = domain.index(nx, nz);
                if (reached[n]) continue;
                reached[n] = true, level[n] = std::max(ground[n], stand), reachedFrom[n] = cell;
                open.push({level[n], int32_t(n)});
            }
        }
    }
    // Each lake's cells, and the room its floor offers on the cells this domain evolves.
    std::vector<int32_t> lake(cells, -1);
    std::vector<std::vector<int32_t>> lakes;
    std::vector<double> room;
    for (size_t s = 0; s < cells; ++s) {
        if (lake[s] >= 0 || level[s] <= ground[s] || ground[s] >= 1e30f) continue;
        const int32_t id = int32_t(lakes.size());
        lakes.emplace_back();
        room.push_back(0.0);
        std::vector<int32_t> stack{int32_t(s)};
        lake[s] = id;
        while (!stack.empty()) {
            const int32_t cell = stack.back();
            stack.pop_back();
            lakes[size_t(id)].push_back(cell);
            if (domain.hold[size_t(cell)] == Free) room[size_t(id)] += double(level[size_t(cell)] - ground[size_t(cell)]) * area;
            const int x = cell % size, z = cell / size;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                const size_t n = domain.index(nx, nz);
                if (lake[n] < 0 && level[n] > ground[n] && ground[n] < 1e30f) lake[n] = id, stack.push_back(int32_t(n));
            }
        }
    }
    // What reaches a lake, by class, on the tile's cells and in all.
    std::vector<double> arrived[2], arrivedOwned(lakes.size(), 0.0);
    for (auto& part : arrived) part.assign(lakes.size(), 0.0);
    const auto pooled = [&](size_t n) { return lake[n] >= 0 && domain.hold[n] == Free; };
    // Beyond the ice the water runs down the ground and divides over every lower neighbour, as a
    // river with more gravel than it can carry does; over a flat it runs on the way the water
    // reached it, back towards its outlet. Every cell is passed before the cells it sends to.
    const float settling[2] = {1.0f - std::exp(-float(domain.cell) / OutwashReach[Coarse]), 1.0f - std::exp(-float(domain.cell) / OutwashReach[Sand])};
    for (auto next = order.rbegin(); next != order.rend(); ++next) {
        const size_t i = size_t(*next);
        if (ground[i] >= 1e30f || load[Coarse][i] + load[Sand][i] <= 0) continue;
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        const bool free = domain.hold[i] == Free;
        // Standing water takes in everything that reaches it, to be spread over its floor below.
        if (pooled(i)) {
            for (int c = 0; c < 2; ++c) arrived[c][size_t(lake[i])] += load[c][i];
            if (domain.owned(x, z)) arrivedOwned[size_t(lake[i])] += load[Coarse][i] + load[Sand][i];
            continue;
        }
        float fall[8], total = 0;
        for (int k = 0; k < 8; ++k) {
            fall[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t n = domain.index(nx, nz);
            if (ground[n] < ground[i]) total += fall[k] = (ground[i] - ground[n]) / NeighborReach[k];
        }
        const int32_t back = reachedFrom[i];
        const bool flat = total <= 0 && back >= 0 && ground[size_t(back)] < 1e30f;
        // The sea floor keeps what reaches it up to the sea's surface and passes the rest on down.
        const bool sunk = ground[i] < sea;
        float room = std::max(0.0f, sea - ground[i]) * area;
        for (int c = 0; c < 2; ++c) {
            const float settled = !free ? 0.0f : sunk ? std::min(load[c][i], room) : load[c][i] * settling[c];
            if (sunk) room -= settled;
            if (settled > 0) {
                domain.ground.sediment[c][i] += settled / area;
                domain.deposited[i] += settled / area;
                outwash[i] += settled / area;
                if (domain.owned(x, z)) domain.ledger.deposited += settled;
                totals.settled += settled;
            }
            const float onward = load[c][i] - settled;
            if (onward <= 0) continue;
            if (!free || (total <= 0 && !flat)) {
                totals.carriedOff += onward;
                if (domain.owned(x, z)) domain.ledger.sedimentOut += onward;
                continue;
            }
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                const size_t n = domain.index(nx, nz);
                const float share = flat ? (n == size_t(back) ? onward : 0.0f) : onward * fall[k] / total;
                if (share <= 0) continue;
                if (pooled(n)) {
                    arrived[c][size_t(lake[n])] += share;
                    if (domain.owned(nx, nz)) arrivedOwned[size_t(lake[n])] += share;
                } else load[c][n] += share;
                // What crosses between the tile's cells and its halo crosses its edge.
                const bool from = domain.owned(x, z), into = domain.owned(nx, nz);
                if (from && !into) domain.ledger.sedimentOut += share;
                if (!from && into) domain.ledger.sedimentIn += share;
            }
        }
    }
    // A lake's floor takes what reached it up to the level its water spills at, deepest cells most,
    // so it fills flat; the water spilling over its rim takes the rest away.
    for (size_t id = 0; id < lakes.size(); ++id) {
        const double reached = arrived[Coarse][id] + arrived[Sand][id];
        if (reached <= 0) continue;
        const double kept = std::min(1.0, room[id] / reached);
        double depths = 0, ownedLaid = 0;
        for (const int32_t cell : lakes[id])
            if (domain.hold[size_t(cell)] == Free) depths += double(level[size_t(cell)] - ground[size_t(cell)]);
        for (const int32_t cell : lakes[id]) {
            const size_t i = size_t(cell);
            if (domain.hold[i] != Free || depths <= 0) continue;
            const double share = double(level[i] - ground[i]) / depths;
            float laid = 0;
            for (int c = 0; c < 2; ++c) {
                const float part = float(arrived[c][id] * kept * share / area);
                domain.ground.sediment[c][i] += part, laid += part;
            }
            domain.deposited[i] += laid, outwash[i] += laid;
            if (domain.owned(cell % size, cell / size)) ownedLaid += double(laid) * area;
        }
        const double settled = depths > 0 ? reached * kept : 0.0;
        totals.settled += settled, totals.carriedOff += reached - settled;
        // What reached the lake on the tile's cells and what its floor kept there differ by what
        // crossed the tile's edge inside the lake or left over its rim.
        domain.ledger.deposited += ownedLaid;
        if (ownedLaid > arrivedOwned[id]) domain.ledger.sedimentIn += ownedLaid - arrivedOwned[id];
        else domain.ledger.sedimentOut += arrivedOwned[id] - ownedLaid;
    }
    return totals;
}

} // namespace derivean::sim

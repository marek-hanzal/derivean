#include "groundwater/fn/Spill.hpp"

#include "groundwater/type/Sinking.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

namespace derivean::sim {
namespace {

// What SinkWork::pour holds for a lake: its pass is not known yet, or the lake joined another; its
// pass is the domain's edge. Any other value is the cell beyond the pass.
constexpr int32_t Unknown = -1, EdgePass = -2;

float outOf(const std::vector<float>& share, size_t cell) {
    float sum = 0;
    for (int k = 0; k < 8; ++k) sum += share[cell * 8 + size_t(k)];
    return sum;
}

// The catchment of the hollow `from`: every cell whose water, or part of it, the route took there,
// listed under lake `id`. A cell whose water parts between two hollows belongs to both catchments,
// so no pass of either may lead back into it: it is listed under every lake that took it, and
// `lakeOf` names the last.
void basin(const Domain& domain, const std::vector<float>& share, SinkWork& work, int32_t from, int32_t id, std::vector<int32_t>& stack) {
    const int size = domain.size;
    stack.assign(1, from);
    work.lakeOf[size_t(from)] = id;
    work.lakes[size_t(id)].push_back(from);
    while (!stack.empty()) {
        const int32_t u = stack.back();
        stack.pop_back();
        const int x = u % size, z = u / size;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t v = domain.index(nx, nz);
            if (work.lakeOf[v] == id || share[v * 8 + size_t((k + 4) & 7)] <= 0) continue;
            work.lakeOf[v] = id;
            work.lakes[size_t(id)].push_back(int32_t(v));
            stack.push_back(int32_t(v));
        }
    }
}

// The lowest pass out of lake `id`'s catchment and the cell beyond it, or EdgePass. A pass into the
// catchment of a lake filled earlier in the step joins that lake to this one, and the search goes on
// from the two together.
int32_t pourOut(const Domain& domain, SinkWork& work, int32_t id) {
    const int size = domain.size;
    while (true) {
        float lowest = std::numeric_limits<float>::max(), beyond = 0;
        int32_t to = Unknown;
        for (const int32_t a : work.lakes[size_t(id)]) {
            if (work.lakeOf[size_t(a)] != id) continue;   // listed here, taken by a later lake since
            const int x = a % size, z = a / size;
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) {
                    // The ground beyond the domain is a neighbour's and is not read here: a catchment
                    // that reaches the edge runs over it at the edge cell's own height, which may lie
                    // above the true pass.
                    if (domain.surface[size_t(a)] < lowest) lowest = domain.surface[size_t(a)], beyond = lowest, to = EdgePass;
                    continue;
                }
                const size_t b = domain.index(nx, nz);
                if (work.lakeOf[b] == id) continue;
                const float pass = std::max(domain.surface[size_t(a)], domain.surface[b]);
                if (pass < lowest || (pass == lowest && domain.surface[b] < beyond)) lowest = pass, beyond = domain.surface[b], to = int32_t(b);
            }
        }
        // Every catchment has a cell on the edge of a finite domain or a neighbour outside it.
        if (to == Unknown) throw std::logic_error("spillHollows: a lake's catchment has no pass");
        if (to == EdgePass || work.lakeOf[size_t(to)] < 0) return to;
        const int32_t other = work.lakeOf[size_t(to)];
        for (const int32_t c : work.lakes[size_t(other)])
            if (work.lakeOf[size_t(c)] == other) work.lakeOf[size_t(c)] = id, work.lakes[size_t(id)].push_back(c);
        work.lakes[size_t(other)].clear();
        work.pour[size_t(other)] = Unknown;
    }
}

} // namespace

void spillHollows(Domain& domain, const std::vector<float>& share, std::vector<float>* gathered) {
    SinkWork& work = domain.sinking;
    work.spilled = 0, work.overEdge = 0;
    if (!work.active()) return;
    const size_t cells = domain.cells();
    if (std::none_of(work.ponded.begin(), work.ponded.end(), [](float p) { return p > 0; })) return;
    const int size = domain.size;
    const float area = float(domain.cell * domain.cell);
    std::vector<float>& extra = work.extra;
    work.extra.assign(cells, 0.0f), work.lakeOf.assign(cells, -1), work.lakes.clear(), work.pour.clear();
    std::vector<int32_t> stack;

    // Cells with water still to pass on, the highest in the route's order first: the route hands
    // water only to cells after it in the order, so most cells are passed once.
    std::priority_queue<std::pair<int32_t, int32_t>> due;
    // A share small enough to round to nothing is not queued: queued again and again, cells holding
    // nothing would be passed on once for every way into them.
    const auto add = [&](int32_t cell, float water) {
        if (water <= 0) return;
        if (extra[size_t(cell)] <= 0) due.emplace(domain.rank[size_t(cell)], cell);
        extra[size_t(cell)] += water;
    };
    const auto spill = [&](int32_t cell, float water) {
        int32_t id = work.lakeOf[size_t(cell)];
        if (id < 0) {
            id = int32_t(work.lakes.size());
            work.lakes.emplace_back(), work.pour.push_back(Unknown);
            basin(domain, share, work, cell, id, stack);
            work.pour[size_t(id)] = pourOut(domain, work, id);
        } else if (const int32_t pour = work.pour[size_t(id)]; pour == Unknown || (pour != EdgePass && work.lakeOf[size_t(pour)] >= 0)) {
            // Its pass led into a lake filled since: the two are one now.
            work.pour[size_t(id)] = pourOut(domain, work, id);
        }
        work.spilled += water;
        if (work.pour[size_t(id)] == EdgePass) work.overEdge += water;
        else add(work.pour[size_t(id)], water);
    };

    // Each step of the route starts its sinking afresh (startRoute), so this is the route's own.
    for (size_t i = 0; i < cells; ++i)
        if (work.ponded[i] > 0) spill(int32_t(i), work.ponded[i]);
    // Water passes only along the route's shares, downward in its order, and from a hollow over a
    // pass out of every catchment that drains to it: it cannot come back to a cell it left. A
    // queue that does not empty in this many passes is a fault, not water to drop.
    const size_t bound = 64 * cells + 64;
    for (size_t passes = 0; !due.empty(); ++passes) {
        if (passes > bound) throw std::logic_error("spillHollows: water runs in a circle");
        const int32_t c = due.top().second;
        due.pop();
        const float water = extra[size_t(c)];
        extra[size_t(c)] = 0;
        // A finished neighbour's cell hands on what that neighbour recorded (landscape/fn/Flow.cpp):
        // water reaching it has left this domain.
        if (domain.hold[size_t(c)] == Settled) {
            work.overEdge += water;
            continue;
        }
        // A free cell the route passed nothing on from: an open hollow, where its floor sinks.
        if (domain.hold[size_t(c)] == Free && outOf(share, size_t(c)) <= 0) {
            // What the floor still takes, then over the pass; the cell keeps, as in the route, what
            // did not go down.
            float take = 0;
            if (work.rate[size_t(c)] > 0) {
                const float took = work.received[size_t(c)] >= 0 ? work.received[size_t(c)] - work.ponded[size_t(c)] : 0.0f;
                take = std::min(water, std::max(0.0f, work.rate[size_t(c)] * area - took));
                work.received[size_t(c)] = std::max(0.0f, work.received[size_t(c)]) + water;
                work.ponded[size_t(c)] += water - take;
                work.sunk[size_t(c)] += take, work.total += take;
            }
            domain.discharge[size_t(c)] += water - take;
            if (gathered) (*gathered)[size_t(c)] += water - take;
            if (water > take) spill(c, water - take);
            continue;
        }
        domain.discharge[size_t(c)] += water;
        if (gathered) (*gathered)[size_t(c)] += water;
        const int x = c % size, z = c / size;
        for (int k = 0; k < 8; ++k)
            if (const float part = share[size_t(c) * 8 + size_t(k)]; part > 0) add(int32_t(domain.index(x + NeighborX[k], z + NeighborZ[k])), water * part);
    }
}

} // namespace derivean::sim

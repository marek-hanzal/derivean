#include "groundwater/fn/Karst.hpp"

#include <queue>

namespace derivean::sim {
namespace {

constexpr int Rounds = 5;
constexpr float PlaneEase[3] = {1.0f, 3.0f, 12.0f};   // how much easier water follows a joint corridor and a joint zone than unbroken rock
constexpr float PlaneReach = 45.0f;       // m from a cell's centre within which a plane passes through the cell
constexpr float FlowUnit = 3.0e5f;        // m³/yr (10 l/s) of use that makes a path as easy again as a joint zone made it
constexpr float FlowEase = 12.0f;
constexpr float Entry = 600.0f;           // m of unbroken rock an unused outlet is worth; less where a zone meets it, far less once water uses it
constexpr float OffZone = 4.0f;
constexpr float LastResort = 6000.0f;     // m of unbroken rock beyond which water finds its way down and out of the model instead
constexpr float Climb = 60.0f;            // how much harder water on a tight bed gets up the bed than along it, per unit of rise over run

float grown(float used) { return FlowEase * std::pow(used / FlowUnit, 0.8f); }

struct Paths {
    const WaterField& field;
    std::vector<float> used;        // nodes × 8: m³/yr each step carried in the round before
    std::vector<float> usedOut;     // per node: what left through it as an outlet
    std::vector<float> cost;        // m of unbroken rock between a node and the surface

    // Resistance of the step from node (from, storey) towards neighbour k.
    float step(size_t from, int storey, size_t to, int toStorey, int k) const {
        const float length = float(LocalCell) * NeighborReach[k];
        int set = 0;
        const float ease = PlaneEase[planeAlong(field, from, to, k, set)] + grown(used[(from * 2 + size_t(storey)) * 8 + size_t(k)]);
        const float rise = storey == Upper && toStorey == Upper ? field.column[to].floor - field.column[from].floor : 0.0f;
        return length / ease * (1.0f + Climb * std::max(0.0f, rise) / length);
    }
};

Outlet outletOf(const WaterField& field, size_t cell, int storey) {
    const AquiferColumn& column = field.column[cell];
    if (column.drain != Drain::None) return storey == Upper ? Outlet::Lip : Outlet::Rising;
    if (storey == Lower) return field.level[cell * 2 + Lower] >= column.ground - 1.0f ? Outlet::Rising : Outlet::Underflow;
    for (int k = 0; k < 8; ++k)
        if (Way(field.way[cell * 8 + size_t(k)]) == Way::Outcrop && field.share[(cell * 2 + Upper) * 8 + size_t(k)] > 0) return Outlet::Lip;
    return Outlet::Underflow;
}

bool onZone(const WaterField& field, size_t cell) {
    if (field.column[cell].fissures < 0) return false;
    const Fissures& fissures = field.fissures[cell];
    return std::min(std::abs(fissures.zone[0]), std::abs(fissures.zone[1])) <= PlaneReach;
}

} // namespace

int planeAlong(const WaterField& field, size_t from, size_t to, int k, int& set) {
    if (field.column[from].fissures < 0 || field.column[to].fissures < 0) return 0;
    const Fissures &here = field.fissures[from], &there = field.fissures[to];
    const float stepX = float(NeighborX[k]) * float(LocalCell), stepZ = float(NeighborZ[k]) * float(LocalCell), length = float(LocalCell) * NeighborReach[k];
    int best = 0;
    for (int s = 0; s < 2; ++s) {
        const float c = std::cos(here.strike[s]), n = std::sin(here.strike[s]);
        if (std::abs(c * stepX + n * stepZ) < 0.64f * length) continue;
        // The same plane seen from both cells: the second cell's offset is the first one's less the step across the strike.
        const float across = -n * stepX + c * stepZ;
        const auto same = [&](float a, float b) { return std::abs(a) <= PlaneReach && std::abs(b) <= PlaneReach && std::abs(b - (a - across)) < 8.0f; };
        const int grade = same(here.zone[s], there.zone[s]) ? 2 : same(here.corridor[s], there.corridor[s]) ? 1 : 0;
        if (grade > best) best = grade, set = s;
    }
    return best;
}

KarstNet drainKarst(const WaterField& field) {
    const size_t nodes = field.cells() * 2;
    const int size = field.size;
    KarstNet net;
    net.parent.assign(nodes, -1), net.toward.assign(nodes, 255), net.outlet.assign(nodes, Outlet::None);
    net.root.assign(nodes, -1), net.flux.assign(nodes, 0.0), net.run.assign(nodes, 0.0f);
    std::vector<int32_t> members;
    for (int z = field.margin; z < size - field.margin; ++z)
        for (int x = field.margin; x < size - field.margin; ++x)
            for (int storey = Upper; storey <= Lower; ++storey)
                if (field.karst[field.index(x, z) * 2 + size_t(storey)]) members.push_back(int32_t(field.index(x, z) * 2 + size_t(storey)));
    if (members.empty()) return net;

    Paths paths{field, std::vector<float>(nodes * 8, 0.0f), std::vector<float>(nodes, 0.0f), std::vector<float>(nodes, 0.0f)};
    using Reached = std::pair<float, int32_t>;
    for (int round = 0; round < Rounds; ++round) {
        // Cheapest way out for every node, found from the outlets inward.
        std::priority_queue<Reached, std::vector<Reached>, std::greater<Reached>> open;
        for (const int32_t node : members) {
            const size_t cell = size_t(node) / 2;
            const Outlet outlet = outletOf(field, cell, node & 1);
            net.outlet[size_t(node)] = outlet, net.parent[size_t(node)] = -1, net.toward[size_t(node)] = 255;
            paths.cost[size_t(node)] = outlet == Outlet::Underflow ? LastResort : Entry * (onZone(field, cell) ? 1.0f : OffZone) / (1.0f + grown(paths.usedOut[size_t(node)]));
            open.push({paths.cost[size_t(node)], node});
        }
        while (!open.empty()) {
            const auto [reached, node] = open.top();
            open.pop();
            if (reached > paths.cost[size_t(node)]) continue;
            const size_t to = size_t(node) / 2;
            const int toStorey = node & 1, tx = int(to % size_t(size)), tz = int(to / size_t(size));
            for (int k = 0; k < 8; ++k) {
                const int fx = tx - NeighborX[k], fz = tz - NeighborZ[k];
                if (!field.owned(fx, fz)) continue;
                const size_t from = field.index(fx, fz);
                const Way way = Way(field.way[from * 8 + size_t(k)]);
                for (int storey = Upper; storey <= Lower; ++storey) {
                    const size_t source = from * 2 + size_t(storey);
                    if (!field.karst[source]) continue;
                    const bool joined = storey == Lower ? toStorey == Lower : (toStorey == Upper ? way == Way::Along : way == Way::Under);
                    if (!joined) continue;
                    const float through = reached + paths.step(from, storey, to, toStorey, k);
                    if (through >= paths.cost[source]) continue;
                    paths.cost[source] = through;
                    net.parent[source] = node, net.toward[source] = uint8_t(k), net.outlet[source] = Outlet::None;
                    open.push({through, int32_t(source)});
                }
            }
        }
        // What each node took in runs down its path; every step remembers what it carried.
        net.order = members;
        std::sort(net.order.begin(), net.order.end(), [&](int32_t a, int32_t b) {
            return paths.cost[size_t(a)] != paths.cost[size_t(b)] ? paths.cost[size_t(a)] > paths.cost[size_t(b)] : a < b;
        });
        std::fill(paths.used.begin(), paths.used.end(), 0.0f);
        std::fill(paths.usedOut.begin(), paths.usedOut.end(), 0.0f);
        for (const int32_t node : members) net.flux[size_t(node)] = field.fate[size_t(node)].captured;
        for (const int32_t node : net.order) {
            const int32_t parent = net.parent[size_t(node)];
            if (parent < 0) {
                paths.usedOut[size_t(node)] = float(net.flux[size_t(node)]);
                continue;
            }
            net.flux[size_t(parent)] += net.flux[size_t(node)];
            paths.used[size_t(node) * 8 + net.toward[size_t(node)]] = float(net.flux[size_t(node)]);
        }
    }
    for (size_t at = net.order.size(); at-- > 0;) {
        const int32_t node = net.order[at], parent = net.parent[size_t(node)];
        net.root[size_t(node)] = parent < 0 ? node : net.root[size_t(parent)];
        net.run[size_t(node)] = parent < 0 ? 0.0f : net.run[size_t(parent)] + float(LocalCell) * NeighborReach[net.toward[size_t(node)]];
    }
    return net;
}

} // namespace derivean::sim

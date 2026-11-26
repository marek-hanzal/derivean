#include "groundwater/fn/Karst.hpp"

#include <queue>

namespace derivean::sim {
namespace {

constexpr int Rounds = 5;
constexpr float PlaneEase[3] = {1.0f, 3.0f, 12.0f};   // how much easier water follows a joint corridor and a joint zone than unbroken rock
constexpr float Fall = 0.02f;             // m of head a metre of unbroken rock uses up
constexpr float FlowUnit = 3.0e5f;        // m³/yr (10 l/s) of use that makes a step as easy again as a joint zone made it
constexpr float FlowEase = 12.0f;
constexpr float EntryHead = 3.0f;         // m of head it takes to open a way out to the surface where a joint zone meets it
constexpr float OffZone = 4.0f;           // times as much where none does
constexpr float OpenWater = 0.25f;        // share of it into a stream or standing water, and over the lip of a bed
constexpr float Thin = 0.3f;              // m of water running on a tight bed

float grown(float used) { return FlowEase * std::pow(used / FlowUnit, 0.8f); }

bool onZone(const WaterField& field, size_t cell) {
    if (!field.column[cell].fissured) return false;
    const Fissures& fissures = field.fissures[cell];
    return std::min(std::abs(fissures.zone[0]), std::abs(fissures.zone[1])) <= PlaneReach;
}

// What the round before left behind.
struct Use {
    std::vector<float> step;     // nodes × 8: m³/yr each step carried
    std::vector<float> outlet;   // per node: m³/yr that left through it
};

// Whether the water of a node can go on to the node (to, toStorey) in direction k.
bool joined(const WaterField& field, size_t from, int storey, int toStorey, int k) {
    if (storey == Lower) return toStorey == Lower;
    const Way way = Way(field.way[from * 8 + size_t(k)]);
    return toStorey == Upper ? way == Way::Along : way == Way::Under;
}

// m of head the step from a node towards neighbour k uses up.
float lossOf(const WaterField& field, const Use& use, size_t from, int storey, size_t to, int k) {
    int set = 0;
    return float(WaterCell) * NeighborReach[k] * Fall / (PlaneEase[planeAlong(field, from, to, k, set)] + grown(use.step[(from * 2 + size_t(storey)) * 8 + size_t(k)]));
}

// m: the lowest a node's water can stand: on the tight bed it runs on, for the upper storey.
float leastHead(const WaterField& field, size_t node) { return node & 1 ? -1e30f : field.column[node / 2].floor + Thin; }

// The level at which a node's water gets out without passing another node of the tile, and how.
float wayOut(const WaterField& field, const Use& use, size_t node, Outlet& outlet) {
    const size_t cell = node / 2;
    const int storey = int(node & 1), x = int(cell % size_t(field.size)), z = int(cell / size_t(field.size));
    const AquiferColumn& column = field.column[cell];
    const float entry = EntryHead * (onZone(field, cell) ? 1.0f : OffZone) / (1.0f + grown(use.outlet[node]));
    outlet = Outlet::Rising;
    float level = column.ground + entry;
    if (column.drain != Drain::None) level = std::min(level, column.stage + OpenWater * entry);
    for (int k = 0; k < 8; ++k) {
        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
        if (nx < 0 || nz < 0 || nx >= field.size || nz >= field.size) continue;
        const size_t other = field.index(nx, nz);
        if (storey == Upper && Way(field.way[cell * 8 + size_t(k)]) == Way::Outcrop) {
            const float lip = std::min(sheetToward(column, k), field.column[other].ground) + OpenWater * entry;
            if (lip < level) level = lip, outlet = Outlet::Lip;
        }
        // Beyond the edge the water table of the same rock, as both tiles know it, takes the water on.
        for (int toStorey = storey; toStorey <= Lower; ++toStorey) {
            if (field.owned(nx, nz) || !field.karst[other * 2 + size_t(toStorey)] || !joined(field, cell, storey, toStorey, k)) continue;
            const float beyond = field.level[other * 2 + size_t(toStorey)] + lossOf(field, use, cell, storey, other, k);
            if (beyond < level) level = beyond, outlet = Outlet::Underflow;
        }
    }
    return level;
}

} // namespace

int planeAlong(const WaterField& field, size_t from, size_t to, int k, int& set) {
    if (!field.column[from].fissured || !field.column[to].fissured) return 0;
    const Fissures &here = field.fissures[from], &there = field.fissures[to];
    const float stepX = float(NeighborX[k]) * float(WaterCell), stepZ = float(NeighborZ[k]) * float(WaterCell), length = float(WaterCell) * NeighborReach[k];
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
    net.root.assign(nodes, -1), net.flux.assign(nodes, 0.0), net.head.assign(nodes, 0.0f), net.run.assign(nodes, 0.0f);
    std::vector<int32_t> members;
    for (int z = field.margin; z < size - field.margin; ++z)
        for (int x = field.margin; x < size - field.margin; ++x)
            for (int storey = Upper; storey <= Lower; ++storey)
                if (field.karst[field.index(x, z) * 2 + size_t(storey)]) members.push_back(int32_t(field.index(x, z) * 2 + size_t(storey)));
    if (members.empty()) return net;

    Use use{std::vector<float>(nodes * 8, 0.0f), std::vector<float>(nodes, 0.0f)};
    std::vector<uint8_t> settled(nodes);
    using Reached = std::pair<float, int32_t>;
    for (int round = 0; round < Rounds; ++round) {
        // Lowest stand for every node, found from the lowest outlets upward. A node is settled when
        // it is first taken at its final stand; the node it drains to was settled before it.
        std::priority_queue<Reached, std::vector<Reached>, std::greater<Reached>> open;
        for (const int32_t node : members) {
            net.parent[size_t(node)] = -1, net.toward[size_t(node)] = 255, settled[size_t(node)] = 0;
            net.head[size_t(node)] = std::max(leastHead(field, size_t(node)), wayOut(field, use, size_t(node), net.outlet[size_t(node)]));
            open.push({net.head[size_t(node)], node});
        }
        net.order.clear();
        while (!open.empty()) {
            const auto [reached, node] = open.top();
            open.pop();
            if (reached > net.head[size_t(node)] || settled[size_t(node)]) continue;
            settled[size_t(node)] = 1;
            net.order.push_back(node);
            const size_t to = size_t(node) / 2;
            const int toStorey = node & 1, tx = int(to % size_t(size)), tz = int(to / size_t(size));
            for (int k = 0; k < 8; ++k) {
                const int fx = tx - NeighborX[k], fz = tz - NeighborZ[k];
                if (!field.owned(fx, fz)) continue;
                const size_t from = field.index(fx, fz);
                for (int storey = Upper; storey <= Lower; ++storey) {
                    const size_t source = from * 2 + size_t(storey);
                    if (!field.karst[source] || settled[source] || !joined(field, from, storey, toStorey, k)) continue;
                    const float through = std::max(leastHead(field, source), reached + lossOf(field, use, from, storey, to, k));
                    if (through >= net.head[source]) continue;
                    net.head[source] = through;
                    net.parent[source] = node, net.toward[source] = uint8_t(k), net.outlet[source] = Outlet::None;
                    open.push({through, int32_t(source)});
                }
            }
        }
        // What each node took in runs down its way, the last settled first; every step remembers what it carried.
        std::reverse(net.order.begin(), net.order.end());
        std::fill(use.step.begin(), use.step.end(), 0.0f);
        std::fill(use.outlet.begin(), use.outlet.end(), 0.0f);
        for (const int32_t node : members) net.flux[size_t(node)] = field.fate[size_t(node)].captured;
        for (const int32_t node : net.order) {
            const int32_t parent = net.parent[size_t(node)];
            if (parent < 0) {
                use.outlet[size_t(node)] = float(net.flux[size_t(node)]);
                continue;
            }
            net.flux[size_t(parent)] += net.flux[size_t(node)];
            use.step[size_t(node) * 8 + net.toward[size_t(node)]] = float(net.flux[size_t(node)]);
        }
    }
    for (size_t at = net.order.size(); at-- > 0;) {
        const int32_t node = net.order[at], parent = net.parent[size_t(node)];
        net.root[size_t(node)] = parent < 0 ? node : net.root[size_t(parent)];
        net.run[size_t(node)] = parent < 0 ? 0.0f : net.run[size_t(parent)] + float(WaterCell) * NeighborReach[net.toward[size_t(node)]];
    }
    return net;
}

} // namespace derivean::sim

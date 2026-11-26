#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Wind.hpp"

#include "hydrology/constant/Water.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

// m³ of evaporite a m³ of river water leaves when all of it goes off as vapour: about 0.3 kg/m³ of
// dissolved load. The routing does not carry solutes, so this is a fixed share of the water, and no
// tile lays down more salt than its own rock gave to solution.
constexpr float SoluteLoad = 1.4e-4f;
constexpr double Freeboard = 0.05;   // m the salt floor stays below the rim
constexpr int LevelSearch = 40;      // halvings of the search for the floor

} // namespace

// The flood fills every hollow to its spill. A basin is terminal when what flows into it is no more
// than the whole of that surface gives back to the air: its real lake would stand lower, at the
// level where the two balance, and never spill. The routing evaporates a lake only along the one
// path its water takes to the outlet, so its outlet flow cannot say this. What flowed in has left as
// vapour, and the salt it carried lies over the floor of the basin.
void dryBasins(Domain& domain, FlowScratch& scratch) {
    WindField& wind = domain.aeolian;
    const size_t cells = domain.cells();
    const int size = domain.size;
    const float area = float(domain.cell * domain.cell), dt = float(domain.step), sea = float(domain.seaLevel);
    if (scratch.share.size() != cells * 8) return;
    std::vector<int32_t> basin(cells, -1), members, stack;
    std::vector<float> inflow;
    std::vector<int32_t> first;   // start of each basin's cells in `members`
    const auto standing = [&](size_t i) { return underFlood(domain, i) && domain.level[i] > sea + 0.5f; };
    // The ground as the step's cutting and filling left it; `surface` is still the one the flood ran on.
    const auto ground = [&](size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); };
    for (size_t seed = 0; seed < cells; ++seed) {
        if (basin[seed] >= 0 || !standing(seed)) continue;
        const int32_t id = int32_t(first.size());
        first.push_back(int32_t(members.size()));
        stack.assign(1, int32_t(seed));
        basin[seed] = id;
        while (!stack.empty()) {
            const int32_t at = stack.back();
            stack.pop_back();
            members.push_back(at);
            for (int k = 0; k < 8; ++k) {
                const int nx = at % size + NeighborX[k], nz = at / size + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                const size_t next = domain.index(nx, nz);
                if (basin[next] < 0 && standing(next)) basin[next] = id, stack.push_back(int32_t(next));
            }
        }
    }
    first.push_back(int32_t(members.size()));
    inflow.assign(first.size(), 0.0f);
    std::vector<float> vapour(first.size(), 0.0f);   // m³/yr the full lake surface can lose
    for (size_t i = 0; i < cells; ++i) {
        if (basin[i] >= 0) vapour[size_t(basin[i])] += std::max(0.0f, evaporationDemand(domain.warmthNow[i]) - domain.rain[i]) * area;
        if (basin[i] >= 0) inflow[size_t(basin[i])] += domain.runoff[i] * area;
        for (int k = 0; k < 8; ++k) {
            if (scratch.share[i * 8 + size_t(k)] <= 0 || basin[i] >= 0) continue;
            const size_t next = domain.index(int(i % size_t(size)) + NeighborX[k], int(i / size_t(size)) + NeighborZ[k]);
            if (basin[next] >= 0) inflow[size_t(basin[next])] += domain.discharge[i] * scratch.share[i * 8 + size_t(k)];
        }
    }
    for (size_t id = 0; id + 1 < first.size(); ++id) {
        const auto begin = members.begin() + first[id], end = members.begin() + first[id + 1];
        bool whole = true;
        int32_t outlet = *begin;
        for (auto it = begin; it != end; ++it) {
            whole = whole && domain.hold[size_t(*it)] == Free;
            if (domain.rank[size_t(*it)] < domain.rank[size_t(outlet)]) outlet = *it;
        }
        // A basin that spills, or one this domain sees only part of, keeps its salt in solution.
        if (!whole || inflow[id] > vapour[id] || inflow[id] <= 0) continue;
        // At each wet phase the inflow floods the hollow and its brine spreads over all of it; the
        // salt it leaves lies level, so it builds a flat floor from the lowest ground up across the
        // whole flood, thin and wide, never a column on the lowest cell. The floor rises no faster
        // than salt pans accumulate, nor above the rim; what does not fit stays in solution.
        const double given = std::max(0.0, domain.ledger.dissolved - wind.ledger.salt);
        const double volume = std::min(given, double(SoluteLoad) * inflow[id] * dt);   // m³
        double lowest = double(domain.level[size_t(outlet)]);
        for (auto it = begin; it != end; ++it) lowest = std::min(lowest, double(ground(size_t(*it))));
        // The floor is level: it stands no higher than the rim, nor than the lowest ground plus
        // what a pan lays in a step.
        const double top = std::min(double(domain.level[size_t(outlet)]) - Freeboard, lowest + EvaporiteRate * dt);
        const auto crustAt = [&](size_t i, double floor) { return std::max(0.0, floor - double(ground(i))); };
        const auto laid = [&](double floor) {
            double sum = 0;
            for (auto it = begin; it != end; ++it) sum += crustAt(size_t(*it), floor);
            return sum * area;
        };
        // The highest floor whose crust the salt fills, between the lowest ground and the top.
        double floor = top;
        if (laid(top) > volume) {
            double low = lowest, high = top;
            for (int k = 0; k < LevelSearch; ++k) {
                const double middle = 0.5 * (low + high);
                (laid(middle) > volume ? high : low) = middle;
            }
            floor = low;
        }
        for (auto it = begin; it != end; ++it) {
            const size_t i = size_t(*it);
            const float crust = float(crustAt(i, floor));
            if (crust <= 0) continue;
            domain.ground.sediment[Fines][i] += crust;
            wind.labels.salt[i] += crust;
            bookWind(domain, i, crust);
            wind.ledger.salt += double(crust) * area;
        }
    }
}

} // namespace derivean::sim

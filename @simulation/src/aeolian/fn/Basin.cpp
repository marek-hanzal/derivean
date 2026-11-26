#include "aeolian/fn/Wind.hpp"

#include "aeolian/fn/Regime.hpp"

#include <numeric>

namespace derivean::sim {
namespace {

// m³ of evaporite a m³ of river water leaves when all of it goes off as vapour: about 0.3 kg/m³ of
// dissolved load. The routing does not carry solutes, so this is a fixed share of the water, and no
// tile lays down more salt than its own rock gave to solution.
constexpr float SoluteLoad = 1.4e-4f;
constexpr float MostCrust = 2.0f;   // m of evaporite one step can leave on a cell

} // namespace

// The flood fills every hollow to its spill and the routing then takes evaporation off the water
// as it crosses the hollow, so a basin whose water never reaches the spill is known by an outlet
// that passes none. What flowed in has left as vapour over an area the inflow and the air's thirst
// fix; that area is the lowest ground of the basin, the pan, and the salt the water carried is there.
void dryBasins(Domain& domain, FlowScratch& scratch) {
    WindField& wind = domain.aeolian;
    const size_t cells = domain.cells();
    const int size = domain.size;
    const float area = float(domain.cell * domain.cell), dt = float(domain.step), sea = float(domain.seaLevel);
    if (scratch.share.size() != cells * 8) return;
    std::vector<int32_t> basin(cells, -1), members, stack;
    std::vector<float> inflow;
    std::vector<int32_t> first;   // start of each basin's cells in `members`
    const auto standing = [&](size_t i) { return domain.level[i] > domain.surface[i] + 1e-3f && domain.level[i] > sea + 0.5f; };
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
    for (size_t i = 0; i < cells; ++i) {
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
        double thirst = 0;
        for (auto it = begin; it != end; ++it) {
            whole = whole && domain.hold[size_t(*it)] == Free;
            if (domain.rank[size_t(*it)] < domain.rank[size_t(outlet)]) outlet = *it;
            thirst += std::max(0.05, evaporationDemand(domain.warmthNow[size_t(*it)]) - domain.rain[size_t(*it)]);
        }
        // A basin that spills, or one this domain sees only part of, keeps its salt in solution.
        if (!whole || domain.discharge[size_t(outlet)] > 0 || inflow[id] <= 0) continue;
        thirst /= double(end - begin);
        const size_t pan = std::clamp(size_t(std::ceil(inflow[id] / (thirst * area))), size_t(1), size_t(end - begin));
        std::partial_sort(begin, begin + long(pan), end, [&](int32_t a, int32_t b) {
            return ground(size_t(a)) != ground(size_t(b)) ? ground(size_t(a)) < ground(size_t(b)) : a < b;
        });
        // Brine stands level, so its salt builds a level floor: the lowest cells of the pan are raised to one surface.
        const double given = std::max(0.0, domain.ledger.dissolved - wind.ledger.salt);
        const double volume = std::min(given, double(SoluteLoad) * inflow[id] * dt) / area;   // m over one cell
        double floor = ground(size_t(*begin)), used = 0;
        size_t under = 1;
        for (; under < pan; ++under) {
            const double step = (ground(size_t(begin[long(under)])) - floor) * double(under);
            if (used + step > volume) break;
            used += step, floor = ground(size_t(begin[long(under)]));
        }
        floor += (volume - used) / double(under);
        for (auto it = begin; it != begin + long(under); ++it) {
            const size_t i = size_t(*it);
            const float crust = std::min(MostCrust, float(floor - ground(i)));
            if (crust <= 0) continue;
            domain.ground.sediment[Fines][i] += crust;
            wind.labels.salt[i] += crust;
            bookWind(domain, i, crust);
            wind.ledger.salt += double(crust) * area;
        }
    }
}

} // namespace derivean::sim

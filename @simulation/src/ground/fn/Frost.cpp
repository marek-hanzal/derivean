#include "ground/fn/Frost.hpp"

#include "climate/fn/Seasons.hpp"
#include "ground/constant/Ground.hpp"

namespace derivean::sim {
namespace {

constexpr float SortingBelow = 0.5f;    // °C annual mean at the ground under which the ground sorts
constexpr float FlowingBelow = 2.0f;    // °C under which the thawed layer flows
constexpr int SortRounds = 64;
constexpr int NearReach = 2, FarReach = 6;   // cells: stones gather over the first, borders stand the second apart
constexpr float SortRate = 2.5f;        // share of a cell's stones that follows a unit difference per round
constexpr float Mixing = 0.05f;         // heave also stirs neighbours together: this share of their difference evens out per round
constexpr float SlopeMixing = 0.07f;    // more on a slope, where the stones' own travel evens them out along the fall line
constexpr int FlowRounds = 40;
constexpr float FlowRate = 0.9f;        // 1/m² per round: share of a column that leaves per unit slope and squared thickness
constexpr float ThawDepth = 0.8f;       // m of mantle that thaws
constexpr float Puddle = 0.05f;         // m of standing water under which ground still freezes and thaws as ground

// Mean over a square of cells around each, by two running sums. Rows, then columns: each is its own
// work, so they are spread over the domain's pool.
void boxMean(const Domain& domain, const std::vector<float>& from, std::vector<float>& into, std::vector<float>& work, int reach) {
    const int size = domain.size;
    const float scale = 1.0f / float(2 * reach + 1);
    forRows(domain, size, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            float sum = 0;
            const auto at = [&](int x) { return from[size_t(z) * size_t(size) + size_t(std::clamp(x, 0, size - 1))]; };
            for (int x = -reach; x <= reach; ++x) sum += at(x);
            for (int x = 0; x < size; ++x) work[size_t(z) * size_t(size) + size_t(x)] = sum * scale, sum += at(x + reach + 1) - at(x - reach);
        }
    });
    forRows(domain, size, [&](int begin, int end) {
        for (int x = begin; x < end; ++x) {
            float sum = 0;
            const auto at = [&](int z) { return work[size_t(std::clamp(z, 0, size - 1)) * size_t(size) + size_t(x)]; };
            for (int z = -reach; z <= reach; ++z) sum += at(z);
            for (int z = 0; z < size; ++z) into[size_t(z) * size_t(size) + size_t(x)] = sum * scale, sum += at(z + reach + 1) - at(z - reach);
        }
    });
}

bool sortStones(Domain& domain, FlowScratch& scratch, std::vector<uint8_t>& sorted) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    std::vector<float>& share = scratch.moved[0];
    std::vector<float>& near = scratch.moved[1];
    std::vector<float>& far = scratch.moved[2];
    std::vector<float>& work = scratch.moved[3];
    for (auto* field : {&share, &near, &far, &work}) field->assign(cells, 0.0f);
    std::vector<float>& coarse = domain.ground.sediment[Coarse];
    std::vector<float>& fines = domain.ground.sediment[Fines];
    bool any = false;
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const bool works = domain.owned(x, z) && domain.warmthNow[i] < SortingBelow && coarse[i] > 0.02f && fines[i] > 0.04f &&
                               domain.level[i] <= domain.surface[i] + Puddle && domain.stream[i] < BedFlow;
            sorted[i] = works;
            any = any || works;
        }
    if (!any) return false;
    for (int round = 0; round < SortRounds; ++round) {
        for (size_t i = 0; i < cells; ++i) share[i] = coarse[i] / (coarse[i] + fines[i] + 1e-4f);
        // Bell-shaped means: two box passes each.
        boxMean(domain, share, near, work, NearReach);
        boxMean(domain, near, near, work, NearReach);
        boxMean(domain, share, far, work, FarReach);
        boxMean(domain, far, far, work, FarReach);
        // Stones go to the neighbour where stones stand out more against their wider surroundings;
        // the same volume of fines comes back, so neither cell changes its thickness. East and south
        // pairs once each: a pair trades one way only.
        for (int z = Halo; z < Halo + TileCells; ++z)
            for (int x = Halo; x < Halo + TileCells; ++x) {
                const size_t a = domain.index(x, z);
                if (!sorted[a]) continue;
                for (const size_t b : {a + 1, a + size_t(size)}) {
                    if (!sorted[b]) continue;
                    // On a slope the pair along the fall line sorts less and mixes more: stripes run down it.
                    const float drop = std::abs(domain.surface[a] - domain.surface[b]) / float(domain.cell);
                    const float mantle = 0.5f * (coarse[a] + fines[a] + coarse[b] + fines[b]);
                    // Fines heave and close up into plugs; the stones are what gets pushed aside. So ground
                    // sorts fastest where fines prevail, and it is the stones that end up as the net between.
                    const float stiff = 0.5f * (1.0f - 0.5f * (share[a] + share[b]));
                    const float pull = SortRate * ((near[b] - far[b]) - (near[a] - far[a])) / (1.0f + 12.0f * drop) * stiff -
                                       (Mixing + SlopeMixing * std::min(1.0f, 6.0f * drop)) * (share[b] - share[a]);
                    const size_t from = pull > 0 ? a : b, to = pull > 0 ? b : a;
                    const float moved = std::min({std::abs(pull) * mantle, 0.25f * coarse[from], 0.25f * fines[to]});
                    coarse[from] -= moved, coarse[to] += moved;
                    fines[to] -= moved, fines[from] += moved;
                }
            }
    }
    return true;
}

bool flowThawed(Domain& domain, FlowScratch& scratch) {
    const int size = domain.size;
    std::vector<float>* moved = scratch.moved;
    const float cell = float(domain.cell);
    bool any = false;
    for (int z = Halo; z < Halo + TileCells && !any; ++z)
        for (int x = Halo; x < Halo + TileCells && !any; ++x) {
            const size_t i = domain.index(x, z);
            any = domain.warmthNow[i] < FlowingBelow && domain.ground.sediment[Fines][i] > 0.1f;
        }
    if (!any) return false;
    for (int round = 0; round < FlowRounds; ++round) {
        for (int c = 0; c < SedimentClasses; ++c) moved[c].assign(domain.cells(), 0.0f);
        for (int z = Halo + 1; z < Halo + TileCells - 1; ++z)
            for (int x = Halo + 1; x < Halo + TileCells - 1; ++x) {
                const size_t i = domain.index(x, z);
                const float mantle = domain.sedimentAt(i), fines = domain.ground.sediment[Fines][i];
                if (domain.warmthNow[i] >= FlowingBelow || fines < 0.1f || domain.stream[i] >= BedFlow || domain.level[i] > domain.surface[i] + Puddle) continue;
                const float here = domain.ground.rock[i] + mantle;
                int lowest = -1;
                float steepest = 0.03f;
                for (int k = 0; k < 8; ++k) {
                    const size_t other = domain.index(x + NeighborX[k], z + NeighborZ[k]);
                    const float slope = (here - domain.ground.rock[other] - domain.sedimentAt(other)) / (cell * NeighborReach[k]);
                    if (slope > steepest) steepest = slope, lowest = k;
                }
                if (lowest < 0) continue;
                // Only the thawed part of the column flows, and only while that leaves it above its neighbour.
                const float thawed = std::min(mantle, ThawDepth);
                const float leaves = std::min({FlowRate * thawed * thawed * std::min(steepest, 0.6f) * thawed, 0.2f * thawed, 0.4f * steepest * cell * NeighborReach[lowest]});
                const size_t to = domain.index(x + NeighborX[lowest], z + NeighborZ[lowest]);
                for (int c = 0; c < SedimentClasses; ++c) {
                    const float part = leaves * domain.ground.sediment[c][i] / mantle;
                    moved[c][i] -= part, moved[c][to] += part;
                }
            }
        for (int c = 0; c < SedimentClasses; ++c)
            for (size_t i = 0; i < domain.cells(); ++i) domain.ground.sediment[c][i] = std::max(0.0f, domain.ground.sediment[c][i] + moved[c][i]);
    }
    return true;
}

} // namespace

bool workFrozenGround(Domain& domain, FlowScratch& scratch, std::vector<uint8_t>& sorted) {
    sorted.assign(domain.cells(), 0);
    const bool flowed = flowThawed(domain, scratch);
    return sortStones(domain, scratch, sorted) || flowed;
}

float frostAt(const Domain& domain, size_t cell) { return float(frostCracking(domain.warmthNow[cell], domain.halfRange)); }

} // namespace derivean::sim

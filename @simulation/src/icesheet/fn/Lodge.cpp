#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Region.hpp"
#include "icesheet/fn/Wear.hpp"

#include <numeric>

namespace derivean::sim {
namespace {

constexpr int LodgeCells = 10;              // cells over which a clean bed lodges all but 1/e of the load passing over it
constexpr int ShelterCells = 8;             // cells up-ice the bed is searched for a rise that shelters a cell
constexpr int MeanRadius = Refinement / 2;  // cells: rock is weak or strong against the mean over a parent cell around it
// Cells over which the ice plucks as one: a basin grows from a zone of close planes until its rim
// stands in sound rock, so it is wider than any one plane a cell resolves.
constexpr int PluckRadius = 2;
constexpr float CoverShield = 1.0f;         // m of cover left over rock that halves what the ice digs from it, twice over

float groundOf(const Domain& domain, size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); }

// Lays `amount` (m by class) on cell i and books it.
void lay(Domain& domain, size_t i, const float amount[SedimentClasses], std::vector<float>& kind) {
    float all = 0;
    for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] += amount[c], all += amount[c];
    domain.deposited[i] += all;
    kind[i] += all;
    if (domain.owned(int(i % size_t(domain.size)), int(i / size_t(domain.size)))) domain.ledger.deposited += double(all) * domain.cell * domain.cell;
}

// Unit vector the last maximum's ice moved along at a cell: its sliding, or down its surface where it did not slide.
Vec2 flowAt(const Domain& domain, const ParentSheet& parent, int x, int z) {
    const size_t i = domain.index(x, z);
    if (std::hypot(parent.slideX[i], parent.slideZ[i]) >= LeastSlide) return normalized({parent.slideX[i], parent.slideZ[i]});
    const int west = std::max(0, x - 1), east = std::min(domain.size - 1, x + 1), north = std::max(0, z - 1), south = std::min(domain.size - 1, z + 1);
    const Vec2 fall{parent.topThen[domain.index(west, z)] - parent.topThen[domain.index(east, z)], parent.topThen[domain.index(x, north)] - parent.topThen[domain.index(x, south)]};
    return length(fall) > 1e-6 ? normalized(fall) : Vec2{};
}

// m of debris a finished neighbour's last sheet carried on from a domain cell it owns; negative where none is recorded.
float recordedPassing(const Domain& domain, int x, int z) {
    const int tx = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, tz = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
    const Tile* tile = tx || tz ? domain.finished[(tz + 1) * 3 + tx + 1] : nullptr;
    if (!tile || !tile->sheet || tile->sheet->passing.empty()) return -1;
    return tile->sheet->passing[size_t(z - Halo - tz * TileCells) * TileCells + size_t(x - Halo - tx * TileCells)];
}

} // namespace

SheetLoad takeUpBed(Domain& domain, SheetWork& work, const ParentSheet& parent) {
    const size_t cells = domain.cells();
    const double area = domain.cell * domain.cell;
    const float lodged = 1.0f - std::exp(-1.0f / float(LodgeCells));
    SheetLoad carried;
    for (auto& part : carried.load) part.assign(cells, 0.0f);
    carried.unstripped.assign(cells, 0.0f);
    std::vector<float> factor(cells);
    for (size_t i = 0; i < cells; ++i) factor[i] = rockFactor(domain, i);
    const std::vector<float> plucked = boxMean(factor, domain.size, PluckRadius), around = boxMean(factor, domain.size, MeanRadius);
    const float deepest = domain.key.level == Level::Local ? BasinDepth : KnobRelief;
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const float speed = std::hypot(parent.slideX[i], parent.slideZ[i]);
            if (parent.iceThen[i] < LeastSheet) continue;
            const float cover = domain.sedimentAt(i), strip = speed < LeastSlide ? 0.0f : std::min(cover, LooseGain * WearRate * speed * LastSheetYears);
            if (domain.hold[i] != Free) {
                // A cell this domain does not evolve feeds the ice what its owner's ice carried on
                // from it; with no such record, the load in balance with what its bed would give.
                const float recorded = domain.hold[i] == Settled ? recordedPassing(domain, x, z) : -1.0f;
                const float passing = recorded >= 0 ? recorded : strip / lodged;
                if (recorded < 0) carried.unstripped[i] = strip;
                for (int c = 0; c < SedimentClasses; ++c) carried.load[c][i] = passing * (cover > 0 ? domain.ground.sediment[c][i] / cover : TillSplit[c]);
                continue;
            }
            if (strip <= 0 && speed < LeastSlide) continue;
            for (int c = 0; c < SedimentClasses; ++c) {
                const float taken = cover > 0 ? strip * domain.ground.sediment[c][i] / cover : 0.0f;
                domain.ground.sediment[c][i] -= taken;
                carried.load[c][i] += taken;
            }
            // The parent's sheets wore its cell as a whole; inside it they took the weaker rock first.
            const float shield = std::exp(-(cover - strip) / CoverShield);
            const float dig = shield * std::min(deepest, parent.worn[i] * std::max(0.0f, plucked[i] / around[i] - 1.0f));
            domain.ground.rock[i] -= dig;
            for (int c = 0; c < SedimentClasses; ++c) carried.load[c][i] += dig * TillSplit[c];
            work.worn[i] += dig;
            domain.eroded[i] += strip + dig;
            if (domain.owned(x, z)) domain.ledger.eroded += double(strip + dig) * area;
        }
    }
    return carried;
}

void lodgeTill(Domain& domain, SheetWork& work, const ParentSheet& parent, SheetLoad& carried) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const double area = domain.cell * domain.cell;
    std::vector<int32_t> order(cells);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](int32_t a, int32_t b) { return parent.topThen[size_t(a)] != parent.topThen[size_t(b)] ? parent.topThen[size_t(a)] > parent.topThen[size_t(b)] : a < b; });
    std::vector<int32_t> rank(cells);
    for (size_t k = 0; k < cells; ++k) rank[size_t(order[k])] = int32_t(k);

    for (const int32_t cell : order) {
        const size_t i = size_t(cell);
        const int x = cell % size, z = cell / size;
        float load[SedimentClasses], total = 0;
        for (int c = 0; c < SedimentClasses; ++c) total += load[c] = carried.load[c][i];
        if (total <= 0) continue;
        const bool free = domain.hold[i] == Free, owned = domain.owned(x, z), iced = parent.iceThen[i] >= LeastSheet;
        // A cell that does not evolve keeps nothing; beyond the ice it is not carried on either.
        if (!free && !iced) continue;
        // As high as till stands against the lowest neighbour.
        const float here = groundOf(domain, i);
        float room = 1e30f;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx >= 0 && nz >= 0 && nx < size && nz < size) room = std::min(room, groundOf(domain, domain.index(nx, nz)) + TillRepose * NeighborReach[k] * float(domain.cell) - here);
        }
        room = std::max(0.0f, room);
        if (!iced) {
            // Beyond the ice: what it brought stays as moraine as steep as till stands; the meltwater takes the rest.
            const float kept = std::min(1.0f, room / total);
            const float stays[SedimentClasses] = {load[0] * kept, load[1] * kept, load[2] * kept};
            lay(domain, i, stays, work.left.moraine);
            if (owned) domain.ledger.sedimentOut += double(total * (1 - kept)) * area;
            continue;
        }
        const Vec2 flow = flowAt(domain, parent, x, z);
        float kept = 0;
        // Down-ice neighbours, by how closely they lie along the flow.
        float weight[8], weights = 0;
        for (int k = 0; k < 8; ++k) {
            weight[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            const bool beyond = nx < 0 || nz < 0 || nx >= size || nz >= size;
            if (!beyond && rank[domain.index(nx, nz)] <= rank[i]) continue;
            const float along = float(NeighborX[k] * flow.x + NeighborZ[k] * flow.z) / NeighborReach[k];
            if (along > 0) weights += weight[k] = along * along * along * along;
        }
        if (free) {
            // The cavity the ice leaves behind every rise up-ice: the bed would have to stand on a
            // line falling from that rise at ShelterSlope. Till squeezed into it fills it first; a
            // cell it fills shelters the next, so the fill runs out behind the rise as a tail.
            float cavity = 0;
            for (int k = 1; k <= ShelterCells; ++k) {
                const int ux = x - int(std::lround(flow.x * k)), uz = z - int(std::lround(flow.z * k));
                if (ux < 0 || uz < 0 || ux >= size || uz >= size) break;
                const size_t u = domain.index(ux, uz);
                cavity = std::max(cavity, groundOf(domain, u) - carried.unstripped[u] - ShelterSlope * float(k * domain.cell) - here);
            }
            const float lodging = weights <= 0 ? 1.0f : std::min(1.0f, cavity / total + (1.0f - std::exp(-1.0f / float(LodgeCells))));
            // A hollow in the ice surface has nowhere to pass its load on: it keeps all of it.
            kept = weights <= 0 ? 1.0f : std::min(lodging, room / total);
            const float stays[SedimentClasses] = {load[0] * kept, load[1] * kept, load[2] * kept};
            lay(domain, i, stays, work.left.till);
        }
        if (free) work.left.passing[i] = total * (1 - kept);
        if (kept >= 1.0f || weights <= 0) continue;
        for (int k = 0; k < 8; ++k) {
            if (weight[k] <= 0) continue;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            const float share = (1 - kept) * weight[k] / weights;
            // Past the domain's edge: only a cell of the halo sends anything there, and it is not the tile's.
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t n = domain.index(nx, nz);
            // A cell this domain does not evolve carries its own balanced load; what enters it has left the tile.
            if (domain.hold[n] == Free)
                for (int c = 0; c < SedimentClasses; ++c) carried.load[c][n] += load[c] * share;
            const bool into = domain.owned(nx, nz);
            if (owned && !into) domain.ledger.sedimentOut += double(total * share) * area;
            if (!owned && into) domain.ledger.sedimentIn += double(total * share) * area;
        }
    }
}

} // namespace derivean::sim

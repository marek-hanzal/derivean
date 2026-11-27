// About 280 lines, one owner: the last sheet's pass over a finer domain's bed. What it takes up
// and where it lodges it read the same flow, the same shelter and the same stripped depth.
#include "icesheet/fn/Local.hpp"

#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Meltwater.hpp"
#include "icesheet/fn/Region.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace derivean::sim {
namespace {

constexpr int LodgeCells = 10;              // cells over which a clean bed lodges all but 1/e of the load passing over it
constexpr int ShelterCells = 8;             // cells up-ice the bed is searched for a rise that shelters a cell
constexpr int MeanRadius = Refinement / 2;  // cells: rock is weak or strong against the mean over a parent cell around it
// Cells over which the ice plucks as one: a basin grows from a zone of close planes until its rim
// stands in sound rock, so it is wider than any one plane a cell resolves.
constexpr int PluckRadius = 2;
// Share of its full depth the ice digs from a weak zone it crosses square-on: it rides over a
// trench across its path, and is channelled down one that runs with it.
constexpr float AcrossShare = 0.25f;
constexpr float CoverShield = 1.0f;         // m of cover left over rock that halves what the ice digs from it, twice over

float groundOf(const Domain& domain, size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); }

// How readily the ice plucks a cell's rock, from the rock-mass field alone (its kind, the share
// broken along planes and faults, the cohesion of the jointed mass), so basins follow structure.
float pluckFactor(const Domain& domain, size_t i) {
    const float kind = std::clamp(std::sqrt(domain.unit[i].traits.erodibility / GraniteErodibility), 0.5f, 3.0f);
    return kind * (1.0f + 2.5f * domain.weakness[i]) / std::max(0.05f, domain.sound[i]);
}

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

// m of debris by class a finished neighbour's last sheet carried on from a domain cell it owns; false where none is recorded.
bool recordedPassing(const Domain& domain, int x, int z, float passing[SedimentClasses]) {
    size_t at = 0;
    const SheetRecord* record = finishedSheet(domain, x, z, at);
    if (!record || record->passing[0].empty()) return false;
    for (int c = 0; c < SedimentClasses; ++c) passing[c] = record->passing[c][at];
    return true;
}

} // namespace

SheetLoad takeUpBed(Domain& domain, SheetWork& work, const ParentSheet& parent) {
    const size_t cells = domain.cells();
    const double area = domain.cell * domain.cell;
    const float lodged = 1.0f - std::exp(-1.0f / float(LodgeCells));
    SheetLoad carried;
    for (auto& part : carried.load) part.assign(cells, 0.0f);
    carried.unstripped.assign(cells, 0.0f);
    // Rock basins are cut at Local; a Site inherits them in its parent's surface, and its own
    // planes, cut again, would only deepen the same basins along the grid of their joints.
    const float deepest = domain.key.level == Level::Local ? BasinDepth : 0.0f;
    std::vector<float> factor(cells, 1.0f);
    if (deepest > 0)
        for (size_t i = 0; i < cells; ++i) factor[i] = pluckFactor(domain, i);
    // Twice over the plucking footprint: a basin's walls slope from the zone into sound rock. Read
    // only where there are basins to cut.
    std::vector<float> plucked, around, along;
    if (deepest > 0) {
        boxMean(factor, domain.size, PluckRadius, plucked, along), boxMean(plucked, domain.size, PluckRadius, plucked, along);
        boxMean(factor, domain.size, MeanRadius, around, along);
    }
    // Which way the weak zones run, from the gradients of the plucking over its footprint taken
    // without their sign (a structure tensor): defined on a zone's axis as well as on its walls.
    std::vector<float> xx(cells, 0.0f), xz(cells, 0.0f), zz(cells, 0.0f);
    if (deepest > 0) {
        for (int z = 0; z < domain.size; ++z)
            for (int x = 0; x < domain.size; ++x) {
                const int west = std::max(0, x - 1), east = std::min(domain.size - 1, x + 1), north = std::max(0, z - 1), south = std::min(domain.size - 1, z + 1);
                const float gx = plucked[domain.index(east, z)] - plucked[domain.index(west, z)], gz = plucked[domain.index(x, south)] - plucked[domain.index(x, north)];
                const size_t i = domain.index(x, z);
                xx[i] = gx * gx, xz[i] = gx * gz, zz[i] = gz * gz;
            }
        for (std::vector<float>* field : {&xx, &xz, &zz}) boxMean(*field, domain.size, 2 * PluckRadius, *field, along);
    }
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const float speed = std::hypot(parent.slideX[i], parent.slideZ[i]);
            if (parent.iceThen[i] < LeastSheet) continue;
            const float cover = std::max(0.0f, domain.sedimentAt(i)), strip = speed < LeastSlide ? 0.0f : std::min(cover, LooseGain * WearRate * speed * LastSheetYears);
            if (domain.hold[i] != Free) {
                // A cell this domain does not evolve feeds the ice what its owner's ice carried on
                // from it; with no such record, the load in balance with what its bed would give.
                float recorded[SedimentClasses] = {};
                if (domain.hold[i] == Settled && recordedPassing(domain, x, z, recorded)) {
                    for (int c = 0; c < SedimentClasses; ++c) carried.load[c][i] = recorded[c];
                    continue;
                }
                carried.unstripped[i] = strip;
                for (int c = 0; c < SedimentClasses; ++c) carried.load[c][i] = strip / lodged * (cover > 0 ? domain.ground.sediment[c][i] / cover : TillSplit[c]);
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
            float dig = 0;
            if (deepest > 0 && plucked[i] > around[i]) {
                // How closely the zone runs with the flow: the share of the change in weakness that lies along the flow.
                const Vec2 flow = flowAt(domain, parent, x, z);
                const float total = xx[i] + zz[i];
                const float crossing = total > 1e-12f ? std::clamp(float(flow.x * flow.x * xx[i] + 2 * flow.x * flow.z * xz[i] + flow.z * flow.z * zz[i]) / total, 0.0f, 1.0f) : 0.0f;
                const float along = AcrossShare + (1 - AcrossShare) * (1 - crossing);
                // Deeper where the parent wore more and the zone is weaker, approaching BasinDepth.
                const float wanted = along * parent.worn[i] * (plucked[i] / around[i] - 1.0f);
                dig = shield * deepest * (1.0f - std::exp(-wanted / deepest));
            }
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
    std::vector<float> washed[2];
    for (auto& part : washed) part.assign(cells, 0.0f);
    std::vector<int32_t> rank(cells);
    for (size_t k = 0; k < cells; ++k) rank[size_t(order[k])] = int32_t(k);
    // The ground of every cell, kept as it is laid on.
    std::vector<float> ground(cells);
    for (size_t i = 0; i < cells; ++i) ground[i] = groundOf(domain, i);
    const float lodged = 1.0f - std::exp(-1.0f / float(LodgeCells));

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
        const float here = ground[i];
        float room = 1e30f, pit = 1e30f;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const float there = ground[domain.index(nx, nz)];
            room = std::min(room, there + TillRepose * NeighborReach[k] * float(domain.cell) - here);
            pit = std::min(pit, there - here);
        }
        room = std::max(0.0f, room);
        if (!iced) {
            // Beyond the ice: what it brought stays as moraine as steep as till stands; the meltwater
            // spreads the gravel and sand of the rest beyond and takes the mud away.
            const float kept = std::min(1.0f, room / total);
            const float stays[SedimentClasses] = {load[0] * kept, load[1] * kept, load[2] * kept};
            lay(domain, i, stays, work.left.moraine);
            ground[i] = groundOf(domain, i);
            // The rest starts down the ground from this cell's lower bare neighbours, so none of it
            // settles on the moraine; with none, the water takes all of it.
            float fall[8], falls = 0;
            for (int k = 0; k < 8; ++k) {
                fall[k] = 0;
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                const size_t n = domain.index(nx, nz);
                if (parent.iceThen[n] < LeastSheet && domain.hold[n] == Free && ground[n] < ground[i]) falls += fall[k] = (ground[i] - ground[n]) / NeighborReach[k];
            }
            float gone = load[Fines] * (1 - kept);
            if (falls <= 0) gone = total * (1 - kept);
            else
                for (int k = 0; k < 8; ++k) {
                    if (fall[k] <= 0) continue;
                    const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                    const size_t n = domain.index(nx, nz);
                    const float share = (1 - kept) * fall[k] / falls;
                    washed[0][n] += float(load[Coarse] * share * area), washed[1][n] += float(load[Sand] * share * area);
                    const bool into = domain.owned(nx, nz);
                    if (owned && !into) domain.ledger.sedimentOut += double((load[Coarse] + load[Sand]) * share) * area;
                    if (!owned && into) domain.ledger.sedimentIn += double((load[Coarse] + load[Sand]) * share) * area;
                }
            if (owned) domain.ledger.sedimentOut += double(gone) * area;
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
        // Where the flow points at no lower surface, the load goes down the surface as it falls.
        if (weights <= 0)
            for (int k = 0; k < 8; ++k) {
                const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                if (nx < 0 || nz < 0 || nx >= size || nz >= size || rank[domain.index(nx, nz)] <= rank[i]) continue;
                weights += weight[k] = std::max(1e-6f, (parent.topThen[i] - parent.topThen[domain.index(nx, nz)]) / NeighborReach[k]);
            }
        if (free) {
            // The cavity the ice leaves behind every rise up-ice: the bed would have to stand on a
            // line falling from that rise at ShelterSlope. Till squeezed into it fills it first; a
            // cell it fills shelters the next, so the fill runs out behind the rise as a tail.
            // A hollow narrower than a cell the ice cannot sole into: till fills it to its lowest rim.
            // Thin ice that stagnated as the sheet withdrew pressed into neither: the hollows under
            // it stay open as kettles, and its load passes over them as over the rest of the bed.
            const float pressing = std::clamp((parent.iceThen[i] - StagnantIce) / (PressingIce - StagnantIce), 0.0f, 1.0f);
            float cavity = pit < 1e30f ? std::max(0.0f, pit) : 0.0f;
            for (int k = 1; k <= ShelterCells && pressing > 0; ++k) {
                const int ux = x - int(std::lround(flow.x * k)), uz = z - int(std::lround(flow.z * k));
                if (ux < 0 || uz < 0 || ux >= size || uz >= size) break;
                const size_t u = domain.index(ux, uz);
                cavity = std::max(cavity, ground[u] - carried.unstripped[u] - ShelterSlope * float(k * domain.cell) - here);
            }
            cavity *= pressing;
            const float lodging = weights <= 0 ? 1.0f : std::min(1.0f, cavity / total + lodged);
            // A hollow in the ice surface has nowhere to pass its load on: it keeps what fits, no
            // higher than till stands against its lowest neighbour.
            kept = std::min(lodging, room / total);
            const float stays[SedimentClasses] = {load[0] * kept, load[1] * kept, load[2] * kept};
            lay(domain, i, stays, work.left.till);
            ground[i] = groundOf(domain, i);
        }
        if (free && weights > 0)
            for (int c = 0; c < SedimentClasses; ++c) work.left.passing[c][i] = load[c] * (1 - kept);
        if (kept >= 1.0f) continue;
        // The meltwater under the ice takes the rest of what a hollow cannot hold.
        if (weights <= 0) {
            if (owned) domain.ledger.sedimentOut += double(total * (1 - kept)) * area;
            continue;
        }
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
    std::vector<float> beyond(cells);
    for (size_t i = 0; i < cells; ++i) beyond[i] = parent.iceThen[i] >= LeastSheet ? 1e30f : ground[i];
    washBeyond(domain, beyond, washed, work.left.outwash);
}

} // namespace derivean::sim

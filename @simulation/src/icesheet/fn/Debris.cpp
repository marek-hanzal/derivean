#include "icesheet/fn/Wear.hpp"

#include "icesheet/constant/Ice.hpp"

#include <algorithm>
#include <numeric>

namespace derivean::sim {
namespace {

constexpr float RateShare = 2.0f / 3.0f;   // as in Wear.cpp

// Lays `volume` (m³ by class) on a free cell and books it.
void lay(Domain& domain, size_t i, const float volume[SedimentClasses], double span, std::vector<float>* kind, double& booked) {
    const float area = float(domain.cell * domain.cell);
    const float all = volume[0] + volume[1] + volume[2];
    if (all <= 0) return;
    for (int c = 0; c < SedimentClasses; ++c) domain.ground.sediment[c][i] += volume[c] / area;
    domain.deposited[i] += all / area;
    if (span > 0) domain.depositionRate[i] += RateShare * all / area / float(domain.step);
    if (kind) (*kind)[i] += all / area;
    if (domain.owned(int(i % size_t(domain.size)), int(i / size_t(domain.size)))) domain.ledger.deposited += all;
    booked += all;
}

} // namespace

std::vector<int32_t> downIceOrder(const SheetField& field) {
    std::vector<int32_t> order(field.top.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int32_t a, int32_t b) { return field.top[size_t(a)] != field.top[size_t(b)] ? field.top[size_t(a)] > field.top[size_t(b)] : a < b; });
    return order;
}

void carryDebris(Domain& domain, const SheetField& field, const std::vector<int32_t>& downIce, std::vector<float> debris[SedimentClasses], double years, double cycles,
                 double span, const std::vector<float>* tunnel, SheetRecord* left, WearTotals& totals) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const float area = float(domain.cell * domain.cell), sea = float(domain.seaLevel);
    const float lodging = 1.0f - std::exp(-float(domain.cell) / TillReach), tunnelling = 1.0f - std::exp(-float(domain.cell) / EskerReach);
    // Till lodged in one glaciation is taken up again by the next and carried on. Of the debris of
    // several cycles worked at once, the last cycle's share is lodged on its way as till; the rest
    // has only the margin to go to.
    const float last = 1.0f / float(std::max(1.0, cycles));

    // `fresh` is the part of `carried` that is the last cycle's.
    std::vector<float> carried[SedimentClasses], fresh[SedimentClasses];
    for (int c = 0; c < SedimentClasses; ++c) carried[c].assign(cells, 0.0f), fresh[c].assign(cells, 0.0f);
    for (const int32_t cell : downIce) {
        const size_t i = size_t(cell);
        const int x = cell % size, z = cell / size;
        float load[SedimentClasses] = {carried[0][i], carried[1][i], carried[2][i]};
        float young[SedimentClasses] = {fresh[0][i], fresh[1][i], fresh[2][i]};
        const bool free = domain.hold[i] == Free;
        const auto keep = [&](float room, std::vector<float>* kind, double& booked) {
            // Lays what fits into `room` (m³) here; the rest goes on with the meltwater and out of this account.
            const float all = load[0] + load[1] + load[2];
            if (all <= 0) return;
            const float kept = std::min(1.0f, room / all);
            const float stays[SedimentClasses] = {load[0] * kept, load[1] * kept, load[2] * kept};
            lay(domain, i, stays, span, kind, booked);
            totals.left += all * (1 - kept), domain.ledger.sedimentOut += all * (1 - kept);
        };
        if (field.thickness[i] <= 0) {
            // Ground the ice does not cover: what it brought this far stays as its end moraine, up
            // to the height a front can pile it (under the sea, up to the surface).
            const float ground = domain.ground.rock[i] + domain.sedimentAt(i);
            if (free) keep((ground < sea ? sea - ground : MoraineLimit) * area, left ? &left->moraine : nullptr, totals.dropped);
            continue;
        }
        if (!free) {
            // Ice over cells that do not evolve carries what it would in balance with its own bed:
            // as much as it lodges per cell is what it grinds off one.
            const float balance = WearRate * field.slide[i] * float(years) * rockFactor(domain, i) * area / lodging;
            for (int c = 0; c < SedimentClasses; ++c) load[c] = balance * TillSplit[c], young[c] = load[c] * last;
        } else {
            float lodged[SedimentClasses], all = 0;
            for (int c = 0; c < SedimentClasses; ++c) all += lodged[c] = young[c] * lodging;
            // No more than a front could pile: a bed cannot rise into its own ice.
            const float fits = all > 0 ? std::min(1.0f, MoraineLimit * area / all) : 0.0f;
            for (int c = 0; c < SedimentClasses; ++c) {
                lodged[c] *= fits;
                load[c] += debris[c][i] - lodged[c], young[c] += debris[c][i] * last - lodged[c];
            }
            lay(domain, i, lodged, span, left ? &left->till : nullptr, totals.lodged);
            if (tunnel && (*tunnel)[i] >= EskerFlow) {
                // A stream under the ice washes the mud on and leaves the gravel and sand in its tunnel.
                const float kept[SedimentClasses] = {young[Coarse] * tunnelling, young[Sand] * tunnelling, 0.0f};
                for (int c = 0; c < 2; ++c) load[c] -= kept[c], young[c] -= kept[c];
                lay(domain, i, kept, span, left ? &left->esker : nullptr, totals.lodged);
            }
        }
        float fall[8], total = 0;
        for (int k = 0; k < 8; ++k) {
            fall[k] = 0;
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t n = domain.index(nx, nz);
            if (field.top[n] < field.top[i]) total += fall[k] = (field.top[i] - field.top[n]) / NeighborReach[k];
        }
        if (total <= 0) {
            // A hollow in the ice surface: nowhere to pass it on.
            if (free) keep(MoraineLimit * area, left ? &left->till : nullptr, totals.lodged);
            continue;
        }
        // Only what crosses between the cells that evolve and the others is booked as entering or leaving.
        for (int k = 0; k < 8; ++k) {
            if (fall[k] <= 0) continue;
            const size_t n = domain.index(x + NeighborX[k], z + NeighborZ[k]);
            const bool into = domain.hold[n] == Free;
            if (!free && !into) continue;
            const float share = fall[k] / total, all = (load[0] + load[1] + load[2]) * share;
            if (free && !into) {
                totals.left += all, domain.ledger.sedimentOut += all;
                continue;
            }
            if (!free) totals.entered += all, domain.ledger.sedimentIn += all;
            for (int c = 0; c < SedimentClasses; ++c) carried[c][n] += load[c] * share, fresh[c][n] += young[c] * share;
        }
    }
}

} // namespace derivean::sim

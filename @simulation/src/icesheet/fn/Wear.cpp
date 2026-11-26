#include "icesheet/fn/Wear.hpp"

#include "icesheet/constant/Ice.hpp"

namespace derivean::sim {
namespace {

constexpr float RateShare = 2.0f / 3.0f;   // of a working's rate taken into the tile's recent mean: the fluvial step that follows takes it by 0.4 and keeps 0.6 of this

float groundOf(const Domain& domain, size_t i) { return domain.ground.rock[i] + domain.sedimentAt(i); }

// m: the lowest the ice may leave the ground of cell (x, z), or beyond reach where it may not cut at all.
float floorOf(const Domain& domain, const SheetField& field, int x, int z) {
    const size_t i = domain.index(x, z);
    const float sea = float(domain.seaLevel);
    float floor = 1e30f;
    for (int k = 0; k < 8; ++k) {
        const size_t n = domain.index(x + NeighborX[k], z + NeighborZ[k]);
        if (field.top[n] >= field.top[i]) continue;
        const float ground = groundOf(domain, n);
        if (field.thickness[n] <= 0 && ground < sea) {
            // Open water: this cell is the front. It stands only while the ice arriving outruns
            // what breaks off, which the same law as in the flow puts at this depth of water.
            const float front = std::sqrt(field.slide[i] * field.thickness[i] * float(IceToWater / CalvingRate));
            floor = std::min(floor, sea - front);
        } else {
            floor = std::min(floor, ground - Overdeepening * (field.top[i] - field.top[n]));
        }
    }
    return floor;
}

} // namespace

float rockFactor(const Domain& domain, size_t i) {
    const float kind = std::clamp(std::sqrt(domain.unit[i].traits.erodibility / GraniteErodibility), 0.5f, 3.0f);
    return kind * domain.quality[i] * (1.0f + 2.5f * domain.weakness[i]);
}

void wearBed(Domain& domain, SheetWork& work, const SheetField& field, const std::vector<int32_t>& downIce, double years, double span,
             std::vector<float> debris[SedimentClasses], WearTotals& totals) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const float area = float(domain.cell * domain.cell);
    for (int c = 0; c < SedimentClasses; ++c) debris[c].assign(cells, 0.0f);
    if (work.worn.size() != cells) work.worn.assign(cells, 0.0f);

    // From the margin inwards: every cell is cut against the ground its down-ice neighbours are
    // left with, so a floor that lowers as a whole is not held back and a basin is bounded by the
    // lip that does not lower.
    for (size_t step = cells; step-- > 0;) {
        const size_t i = size_t(downIce[step]);
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        if (field.slide[i] <= 0 || domain.hold[i] != Free || x == 0 || z == 0 || x == size - 1 || z == size - 1) continue;
        const float room = std::min(groundOf(domain, i) - floorOf(domain, field, x, z), DeepestCut * field.thickness[i]);
        if (room <= 0) continue;
        float effort = WearRate * field.slide[i] * float(years);
        const float cover = domain.sedimentAt(i), loose = std::min({cover, effort * LooseGain, room});
        effort -= loose / LooseGain;
        const float rock = std::min(effort * rockFactor(domain, i), room - loose), cut = loose + rock;
        if (cut <= 0) continue;
        for (int c = 0; c < SedimentClasses; ++c) {
            const float taken = cover > 0 ? loose * domain.ground.sediment[c][i] / cover : 0.0f;
            domain.ground.sediment[c][i] -= taken;
            debris[c][i] = (taken + rock * TillSplit[c]) * area;
        }
        domain.ground.rock[i] -= rock;
        work.worn[i] += rock;
        domain.eroded[i] += cut;
        if (span > 0) domain.erosionRate[i] += RateShare * cut / float(span);
        if (domain.owned(x, z)) domain.ledger.eroded += double(cut) * area;
        totals.worn += double(cut) * area;
    }
}

} // namespace derivean::sim

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

void wearBed(Domain& domain, SheetWork& work, const SheetField& field, double years, double span, std::vector<float> debris[SedimentClasses], WearTotals& totals) {
    const int size = domain.size;
    const size_t cells = domain.cells();
    const float area = float(domain.cell * domain.cell);
    for (int c = 0; c < SedimentClasses; ++c) debris[c].assign(cells, 0.0f);
    if (work.worn.size() != cells) work.worn.assign(cells, 0.0f);

    // Every cut is measured against the ground its neighbours had before this working, so a basin
    // deepens by no more than its outlet allows however the cells are visited.
    std::vector<float> loose(cells, 0.0f), rock(cells, 0.0f);
    for (int z = 1; z < size - 1; ++z) {
        for (int x = 1; x < size - 1; ++x) {
            const size_t i = domain.index(x, z);
            if (field.slide[i] <= 0 || domain.hold[i] != Free) continue;
            const float room = groundOf(domain, i) - floorOf(domain, field, x, z);
            if (room <= 0) continue;
            float effort = WearRate * field.slide[i] * float(years);
            loose[i] = std::min({domain.sedimentAt(i), effort * LooseGain, room});
            effort -= loose[i] / LooseGain;
            rock[i] = std::min(effort * rockFactor(domain, i), room - loose[i]);
        }
    }
    for (int z = 1; z < size - 1; ++z) {
        for (int x = 1; x < size - 1; ++x) {
            const size_t i = domain.index(x, z);
            const float cut = loose[i] + rock[i];
            if (cut <= 0) continue;
            const float cover = domain.sedimentAt(i);
            for (int c = 0; c < SedimentClasses; ++c) {
                const float taken = cover > 0 ? loose[i] * domain.ground.sediment[c][i] / cover : 0.0f;
                domain.ground.sediment[c][i] -= taken;
                debris[c][i] = (taken + rock[i] * TillSplit[c]) * area;
            }
            domain.ground.rock[i] -= rock[i];
            work.worn[i] += rock[i];
            domain.eroded[i] += cut;
            if (span > 0) domain.erosionRate[i] += RateShare * cut / float(span);
            if (domain.owned(x, z)) domain.ledger.eroded += double(cut) * area;
            totals.worn += double(cut) * area;
        }
    }
}

} // namespace derivean::sim

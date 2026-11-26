#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "chronology/type/Calendar.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// A stream of dry country runs a few days a year, in floods many times its yearly mean and as wide
// as they are; between them its bed drains into the dry ground under it. Where rain keeps up with
// what the air can take the ground under a bed stays wet and a stream gains from it as often as it
// loses. Measured channel losses of desert washes are some 1–10 % of a flood per kilometre.
constexpr float FloodWidth = 4.5f;    // m of flood channel per (m³/s)^0.5
constexpr float BedDrain = 5.0f;      // m of water a year a dry sand or gravel bed passes on to the ground under it
constexpr float BedDepth = 2.0f;      // m of loose bed below which the rock under it holds the water up

} // namespace

float sinkIntoBed(const Domain& domain, size_t i, float discharge) {
    if (discharge <= 0 || domain.hold[i] != Free || domain.ice[i] > 0) return 0;
    // Standing water is the lake's balance, which the routing strikes itself.
    if (domain.level[i] > domain.surface[i] + 1e-3f) return 0;
    const float coarse = std::max(0.0f, domain.ground.sediment[Coarse][i]), fines = std::max(0.0f, domain.ground.sediment[Fines][i]);
    const float sand = std::max(0.0f, domain.ground.sediment[Sand][i]) + (domain.aeolian.parked.size() == domain.cells() ? domain.aeolian.parked[i] : 0.0f);
    const float loose = coarse + sand + fines;
    if (loose <= 0) return 0;
    // Fines seal a bed in proportion to their share of it.
    const float open = (coarse + sand) / loose;
    const float cell = float(domain.cell);
    const float width = std::min(cell, FloodWidth * std::sqrt(discharge / (FloodShare * float(YearSeconds))));
    const float dry = aridityOf(domain.rain[i], domain.warmthNow[i]);
    return std::min(discharge, dry * open * std::min(1.0f, loose / BedDepth) * BedDrain * width * cell);
}

} // namespace derivean::sim

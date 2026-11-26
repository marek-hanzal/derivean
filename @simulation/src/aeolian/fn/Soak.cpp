#include "aeolian/fn/Wind.hpp"

#include "chronology/type/Calendar.hpp"
#include "hydrology/constant/Water.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// A stream of dry country runs a few days a year, in floods many times its yearly mean and as wide
// as they are; between them its bed drains into the dry ground under it. Where rain keeps up with
// what the air can take the ground under a bed stays wet and a stream gains from it as often as it
// loses. Measured channel losses of desert washes are some 1–10 % of a flood per kilometre.
constexpr float FloodShare = 0.02f;   // share of the year a stream of dry country runs
constexpr float FloodWidth = 4.5f;    // m of flood channel per (m³/s)^0.5
constexpr float BedDrain = 5.0f;      // m of water a year a dry sand or gravel bed passes on to the ground under it
constexpr float BedDepth = 2.0f;      // m of loose bed below which the rock under it holds the water up
// Rain on a sand sea sinks where it falls: wind-laid sand this thick sheds none.
constexpr float SandSoak = 0.5f;      // m
// A desert storm brings a few centimetres of rain; a loose cover a few times as thick holds all of
// it, and the sheet a storm sends over such ground sinks within a few hundred metres. Runoff in dry
// country therefore falls with the length of slope it is measured on.
constexpr float SheetDepth = 0.1f;    // m of loose cover that holds the sheet of a storm
constexpr float SheetReach = 300.0f;  // m a storm sheet runs over loose dry ground before most of it has sunk

// How much of the year the ground under a cell is dry: none where rain keeps up with the air.
float dryness(const Domain& domain, size_t i) {
    return std::clamp(1.0f - domain.rain[i] / evaporationDemand(domain.warmthNow[i]), 0.0f, 1.0f);
}

struct Bed {
    float loose, open;   // m of loose cover with the parked sand; share of it that is not fines
};
Bed bedOf(const Domain& domain, size_t i) {
    const float coarse = std::max(0.0f, domain.ground.sediment[Coarse][i]), fines = std::max(0.0f, domain.ground.sediment[Fines][i]);
    const float sand = std::max(0.0f, domain.ground.sediment[Sand][i]) + (domain.aeolian.parked.size() == domain.cells() ? domain.aeolian.parked[i] : 0.0f);
    const float loose = coarse + sand + fines;
    return {loose, loose > 0 ? (coarse + sand) / loose : 0.0f};
}

float blownSand(const Domain& domain, size_t i) {
    const WindField& wind = domain.aeolian;
    const float laid = wind.labels.blown.size() == domain.cells() ? std::min(wind.labels.blown[i], std::max(0.0f, domain.ground.sediment[Sand][i])) : 0.0f;
    return laid + (wind.parked.size() == domain.cells() ? wind.parked[i] : 0.0f);
}

} // namespace

float soakedShare(const Domain& domain, size_t i) {
    if (domain.hold[i] != Free || domain.ice[i] > 0) return 0;
    if (blownSand(domain, i) >= SandSoak) return 1;
    const Bed bed = bedOf(domain, i);
    return dryness(domain, i) * bed.open * std::min(1.0f, bed.loose / SheetDepth) * (1.0f - std::exp(-float(domain.cell) / SheetReach));
}

float sinkIntoBed(const Domain& domain, size_t i, float discharge) {
    if (discharge <= 0 || domain.hold[i] != Free || domain.ice[i] > 0) return 0;
    // Standing water is the lake's balance, which the routing strikes itself.
    if (domain.level[i] > domain.surface[i] + 1e-3f) return 0;
    const Bed bed = bedOf(domain, i);
    const float cell = float(domain.cell);
    const float width = std::min(cell, FloodWidth * std::sqrt(discharge / (FloodShare * float(YearSeconds))));
    // Fines seal a bed in proportion to their share of it.
    return std::min(discharge, dryness(domain, i) * bed.open * std::min(1.0f, bed.loose / BedDepth) * BedDrain * width * cell);
}

} // namespace derivean::sim

#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "chronology/type/Calendar.hpp"

namespace derivean::sim {
namespace {

constexpr float StreamWidth = 4.5f;   // m of channel per (m³/s)^0.5, as the ground level widens its streams
constexpr float Trace = 1e-4f;        // m below which a class counts as absent

} // namespace

bool heldByWater(const Domain& domain, size_t i) {
    if (domain.ice[i] > 0.5f) return true;
    const bool flooded = domain.level[i] > domain.surface[i] + 1e-3f;
    // A hollow the flood fills on paper is dry where no water is left to reach it.
    if (flooded && (domain.discharge[i] > 0 || domain.level[i] <= float(domain.seaLevel) + 0.5f)) return true;
    const float width = StreamWidth * std::sqrt(domain.discharge[i] / float(YearSeconds));
    return width >= 0.5f * float(domain.cell);
}

// A mixed cover has no layers: taking a share of its sand off the top leaves that share of its
// stones behind, and the wind stops once they amount to LagArmour.
float mobileSand(const Domain& domain, size_t i) {
    const WindLabels& labels = domain.aeolian.labels;
    const float sand = domain.ground.sediment[Sand][i];
    const float blown = std::min(labels.blown[i], sand), mixed = sand - blown;
    const float loose = domain.ground.sediment[Coarse][i] - std::min(labels.lag[i], domain.ground.sediment[Coarse][i]);
    const float room = std::max(0.0f, LagArmour - labels.lag[i]);
    return blown + (loose <= Trace ? mixed : std::min(mixed, room * mixed / loose));
}

float takeSand(Domain& domain, size_t i, float amount) {
    WindLabels& labels = domain.aeolian.labels;
    float* cover[SedimentClasses] = {&domain.ground.sediment[Coarse][i], &domain.ground.sediment[Sand][i], &domain.ground.sediment[Fines][i]};
    amount = std::min(amount, *cover[Sand]);
    const float fromBlown = std::min(amount, std::min(labels.blown[i], *cover[Sand]));
    const float mixed = *cover[Sand] - std::min(labels.blown[i], *cover[Sand]), fromMixed = amount - fromBlown;
    labels.blown[i] = std::max(0.0f, labels.blown[i] - fromBlown);
    *cover[Sand] -= amount;
    bookWind(domain, i, -amount);
    if (fromMixed <= 0 || mixed <= Trace) return 0;
    const float share = fromMixed / mixed;
    labels.lag[i] = std::min(*cover[Coarse], labels.lag[i] + share * std::max(0.0f, *cover[Coarse] - labels.lag[i]));
    // Fines that lay between the grains go up as dust; loess and salt at the top are a crust of their own.
    const float crust = std::min(*cover[Fines], labels.loess[i] + labels.salt[i]);
    const float dust = share * (*cover[Fines] - crust);
    *cover[Fines] -= dust;
    bookWind(domain, i, -dust);
    return dust;
}

void layBlown(Domain& domain, size_t i, float amount) {
    domain.ground.sediment[Sand][i] += amount;
    domain.aeolian.labels.blown[i] += amount;
    bookWind(domain, i, amount);
}

void bookWind(Domain& domain, size_t i, float metres) {
    (metres >= 0 ? domain.deposited : domain.eroded)[i] += std::abs(metres);
    if (!domain.owned(int(i % size_t(domain.size)), int(i / size_t(domain.size)))) return;
    (metres >= 0 ? domain.ledger.deposited : domain.ledger.eroded) += double(std::abs(metres)) * domain.cell * domain.cell;
}

void tidyLabels(Domain& domain) {
    WindLabels& labels = domain.aeolian.labels;
    for (size_t i = 0; i < domain.cells(); ++i) {
        labels.blown[i] = std::min(labels.blown[i], std::max(0.0f, domain.ground.sediment[Sand][i]));
        labels.lag[i] = std::min(labels.lag[i], std::max(0.0f, domain.ground.sediment[Coarse][i]));
        const float fines = std::max(0.0f, domain.ground.sediment[Fines][i]);
        labels.salt[i] = std::min(labels.salt[i], fines);
        labels.loess[i] = std::min(labels.loess[i], fines - labels.salt[i]);
        // Water that reworks a bed turns its stones over: an armour lasts only where none cuts or lays.
        if (domain.erosionRate[i] * float(domain.step) > LagArmour || domain.depositionRate[i] * float(domain.step) > LagArmour) labels.lag[i] = 0;
    }
}

} // namespace derivean::sim

#include "atmosphere/type/Atmosphere.hpp"

#include <vector>

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double LeastFall = 1e-6;   // kg/m² below which a place counts as free of ash

// kg/m² a patch laid on a point.
double blanket(const AshPatch& patch, Vec2 position) {
    const double dx = wrapDelta(position.x - patch.centre.x, PlanetSpan), dz = wrapDelta(position.z - patch.centre.z, PlanetSpan);
    const double variance = patch.spread * patch.spread;
    return patch.mass * std::exp(-0.5 * (dx * dx + dz * dz) / variance) / (2 * Pi * variance);
}

// What one source id, over all its phases, has to do with a place.
struct Share {
    uint64_t id{};
    double laid{};          // kg/m² its blankets laid here
    double given{};         // kg of dust it has given off so far
    double since{1e300};    // calendar seconds of its first phase
};

} // namespace

AshFall ashFallAt(const Atmosphere& atmosphere, Vec2 position) {
    const Plume& plume = atmosphere.plume;
    const double x = position.x / AirCell - 0.5, z = position.z / AirCell - 0.5, now = double(atmosphere.step) * AirStep;
    std::vector<Share> shares;
    const auto of = [&](uint64_t id) -> Share& {
        for (Share& share : shares)
            if (share.id == id) return share;
        shares.push_back({id});
        return shares.back();
    };
    for (const AirSource& source : plume.sources) {
        Share& share = of(source.id);
        share.given += source.ash[Dust] * std::max(0.0, std::min(now, source.end) - source.start);
        share.since = std::min(share.since, source.start);
    }
    const double dust = sample(plume.dustFallen, x, z, Kernel::Linear, Edge::Wrap);
    AshFall fall;
    fall.mass = dust;
    for (const AshPatch& patch : plume.patches) {
        const double here = blanket(patch, position);
        of(patch.source).laid += here;
        fall.mass += here;
    }
    if (fall.mass < LeastFall) return {};

    // The place is named after the source whose blankets laid most on it; where the dust that
    // settled out of the grid outweighs every blanket, after the source that has given off most dust.
    const bool dusty = dust > fall.mass - dust;
    const Share* named = nullptr;
    for (const Share& share : shares) {
        const double weight = dusty ? share.given : share.laid;
        if (weight > 0 && (!named || weight > (dusty ? named->given : named->laid))) named = &share;
    }
    fall.rate = sample(plume.fallRate, x, z, Kernel::Linear, Edge::Wrap);
    if (named) fall.source = named->id, fall.since = named->since;
    return fall;
}

PlumeLedger plumeLedger(const Atmosphere& atmosphere) {
    PlumeLedger ledger = atmosphere.plume.totals;
    const auto sum = [](const Field<double>& field) {
        double all = 0;
        for (double value : field.cells) all += value;
        return all * AirCellArea;
    };
    for (size_t level = 0; level < PlumeLevels; ++level) {
        ledger.ashAloft += sum(atmosphere.plume.dust[level]);
        ledger.sulphurGas += sum(atmosphere.plume.gas[level]);
        ledger.sulphurVeil += sum(atmosphere.plume.veil[level]);
    }
    return ledger;
}

} // namespace derivean::sim

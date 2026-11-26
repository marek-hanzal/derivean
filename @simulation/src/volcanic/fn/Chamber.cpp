#include "volcanic/fn/Source.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t SequenceStream = 0x43484d42;
// Fluid melt leaves its chamber often and in small parts; viscous melt holds until the roof gives,
// more rarely and with a longer tail of great eruptions. Volumes are log-normal around the median
// with this spread: most eruptions are small, and a few of the largest carry much of the volume.
// These two numbers, with the planet's supply, are what sets how often a volcano erupts.
constexpr double MedianEruption[2] = {2.0e6, 2.0e6};   // m³, by Magma
constexpr double Spread[2] = {1.8, 2.2};

} // namespace

double meanEruption(Magma magma) {
    const double spread = Spread[size_t(magma)];
    return MedianEruption[size_t(magma)] * std::exp(0.5 * spread * spread);
}

double eruptionVolume(uint64_t seed, const Vent& vent, int64_t number) {
    const uint64_t base = stream(seed, SequenceStream) ^ vent.id;
    // A normal draw (Box and Muller) with its whole tail: the rare great eruption lives there.
    const double normal = std::sqrt(-2.0 * std::log(1.0 - unit(base, number, 1))) * std::cos(6.283185307179586 * unit(base, number, 2));
    return MedianEruption[size_t(vent.magma)] * std::exp(Spread[size_t(vent.magma)] * normal);
}

bool nextEruption(uint64_t seed, const Vent& vent, Eruption& next) {
    if (vent.supply <= 0) return false;
    next.volume = eruptionVolume(seed, vent, vent.count);
    next.time = vent.until + std::max(0.0, next.volume - vent.held) / vent.supply;
    return true;
}

void eruptionsAfter(uint64_t seed, const Vent& vent, double until, std::vector<Eruption>& into) {
    Eruption eruption;
    if (!nextEruption(seed, vent, eruption)) return;
    for (int64_t number = vent.count; eruption.time <= until; ++number) {
        into.push_back(eruption);
        eruption.volume = eruptionVolume(seed, vent, number + 1);
        eruption.time += eruption.volume / vent.supply;
    }
}

} // namespace derivean::sim

#include "volcanic/fn/Source.hpp"

#include "chronology/constant/Timeline.hpp"
#include "core/fn/Hash.hpp"

#include <array>

namespace derivean::sim {
namespace {

constexpr uint64_t SequenceStream = 0x43484d42;
// Fluid melt leaves its chamber often and in small parts; viscous melt holds until the roof gives,
// more rarely and with a longer tail of great eruptions. Volumes are log-normal around the median
// with this spread: most eruptions are small, and a few of the largest carry much of the volume.
// These two numbers, with the planet's supply, are what sets how often a volcano erupts.
constexpr double MedianEruption[2] = {5.0e5, 1.0e6};   // m³, by Magma
constexpr double Spread[2] = {2.0, 2.2};
// The sequence is cut into runs of this many eruptions whose volumes sum to exactly what the mean
// says, so the eruptions around any supplied volume are found without counting from the start.
constexpr int Run = 256;

double runVolume(Magma magma) { return Run * meanEruption(magma); }

// Volumes of one run as shares of it.
std::array<double, Run> runShares(uint64_t seed, const Vent& vent, int64_t run) {
    const uint64_t base = stream(seed, SequenceStream) ^ vent.id;
    std::array<double, Run> shares;
    double total = 0;
    for (int i = 0; i < Run; ++i) {
        // A normal draw (Box and Muller) with its whole tail: the rare great eruption lives there.
        const double normal = std::sqrt(-2.0 * std::log(1.0 - unit(base, run, i, 1))) * std::cos(6.283185307179586 * unit(base, run, i, 2));
        shares[size_t(i)] = std::exp(Spread[size_t(vent.magma)] * normal);
        total += shares[size_t(i)];
    }
    for (double& share : shares) share /= total;
    return shares;
}

} // namespace

double meanEruption(Magma magma) {
    const double spread = Spread[size_t(magma)];
    return MedianEruption[size_t(magma)] * std::exp(0.5 * spread * spread);
}

double suppliedBy(const Vent& vent, double time) {
    double supplied = 0;
    for (const Episode& episode : vent.episodes) {
        if (time <= episode.time) return supplied;
        supplied += episode.volume * std::min(1.0, (time - episode.time) / RegionStep);
    }
    return supplied + vent.supply * std::max(0.0, time - vent.until);
}

double dateOf(const Vent& vent, double volume) {
    double supplied = 0;
    for (const Episode& episode : vent.episodes) {
        if (supplied + episode.volume >= volume) return episode.time + RegionStep * (volume - supplied) / episode.volume;
        supplied += episode.volume;
    }
    return vent.supply > 0 ? vent.until + (volume - supplied) / vent.supply : 1e30;
}

void eruptionsBetween(uint64_t seed, const Vent& vent, double from, double to, std::vector<Eruption>& into) {
    const double whole = runVolume(vent.magma);
    for (int64_t run = int64_t(std::floor(from / whole)); double(run) * whole < to; ++run) {
        const std::array<double, Run> shares = runShares(seed, vent, run);
        double reached = 0;
        for (int i = 0; i < Run; ++i) {
            reached += shares[size_t(i)];
            // The last of a run ends on the run's own boundary, free of rounding.
            const double supplied = i + 1 == Run ? double(run + 1) * whole : (double(run) + reached) * whole;
            if (supplied > from && supplied <= to) into.push_back({supplied, shares[size_t(i)] * whole});
        }
    }
}

double largestEruption(uint64_t seed, const Vent& vent, double from, double to) {
    std::vector<Eruption> found;
    eruptionsBetween(seed, vent, from, to, found);
    double largest = 0;
    for (const Eruption& eruption : found) largest = std::max(largest, eruption.volume);
    return largest;
}

} // namespace derivean::sim

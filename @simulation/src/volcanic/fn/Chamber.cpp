#include "volcanic/fn/Source.hpp"

#include "core/fn/Hash.hpp"
#include "volcanic/constant/Rheology.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr uint64_t SequenceStream = 0x43484d42;
// A chamber fails when its overpressure reaches what the roof holds, and an eruption lets out
// what brings the overpressure back down: the chamber's volume times that overpressure times its
// compressibility, wall and magma (Huppert and Woods 2002). Gas-poor basalt is nearly as stiff as
// the wall (2e-10 /Pa); andesite carrying exsolved gas is several times softer (1e-9 /Pa). With
// 5 MPa of overpressure an eruption lets out 0.1 % and 0.5 % of the chamber in the median.
// Volumes are log-normal around that median with this spread: most eruptions are small, and a
// few of the largest carry much of the volume.
constexpr double Overpressure = 5e6;                   // Pa
constexpr double Compressibility[2] = {2e-10, 1e-9};   // /Pa, by Magma
constexpr double Spread[2] = {1.8, 2.2};
// The chamber stores the eruptible magma of this many years of the centre's supply (residence of
// eruptible melt, 1e3 to 1e5 yr in crystal and U-series ages). A centre supplied at the typical
// 0.06 km³ a thousand years holds 0.6 km³.
constexpr double Residence = 1e4;   // yr
constexpr double TypicalChamber = 6e8;   // m³, for a vent that does not say

// A roof goes down as one block once the eruption has taken this share of what the chamber held
// (0.1 to 0.4 in analogue and numerical models of collapse, Roche and Druitt 2001, Geyer et al.
// 2006), and only over a chamber large enough that the block is wide against its depth.
constexpr double CollapseShare = 0.3;

double chamberVolume(const Vent& vent) { return vent.chamber > 0 ? vent.chamber : TypicalChamber; }
double medianOf(const Vent& vent) { return chamberVolume(vent) * Overpressure * Compressibility[size_t(vent.magma)]; }
double normalBelow(double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); }

} // namespace

double chamberOf(double supply) { return supply * Residence; }

// The mean of the log-normal cut at the chamber: E[min(V, C)].
double meanEruption(const Vent& vent) {
    const double spread = Spread[size_t(vent.magma)], median = medianOf(vent), chamber = chamberVolume(vent);
    const double cut = std::log(chamber / median) / spread;
    return median * std::exp(0.5 * spread * spread) * normalBelow(cut - spread) + chamber * (1.0 - normalBelow(cut));
}

bool emptiesChamber(const Vent& vent, double volume) {
    return volume >= std::max(CollapseShare * chamberVolume(vent), CalderaVolume[size_t(vent.magma)]);
}

double eruptionVolume(uint64_t seed, const Vent& vent, int64_t number) {
    const uint64_t base = stream(seed, SequenceStream) ^ vent.id;
    // A normal draw (Box and Muller) with its whole tail: the rare great eruption lives there.
    const double normal = std::sqrt(-2.0 * std::log(1.0 - unit(base, number, 1))) * std::cos(6.283185307179586 * unit(base, number, 2));
    // No eruption brings up more than the chamber holds.
    return std::min(chamberVolume(vent), medianOf(vent) * std::exp(Spread[size_t(vent.magma)] * normal));
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

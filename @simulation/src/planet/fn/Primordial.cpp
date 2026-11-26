#include "planet/type/Planet.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t PlumeStream = 0x504c554d;

// Long-lived hot upwellings of the early mantle; positive over a plume.
double plume(uint64_t seed, Vec2 position) {
    const double scale = 4.0 / PlanetSpan;
    return fractalNoise(stream(seed, PlumeStream), position.x * scale, position.z * scale, 4, 0.55, 4);
}

} // namespace

double quenchTime(uint64_t seed, Vec2 position) {
    // A lid lasts first over downwellings; over plumes lava keeps resurfacing for tens of Myr.
    return MagmaOceanEnd + 24 * Myr * smoothstep(0.08, 0.62, plume(seed, position));
}

CrustColumn primordialColumn(uint64_t seed, Vec2 position, double time) {
    CrustColumn column;
    const double heat = plume(seed, position);
    column.oceanAge = float(std::max(0.0, time - quenchTime(seed, position)));
    column.quietSince = float(time - column.oceanAge);
    // Repeated melting above a plume distils buoyant felsic rock: the seeds of the first continents.
    const double nucleus = smoothstep(0.30, 0.66, heat) * smoothstep(20 * Myr, TectonicsStart, time);
    column.crust = float(27000.0 * nucleus);
    column.basement = nucleus > 0.3 ? Basement::Craton : Basement::Primordial;
    column.fabric = float(6.283185307179586 * unit(hash(seed, int64_t(position.x / PlanetCell), int64_t(position.z / PlanetCell), 77)));
    return column;
}

} // namespace derivean::sim

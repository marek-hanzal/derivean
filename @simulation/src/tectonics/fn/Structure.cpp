#include "tectonics/fn/Structure.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t StructureStream = 0x53545243;

} // namespace

Structure structureAt(uint64_t seed, Vec2 material, double strike) {
    const uint64_t base = stream(seed, StructureStream);
    const double c = std::cos(strike), s = std::sin(strike);
    // Kilometres along and across the regional strike, bent so ranges are not ruler-straight.
    const double bendX = 26.0 * fractalNoise(base + 1, material.x / 190e3, material.z / 190e3, 2);
    const double bendZ = 26.0 * fractalNoise(base + 2, material.x / 190e3, material.z / 190e3, 2);
    const double x = material.x / 1000.0 + bendX, z = material.z / 1000.0 + bendZ;
    const double along = x * c + z * s, across = -x * s + z * c;

    Structure structure;
    // Thrust ranges: long along strike, tens of km across, with saddles where the ridge noise dips.
    const double ridge = ridgedNoise(base + 3, along / 150.0, across / 34.0, 3, 0.55);
    structure.ranges = std::max(-1.0, 3.1 * (ridge - 0.52));

    // Grabens: narrow troughs between broad shoulders.
    const double trough = 1.0 - std::abs(fractalNoise(base + 4, along / 210.0, across / 48.0, 2));
    structure.rifts = std::max(-1.0, 4.0 * (trough * trough * trough - 0.42));

    // Tilted blocks: a sawtooth across strike whose phase wanders, so scarps start, bend and die out.
    const double phase = across / 62.0 + 1.9 * fractalNoise(base + 5, along / 120.0, across / 120.0, 2);
    const double tooth = phase - std::floor(phase);
    const double saw = tooth < 0.86 ? tooth / 0.86 : (1.0 - tooth) / 0.14;
    const double vigor = smoothstep(-0.15, 0.35, fractalNoise(base + 6, x / 170.0, z / 170.0, 2));
    structure.blocks = (2.0 * saw - 1.0) * vigor;

    // Fault and joint zones: thin lines where two cells of a jittered lattice meet.
    // Their trend wanders around the regional strike and only some of them are open enough to matter.
    const double turn = 0.9 * fractalNoise(base + 8, x / 140.0, z / 140.0, 2);
    const double ct = std::cos(turn), st = std::sin(turn);
    const CellularSite site = cellularNoise(base + 7, (along * ct + across * st) / 21.0, (-along * st + across * ct) / 14.0, 1.0);
    const double open = smoothstep(0.35, 0.75, unit(hash(base + 9, site.cellX + site.otherX, site.cellZ + site.otherZ)));
    structure.weakness = open * (1.0 - smoothstep(0.0, 0.10, site.second - site.nearest));
    return structure;
}

} // namespace derivean::sim

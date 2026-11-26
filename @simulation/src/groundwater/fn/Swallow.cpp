#include "groundwater/fn/Swallow.hpp"

#include "groundwater/fn/Conductivity.hpp"

namespace derivean::sim {
namespace {

constexpr float SolutionGain = 30.0f;    // how much wider solution has opened the joints of soluble rock at the surface
constexpr float PlugDepth = 1.5f;        // m of loose cover that seals all but 1/e of the openings under it
constexpr float StreamWidth = 3.5f;      // m of bed a stream of 1 m³/s wets; width grows with the root of the flow

} // namespace

float swallowRate(const SwallowBed& bed) {
    const MaterialDefinition& rock = definitionOf(bed.rock);
    if (!solubleRock(rock)) return 0;
    const float joints = jointConductivity(rock, 0, 1) * SolutionGain * PerYear;
    const float opened = std::clamp(bed.openDepth / 10.0f, 0.5f, 2.0f);
    return joints * (1.0f + 6.0f * bed.fracture) * opened * (1.0f + std::sin(std::max(0.0f, bed.dip))) * std::exp(-std::max(0.0f, bed.cover) / PlugDepth);
}

float swallowed(float rate, float discharge, float cell) {
    if (rate <= 0 || discharge <= 0) return 0;
    const float width = std::min(cell, StreamWidth * std::sqrt(discharge / PerYear));
    return std::min(discharge, rate * width * cell);
}

} // namespace derivean::sim

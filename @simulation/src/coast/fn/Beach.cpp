#include "coast/fn/Coast.hpp"

#include "groundwater/fn/Conductivity.hpp"
#include "hydrology/constant/Water.hpp"

#include <numbers>

namespace derivean::sim {
namespace {

// Beach face of a grain under a breaking train (Sunamura 1984): tanβ = 0.12·(H/(√(gD)·T))^−½. With
// the period of a wind sea, T = √(2πH/(g·s)), this is 0.12·(H·s/(2πD))^−¼: flatter under higher
// waves and on finer grains. Medium sand is 0.06–0.12 under 1–0.05 m waves, gravel 0.13–0.2.
constexpr float FaceGain = 0.12f;
constexpr float SteepestFace = 0.6f;    // rise over run loose ground stands at in still water
// Mud forms no beach face: it holds a flat, a little steeper where the water over it is shallow.
constexpr float MudSlope = 0.004f;
constexpr float MudDepth = 0.5f;        // m
constexpr float Kinematic = 1.0e-6f;    // m²/s, water
constexpr float Submerged = 1.65f;      // density of quartz over water's, less one

// m/s a grain settles at in still water (Ferguson and Church 2004).
float fallOf(float grain) {
    const float g = float(Gravity);
    return Submerged * g * grain * grain / (18.0f * Kinematic + std::sqrt(0.75f * Submerged * g * grain * grain * grain));
}

// Below the step the bed follows the equilibrium shoreface h = A·x^⅔, A = 0.067·w^0.44 (w in cm/s,
// Dean 1991), whose slope at depth h is ⅔·A^1.5/√h.
float deanOf(int kind) {
    const float a = 0.067f * std::pow(100.0f * fallOf(definitionOf(CoverRock[kind]).grainSize), 0.44f);
    return 2.0f / 3.0f * a * std::sqrt(a);
}

} // namespace

float faceSlope(int kind, float height) {
    if (kind == Fines) return MudSlope;
    const float grain = definitionOf(CoverRock[kind]).grainSize;
    if (height <= 1e-3f) return SteepestFace;
    return std::min(SteepestFace, FaceGain * std::pow(height * WindSeaSteepness / (2.0f * std::numbers::pi_v<float> * grain), -0.25f));
}

float profileSlope(int kind, float depth, float height, float face) {
    if (kind == Fines) return MudSlope / std::sqrt(1.0f + std::max(0.0f, depth) / MudDepth);
    // The face runs down to the step at the foot of the swash, about one wave height deep.
    if (depth <= height) return face;
    static const float dean[SedimentClasses] = {deanOf(Coarse), deanOf(Sand), 0.0f};
    return std::min(face, dean[kind] / std::sqrt(depth));
}

float runUp(float height, float slope) {
    // Stockdon et al. 2006, 2 % exceedance: R = 1.1·(0.35·β·√(H·L0) + ½·√(H·L0·(0.563·β² + 0.004))),
    // with L0 = H/s for a wind sea.
    const float reach = height / std::sqrt(WindSeaSteepness);
    return std::min(HighestBerm, 1.1f * (0.35f * slope * reach + 0.5f * reach * std::sqrt(0.563f * slope * slope + 0.004f)));
}

} // namespace derivean::sim

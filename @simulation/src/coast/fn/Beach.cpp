#include "coast/fn/Coast.hpp"

#include "groundwater/fn/Conductivity.hpp"
#include "hydrology/constant/Water.hpp"

#include <numbers>

namespace derivean::sim {
namespace {

// Beach face of a grain under a train breaking at height H (Sunamura 1984): tanβ = 0.12·(H/(√(gD)·T))^−½.
// With the period of a wind sea, T = √(2πH/(g·s)), this is 0.12·(H·s/(2πD))^−¼: flatter under higher
// waves and on finer grains. Medium sand is 0.064–0.134 under 1–0.05 m waves, gravel 0.16–0.34.
// Below a few centimetres of wave the law runs off to the steepest faces beaches are seen with.
constexpr float FaceGain = 0.12f;
constexpr float SteepestFace[SedimentClasses] = {0.4f, 0.2f, 0.0f};   // Fines: unused, mud has no face
// Depth of the step at the foot of the face, over the breaker height. No source: a calibration that
// keeps the steep face inside the swash at every level (a whole 64 m Local surf zone at the face
// would drown a spit); storm beaches show steps of decimetres under metre waves.
constexpr float StepDepth = 0.33f;
// Mud forms no beach face: it holds a flat, a little steeper where the water over it is shallow.
constexpr float MudSlope = 0.004f;
constexpr float MudDepth = 0.5f;        // m
constexpr float Kinematic = 1.0e-6f;    // m²/s, water
constexpr float Submerged = 1.65f;      // density of quartz over water's, less one

// m/s a natural grain settles at in still water (Ferguson and Church 2004, C1 = 18, C2 = 1).
float fallOf(float grain) {
    const float g = float(Gravity);
    return Submerged * g * grain * grain / (18.0f * Kinematic + std::sqrt(0.75f * Submerged * g * grain * grain * grain));
}

// Below the step the bed follows the equilibrium shoreface h = A·x^⅔, A = 0.067·w^0.44 (w in cm/s,
// Dean 1991), whose slope at depth h is ⅔·A^1.5/√h. Dean fitted sand; for gravel it is an extrapolation.
float shapeOf(int kind) { return 0.067f * std::pow(100.0f * fallOf(definitionOf(CoverRock[kind]).grainSize), 0.44f); }
float deanOf(int kind) {
    const float a = shapeOf(kind);
    return 2.0f / 3.0f * a * std::sqrt(a);
}

} // namespace

float faceSlope(int kind, float height) {
    if (kind == Fines) return MudSlope;
    const float grain = definitionOf(CoverRock[kind]).grainSize;
    if (height <= 0) return SteepestFace[kind];
    return std::min(SteepestFace[kind], FaceGain * std::pow(height * WindSeaSteepness / (2.0f * std::numbers::pi_v<float> * grain), -0.25f));
}

float profileSlope(int kind, float depth, float height, float face) {
    if (kind == Fines) return MudSlope / std::sqrt(1.0f + std::max(0.0f, depth) / MudDepth);
    // The face runs down to the step at the foot of the swash, where the breakers plunge (StepDepth).
    // Below it lies the surf zone, as gentle as the equilibrium shoreface.
    if (depth <= StepDepth * height) return face;
    static const float dean[SedimentClasses] = {deanOf(Coarse), deanOf(Sand), 0.0f};
    return std::min(face, dean[kind] / std::sqrt(depth));
}

float shorefaceReach(int kind, float depth) {
    if (kind == Fines || depth <= 0) return 0.0f;
    const float reach = depth / shapeOf(kind);
    return reach * std::sqrt(reach);
}

float wavePeriod(float height) { return std::sqrt(2.0f * std::numbers::pi_v<float> * std::max(0.0f, height) / (float(Gravity) * WindSeaSteepness)); }

float runUp(float height, float slope) {
    // Stockdon et al. 2006, 2 % exceedance: R = 1.1·(0.35·β·√(H·L0) + ½·√(H·L0·(0.563·β² + 0.004))),
    // with L0 = H/s for a wind sea; H is the train's height before the depth broke it. Fitted on sand beaches.
    const float reach = height / std::sqrt(WindSeaSteepness);
    return std::min(HighestBerm, 1.1f * (0.35f * slope * reach + 0.5f * reach * std::sqrt(0.563f * slope * slope + 0.004f)));
}

} // namespace derivean::sim

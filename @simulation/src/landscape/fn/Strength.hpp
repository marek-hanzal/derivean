#pragma once

#include "material/constant/Materials.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace derivean::sim {

// How high a rock mass stands. A face fails along a plane through its foot once its weight on that
// plane exceeds the plane's cohesion and friction (Culmann): a face of height H at angle β holds
// while H ≤ (4c/γ)·sin β·cos φ / (1 − cos(β − φ)). The cohesion of the mass is not that of the
// intact rock: it is `massCohesion` for a face 100 m high and falls with the cube root of the
// height, because a larger face finds longer joints to fail along. Together these say that sound
// granite stands vertical for a few hundred metres, at 60° for about a kilometre, and at little
// more than its friction angle over the height of a mountainside.
//
// The curve is stored as the greatest drop a face of sound rock holds over a horizontal run, at
// runs of 2^(k / StrengthPerOctave) metres. Rock whose cohesion is `factor` times the material's
// has the same curve scaled by factor^(3/4) along both axes.
inline constexpr int StrengthPerOctave = 4;
inline constexpr int StrengthSamples = 22 * StrengthPerOctave + 1;   // runs from 1 m to about 4 000 km

struct StrengthCurve {
    std::array<float, StrengthSamples> drop{};   // m
    float frictionTan{};
};

const StrengthCurve& strengthOf(Material rock);

// m a face may fall over `run` metres. `scale` is factor^(3/4) and `logScale` its base-2 logarithm.
inline float stableDrop(const StrengthCurve& curve, float logRun, float scale, float logScale) {
    const float at = std::clamp((logRun - logScale) * StrengthPerOctave, 0.0f, float(StrengthSamples - 1) - 1e-3f);
    const int below = int(at);
    return scale * (curve.drop[size_t(below)] + (curve.drop[size_t(below) + 1] - curve.drop[size_t(below)]) * (at - float(below)));
}

} // namespace derivean::sim

#include "landscape/fn/Strength.hpp"

#include <algorithm>
#include <vector>

namespace derivean::sim {
namespace {

constexpr double Gravity = 9.81;
constexpr double ScaleHeight = 100.0;   // m, face height `massCohesion` is given for
constexpr int Angles = 512;

StrengthCurve curveOf(Material rock) {
    const MaterialDefinition& d = definitionOf(rock);
    StrengthCurve curve;
    const double friction = std::clamp(double(d.massFriction), 1.0, 60.0) * 0.017453292519943295;
    curve.frictionTan = float(std::tan(friction));
    const double weight = std::max(500.0, double(d.density)) * Gravity;   // N/m³
    // H^(4/3) = reach · sin β / (1 − cos(β − φ)) once the cube-root fall of cohesion is put in.
    const double reach = 4.0 * d.massCohesion * 1000.0 / weight * std::cbrt(ScaleHeight) * std::cos(friction);
    // The curve as (run, drop) pairs from a vertical face down to one barely steeper than friction.
    std::vector<double> runs, drops;
    for (int k = 0; k < Angles; ++k) {
        const double beta = 1.5707963267948966 - (1.5707963267948966 - friction) * (double(k) / Angles);
        const double drop = reach > 0 ? std::pow(reach * std::sin(beta) / (1.0 - std::cos(beta - friction)), 0.75) : 0.0;
        runs.push_back(k == 0 ? 0.0 : drop / std::tan(beta));
        drops.push_back(drop);
    }
    size_t at = 0;
    for (int sample = 0; sample < StrengthSamples; ++sample) {
        const double run = std::pow(2.0, double(sample) / StrengthPerOctave);
        while (at + 1 < runs.size() && runs[at + 1] < run) ++at;
        // Past the last pair cohesion no longer matters: the slope is the friction angle.
        if (at + 1 >= runs.size()) curve.drop[size_t(sample)] = float(std::max(drops.back(), 0.0) + (run - runs.back()) * std::tan(friction));
        else curve.drop[size_t(sample)] = float(drops[at] + (drops[at + 1] - drops[at]) * (run - runs[at]) / std::max(1e-9, runs[at + 1] - runs[at]));
        curve.drop[size_t(sample)] = std::max(curve.drop[size_t(sample)], float(run * std::tan(friction)));
    }
    return curve;
}

} // namespace

const StrengthCurve& strengthOf(Material rock) {
    static const auto table = [] {
        std::array<StrengthCurve, size_t(MaterialCount)> entries;
        for (int i = 0; i < MaterialCount; ++i) entries[size_t(i)] = curveOf(Material(i));
        return entries;
    }();
    return table[size_t(rock)];
}

} // namespace derivean::sim

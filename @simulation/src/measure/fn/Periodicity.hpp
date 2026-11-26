#pragma once

#include <vector>

namespace derivean::sim {

// How regularly a profile sampled at equal steps repeats. The profile is first freed of everything
// longer than a quarter of its length, so at least four repeats must fit to be seen.
struct Periodicity {
    double strength{};    // 0..1, highest autocorrelation at any lag past the first one where it has fallen below zero
    int lag{};            // samples at that lag; 0 when the autocorrelation never falls below zero
    int marks{};          // crests or troughs that stand out by a fifth of the profile's range
    double spacing{};     // samples, mean distance between neighbouring marks
    double spacingCv{};   // its coefficient of variation: 1 for marks scattered at random, 0 for a ruler
    double height{};      // mean prominence of the marks, in the units of the profile
    double heightCv{};
};

enum class Mark { Crests, Troughs };

Periodicity periodicityOf(const std::vector<float>& profile, Mark mark);

// Share of the mean squared height difference over eight samples that neighbouring samples already
// have: 8^(-2H) for a profile of roughness exponent H. A smooth slope reads near 0.02, relief that
// is made cell by cell reads towards 1.
double cellRoughness(const std::vector<float>& profile);

// Samples that are the lowest of their three and lie at least `depth` below the highest sample
// within `reach` on both sides: the floors of cuts no wider than the grid can draw them.
int notchesIn(const std::vector<float>& profile, double depth, int reach = 2);

} // namespace derivean::sim

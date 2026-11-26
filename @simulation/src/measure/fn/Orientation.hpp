#pragma once

#include "measure/type/Mosaic.hpp"

#include <array>
#include <vector>

namespace derivean::sim {

inline constexpr int RoseBins = 36;   // 5° each over the half circle: a valley has an axis, not a heading

// Share of a segment sample within GridBand of a grid axis or diagonal, over the third an
// isotropic sample puts there. A network that follows the grid reads up to 3.
inline constexpr double GridBand = 7.5;   // degrees

// Above this the valleys follow the grid. The share of an isotropic sample of 200 independent
// segments scatters by 0.033 around one third, so 1.3 lies three standard deviations out; a
// network locked to eight directions reads above 2.
inline constexpr double GridExcessLimit = 1.3;

struct Rose {
    std::array<double, RoseBins> share{};
    int segments{};
    double gridExcess{};       // 1 for an isotropic network
    double axisExcess{};       // the same for the two axes alone (rows and columns)
    double diagonalExcess{};   // and for the two diagonals
};

// How many standard deviations an excess lies above what an isotropic network of as many
// independent segments would show. Segments that start on neighbouring cells of one path share
// most of their chord, so only one in `chord` counts. `bandShare` is the isotropic share of the
// band the excess was taken on: a third for both pairs of grid directions, a sixth for one pair.
double excessSigma(double excess, int segments, int chord, double bandShare);

// Angles in degrees; only their value modulo 180 counts.
Rose roseOf(const std::vector<float>& angles);

// Azimuths of valley segments: from every dry cell that carries at least `river`, the straight
// line to the cell `chord` steps down its flow path, where that line is at least half as long. A chord of 32 cells resolves 1.8°, fine
// enough that the steps of the grid do not crowd segments into the bands around its directions.
std::vector<float> valleyAzimuths(const Mosaic& mosaic, double river, int chord = 32);

} // namespace derivean::sim

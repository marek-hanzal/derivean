#pragma once

#include "atmosphere/type/SphereDamping.hpp"
#include "atmosphere/type/StencilMatrix.hpp"
#include "geometry/type/SphereGrid.hpp"

#include <vector>

namespace derivean::sim {

// The damping with the flow's own stiffness (SpherePoisson::levels[0].stiffness, the K the flow
// inverts with; its meaning as the elliptic operator is unchanged), the fastest mode decaying in
// `decay` seconds.
SphereDamping makeSphereDamping(const SphereGrid& grid, const StencilMatrix& stiffness, double decay);

// Adds −ν A⁻¹ K A⁻¹ K `level` to `tendency` (1/s², sized to the cells; empty is taken as zeros).
// `level` is the one the step starts from (baseLevel in fn/SphereFlow), so in the leapfrog it acts
// on the older level, as today's flat step does.
void addDamping(SphereDamping& damping, const SphereGrid& grid, const StencilMatrix& stiffness, const std::vector<double>& level,
                std::vector<double>& tendency);

// ν · bound² (1/s): a step whose span s has s · ν · bound² ≤ 1 keeps every mode's sign and damps it
// monotonically before the filter (SphereHour::stiffest).
double dampingStiffness(const SphereDamping& damping);

} // namespace derivean::sim

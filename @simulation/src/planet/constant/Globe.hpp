#pragma once

namespace derivean::sim {

// The planet's history on the cubed sphere (S3b) runs at one level of the geometry grid; the level is
// a parameter of makeGlobe. Level 7 is 6 × 128² cells of about 72 km, close to the flat 65.5 km cell,
// so constants tuned per cell keep their meaning (lead's decision L1, S3-design.md §9).
inline constexpr int GlobeLevel = 7;

// m of basalt in new ocean floor: the oceanic crust a spreading contact makes from the interior and a
// trench returns to it. Earth's normal oceanic crust is 7.1 ± 0.8 km (White, McKenzie & O'Nions 1992).
inline constexpr double OceanicCrust = 7000.0;

} // namespace derivean::sim

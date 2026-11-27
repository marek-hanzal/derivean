#pragma once

#include <vector>

namespace derivean::sim {

// Bands of latitude of equal angular width over a sphere grid: the sphere's counterpart of the flat
// world's rows, for zonal means and the closures that read them (Codex DREV-CLAUDE-007-01 point 2).
// Band b is centred at −π/2 + (b + ½) · width. A cell shares itself between the two bands whose
// centres bracket its latitude, linearly in latitude; cells poleward of the outermost centres belong
// to the outermost band. The weights w_ib (1 − share on `first`, share on first + 1) sum to one per
// cell, so with B_b = Σ_i A_i w_ib the band mean Σ_i A_i w_ib f_i / B_b and the spread back to the
// cells through the same w_ib keep the area integral. `centre` is the band's nominal latitude; the
// polar band's weights reach past it to the pole, so it is not their centroid there.
//
// The count to use is the grid's side (two cells along a meridian; 2.8° at level 6, the flat
// world's row); why, and how much a wave that is not zonal leaks into the means:
// docs/handoff/atmosphere-sphere.md (bands of latitude).
struct LatitudeBands {
    int count{};
    double width{};               // rad
    std::vector<double> centre;   // rad, per band
    std::vector<double> area;     // m², B_b
    // Per boundary between bands (count + 1, from the south pole to the north), the sine of the
    // latitude circle that cuts off the bands' own area south of it, Σ_{k<b} B_k, and the circle's
    // length (m, √(1 − sine²) · 2πR, exactly zero at the poles). A zonal wind's circulation round it
    // is rim · u. Taking the circles from B_b rather than from the bands' nominal edges keeps Stokes
    // consistent with the weights, which at the poles differ from the edges at first order.
    std::vector<double> boundary;
    std::vector<double> rim;
    std::vector<int> first;       // per cell, the band south of its latitude, 0 to count − 2
    std::vector<double> share;    // per cell, its weight on band first + 1, in [0, 1]
};

} // namespace derivean::sim

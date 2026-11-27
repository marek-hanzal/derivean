#pragma once

#include "ground/fn/SiteView.hpp"
#include "ground/type/Detail.hpp"
#include "ground/type/SpringGround.hpp"
#include "groundwater/type/Spring.hpp"

namespace derivean::sim {

// Grows the deposit of one spring on `around.ground`, epoch by epoch: the water is routed over the
// surface as it stands, pools filled to their rims, and mineral laid where the water thins and
// speeds up, which is the rim of every pool and the lip of every drop. A rim that grows dams its
// pool higher, so terraces step down the slope; water that leaves as steam builds a cone around
// the vent instead. The mineral laid never exceeds what the water brought in the time.
void growSpringDeposit(const Spring& spring, SpringGround& around);

// A spring that brings neither silica nor carbonate and is no hotter than the ground it alters
// grows no crust and alters nothing: growSpringDeposit leaves only its pools, which nothing reads.
bool laysNothing(const Spring& spring, double cell);

// Springs.cpp — what the given springs have built, read onto a ground domain. Each deposit is grown
// on the site surface around its vent, which every ground tile reads alike, so the tiles a deposit
// spans agree on it. Fills the crust of `detail` over the tile's own cells and `altered` (0..1 per
// domain cell), and adds each vent's water to the domain's inflow.
void laySpringDeposits(Domain& domain, const std::vector<Spring>& springs, const SiteView& sites, GroundDetail& detail, std::vector<float>& altered);

} // namespace derivean::sim

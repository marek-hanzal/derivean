#pragma once

#include "landscape/type/Setting.hpp"
#include "landscape/type/TrunkRouting.hpp"
#include "landscape/type/Trunks.hpp"

namespace derivean::sim {

// The water of the trunk field (routeTrunks) over its ground as it stands.

// m³/yr each planet cell yields by itself: what it passes on beyond what its neighbours send it.
std::vector<float> ownYield(const Backdrop& backdrop);
// Load per m³ of water of each planet cell's river.
std::vector<float> loadConcentration(const Backdrop& backdrop);
// The sea and its coast from the ground as it stands.
void markSea(const Trunks& out, TrunkRouting& routing, float sea);
// Each planet cell's own yield over its cells here (TrunkRouting::yield, ::drowned).
void shareYield(const Trunks& out, TrunkRouting& routing, const Planet& planet, const std::vector<float>& own);
// Priority flood from the coast; Trunks::flowTo, ::discharge and TrunkRouting::order.
void routeTrunkWater(Trunks& out, TrunkRouting& routing, float sea);

} // namespace derivean::sim

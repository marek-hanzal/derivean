#pragma once

#include "icesheet/fn/Wear.hpp"
#include "landscape/type/Setting.hpp"

namespace derivean::sim {

// The landscape's one way in is `sheetIce` (landscape/fn/Process.hpp). What follows is what its
// stages are made of at Region level, for tests on hand-built domains and for tools.

// The source the domains of a world read: its planet-wide stages at the place the landscape reads
// the planet for a position.
SheetSource worldSource(std::shared_ptr<const ContinentalIce> ice, uint64_t seed, double age);

// The sheets of a glacial cycle work a region domain: each stage with a share is relaxed over the
// ground as it is now, slides over it for its share of `years`, and its debris is carried and
// laid. With `leave`, this is the last cycle: the present and the last maximum are solved whatever
// their share and held to finished neighbours, the deposits are booked by kind in `work.left`,
// the streams under the last maximum's ice fill their tunnels and spread outwash beyond it, and
// `work.now` and `work.lastMaximum` receive the ice over the ground that results.
WearTotals workSheets(Domain& domain, SheetWork& work, const std::vector<float>& shares, double years, double span, bool leave, Workers* workers);

// The crust under the ice standing now has given way by what the sheet presses it down, and ground
// the last sheet left is still short of its rest by `remaining` of what that sheet added. Lowers
// rock and layering alike and returns, per cell, the metres still to come back.
std::vector<float> sinkCrust(Domain& domain, const SheetWork& work, double remaining);

} // namespace derivean::sim

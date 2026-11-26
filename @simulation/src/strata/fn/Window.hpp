#pragma once

#include "planet/fn/Sample.hpp"
#include "strata/type/Column.hpp"

namespace derivean::sim {

// Turns a column built from the planet's present cover into the one at the start of the region
// window, blended over the same four columns: what the planet removed in the window goes back on
// top and what it laid down comes off, because every replay erodes and deposits that window itself.
void restoreWindow(const Planet& planet, const ColumnBlend& around, StrataColumn& column);

} // namespace derivean::sim

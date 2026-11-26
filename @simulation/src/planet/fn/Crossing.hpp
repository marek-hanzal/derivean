#pragma once

#include "planet/type/Line.hpp"

namespace derivean::sim {

// What happens to crust at the two ends of a plate's stretch along a line when the plate moves one cell.
Parcel openGap(CrustColumn& departing, ColumnRecord& record, double time, bool stretch);
void crossContact(Line line, const Parcel& carry, int ahead, int lead, int direction, int length, int plate, PlateMotion& motion, float strike);

} // namespace derivean::sim

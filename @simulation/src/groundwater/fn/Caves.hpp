#pragma once

#include "groundwater/type/Cave.hpp"

namespace derivean::sim {

// Builds the point index of a field from its conduits: one index cell per local cell, each listing
// the conduits whose tube reaches into it. A field without conduits keeps no index.
void indexCaves(CaveField& field);

// Index of the conduit whose tube holds a point, the first listed where tubes overlap; -1 in rock.
int conduitAt(const CaveField& field, double x, double y, double z);

} // namespace derivean::sim

#pragma once

#include "planet/fn/Band.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Plan.hpp"

namespace derivean::sim {

// The pieces of one transfer plan, numbered locally in the order they are first met, with the plan's
// input read from them as they stand (planet/fn/Plan.hpp).
struct PlanPieces {
    PlanInput input;
    std::vector<int> global;   // local → index into globe.pieces
    std::vector<int> local;    // per piece of the globe, or -1
};

// The local number of a piece of the globe, taken into the plan's input on first use.
int localOf(PlanPieces& plan, const Globe& globe, int piece);

// Books a settled plan (S3-design.md §4): every transfer through the accounts it names, the donors'
// covers read as they stood before any transfer, the consumed pieces emptied into the interior and
// marked dead in the band. At a boundary (`boundary`) stacking brakes the donor's plate and marks the
// receiver an orogen; a seam closing inside one plate (planet/fn/Seam.hpp) does neither, since no
// plates collide there. A cancelled donor is left to the caller.
void applyPlan(Globe& globe, Band& band, const PlanPieces& pieces, const PlanResult& plan, const Substep& substep, bool boundary);

} // namespace derivean::sim

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
    int localOf(const Globe& globe, int piece);
};

// Books a settled plan (S3-design.md §4): every transfer through the accounts it names, the donors'
// covers read as they stood before any transfer, the consumed pieces emptied into the interior and
// marked dead in the band. A cancelled donor is left to the caller.
void applyPlan(Globe& globe, Band& band, const PlanPieces& pieces, const PlanResult& plan, const Substep& substep);

} // namespace derivean::sim

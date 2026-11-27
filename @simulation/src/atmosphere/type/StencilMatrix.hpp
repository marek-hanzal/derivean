#pragma once

#include <array>
#include <vector>

namespace derivean::sim {

// The stiffness K in two parts for a fast product. Rows inside a face (2 ≤ i, j < side − 2) all have
// the same 21 neighbours at the same index offsets (the 5 × 5 square without its corners), so they
// keep only their values: by blocks of up to 64 consecutive rows of a grid line, and within a block
// entry by entry, so a product reads them in one pass and sums several rows at once. The rows within
// two cells of a face edge keep compressed sparse rows. Both add the same products in the same column
// order as the assembled SphereMatrix, so the product is the same bit for bit.
struct StencilMatrix {
    int side{};                       // cells along a face edge
    int interior{};                   // rows in the structured part (0 when side < 5)
    std::array<int, 21> offset{};     // column − row of the interior entries, ascending
    std::vector<double> inner;        // 21 × interior, in the blocks above
    std::vector<int> seamRow, start, column;   // the other rows, start[k] .. start[k + 1] − 1 for seamRow[k]
    std::vector<double> value;
    std::vector<double> diagonal;
};

} // namespace derivean::sim

#pragma once

#include <array>
#include <vector>

namespace derivean::sim {

// The stiffness K in two parts for a fast product. Rows inside a face (2 ≤ i, j < side − 2) all have
// the same 21 neighbours at the same index offsets (the 5 × 5 square without its corners), so they
// keep only their values: by blocks of up to 64 consecutive rows of a grid line, and within a block
// entry by entry, so a product reads them in one pass and sums several rows at once. When the six
// faces' interiors are equal bit for bit (shareFaceInteriors makes them so), one face's values serve
// all six. The rows within two cells of a face edge keep compressed sparse rows, in row order. Both
// add the same products in the same column order as the assembled SphereMatrix, so the product is the
// same bit for bit. The V-cycle's float copy (`narrowed`) is the same matrix rounded to float32.
template <class Real> struct StencilMatrixOf {
    static constexpr int Margin = 2;    // interior rows have 2 ≤ i, j < side − 2
    static constexpr int Chunk = 64;    // rows of a grid line whose values are stored together
    static constexpr int Entries = 21;  // neighbours of an interior row
    int side{};                       // cells along a face edge
    int interior{};                   // rows in the structured part, all six faces (0 when side < 5)
    bool shared{};                    // `inner` holds one face's interior, used by all six
    std::array<int, 21> offset{};     // column − row of the interior entries, ascending
    std::vector<Real> inner;          // 21 × interior (÷ 6 when shared), in the blocks above
    std::vector<int> seamRow, start, column;   // the other rows, start[k] .. start[k + 1] − 1 for seamRow[k]
    std::vector<Real> value;
    std::vector<Real> diagonal;
};

using StencilMatrix = StencilMatrixOf<double>;

} // namespace derivean::sim

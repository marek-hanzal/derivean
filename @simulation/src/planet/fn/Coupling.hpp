#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// Fills the coupling of pieces and cells. A piece flagged in `exact` (or every piece when `exact` is
// empty and `allExact` holds) gets its true overlaps with the cells its envelope can reach (S3a's
// candidates and overlaps; the sum before normalisation is asserted against its area to 1e-9, Codex
// V3); the others get cloud-in-cell weights: the piece as a square of one cell in the face coordinates
// of its centre, overlapping up to four cells of that face, normalised to the piece's area. Inside a
// plate nothing is born or consumed, so normalised cheap weights conserve exactly and only the band
// needs true geometry (S3-design.md §4). The weights are Jones's (1999) first-order conservative coupling.
void couple(const SphereGrid& grid, const std::vector<CrustPiece>& pieces, const std::vector<uint8_t>& exact, bool allExact, Coupling& coupling);

// The exact overlaps of one piece, unnormalised, with their sum.
double exactWeights(const SphereGrid& grid, const CrustPiece& piece, std::vector<CellWeight>& weights, std::vector<int>& cells, std::vector<int>& work);
void cicWeights(const SphereGrid& grid, const CrustPiece& piece, std::vector<CellWeight>& weights);

// The piece with the largest weight in a cell (the lower key on a tie), -1 if none covers it.
int dominantPiece(const Coupling& coupling, int cell);

// A cell's view of an intensive field of its pieces: Σ w h / Σ w over the covered part, so a cell
// with a slip of coverage (a hole or an overlap of up to half a cell) shows its pieces' values.
template <class Value>
double viewOf(const Coupling& coupling, const std::vector<CrustPiece>& pieces, int cell, Value value, double fallback = 0) {
    double sum = 0, weight = 0;
    for (int32_t k = coupling.cellStart[size_t(cell)]; k < coupling.cellStart[size_t(cell) + 1]; ++k) {
        const PieceWeight& w = coupling.byCell[size_t(k)];
        sum += w.area * double(value(pieces[size_t(w.piece)]));
        weight += w.area;
    }
    return weight > 0 ? sum / weight : fallback;
}

} // namespace derivean::sim

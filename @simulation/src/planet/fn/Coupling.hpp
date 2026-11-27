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

// Books a change of felsic crust in a cell (m³, a gain or a loss) into its pieces by their weights over
// the covered part. A loss is limited by what each piece holds; returns what was booked, which is what
// the other side of the transaction must be credited with (S3-design.md §5, V3).
double bookCell(const Coupling& coupling, std::vector<CrustPiece>& pieces, int cell, double volume);

// Shares a change of a cell among the pieces over it by weight over the covered part: calls
// `act(piece, toPiece)` for each, where toPiece = A_c · w / Σw / a_k turns a thickness on the cell
// into the thickness that piece takes, so the volumes the pieces take sum to the cell's (Jones 1999).
template <class Act>
void forPiecesOver(const Coupling& coupling, std::vector<CrustPiece>& pieces, int cell, double cellArea, Act act) {
    const double covered = coupling.covered[size_t(cell)];
    if (covered <= 0) return;
    for (int32_t k = coupling.cellStart[size_t(cell)]; k < coupling.cellStart[size_t(cell) + 1]; ++k) {
        const PieceWeight& w = coupling.byCell[size_t(k)];
        CrustPiece& piece = pieces[size_t(w.piece)];
        act(piece, cellArea * (w.area / covered) / piece.area);
    }
}

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

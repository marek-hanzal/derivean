#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// The boundary band of one planet step (S3-design.md §4). The core is every cell that has pieces of
// two plates within its ring, read from the coupling at the step's start. Within the step a plate
// moves at most `sweep` rings, so births and consumptions can only happen in the core widened by
// sweep + 1 rings (`decide`), and only pieces listed within 2 · sweep + 3 rings of the core can reach
// a deciding cell (a ring more than the sweep and the footprint need, for the ring metric against the
// geodesic one): those are the band's pieces, the only ones coupled exactly in the substeps. Deciding
// is never done from cloud-in-cell weights. The weights list every piece in the cell of its centre, and
// `requireBandHeld` checks at the step's end that no piece outside the band reaches a deciding cell.
struct Band {
    int sweep{};
    std::vector<uint8_t> decide;          // per cell
    std::vector<int32_t> cells;           // the deciding cells, ascending
    std::vector<int32_t> region;          // per cell: index into regionCells, or -1
    std::vector<int32_t> regionCells;
    std::vector<int32_t> pieces;          // band pieces (indices into globe.pieces), ascending, births appended
    std::vector<uint8_t> member;          // per piece of the globe
    std::vector<uint8_t> dead;            // per piece of the globe: consumed this step
    // The exact coupling of the band's live pieces in the current substep.
    std::vector<int32_t> weightStart;     // per band piece + 1
    std::vector<CellWeight> weights;
    std::vector<int32_t> listStart;       // per region cell + 1
    std::vector<PieceWeight> lists;       // in piece order
};

// Marks the band from the globe's coupling at the start of a step.
void markBand(const Globe& globe, int sweep, Band& band);

// Exact overlaps of every live band piece as the pieces stand; lists per region cell.
void coupleBand(const Globe& globe, Band& band);

// Fatal when a piece outside the band reaches a deciding cell at the step's end: the band missed it.
void requireBandHeld(const Globe& globe, const Band& band);

// Adds a piece born in this substep to the band (its footprint is its cell).
void joinBand(const Globe& globe, Band& band, int piece);

// The band's pieces over a region cell, or nothing outside the region.
struct BandList {
    const PieceWeight* begin{};
    const PieceWeight* end{};
};
BandList listOf(const Band& band, int cell);

} // namespace derivean::sim

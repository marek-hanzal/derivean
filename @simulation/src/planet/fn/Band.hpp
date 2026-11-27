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
    // The pieces born in the current substep, with their exact overlaps per region cell, so every later
    // birth of the substep reads them (Codex DREV-CLAUDE-005-01); coupleBand folds them into the lists.
    std::vector<std::vector<PieceWeight>> born;   // per region cell
};

// Marks the band from the globe's coupling at the start of a step.
void markBand(const Globe& globe, int sweep, Band& band);

// Exact overlaps of every live band piece as the pieces stand; lists per region cell.
void coupleBand(const Globe& globe, Band& band);

// Fatal when a piece outside the band reaches a deciding cell at the step's end: the band missed it.
void requireBandHeld(const Globe& globe, const Band& band);

// Adds a piece born in this substep to the band with its exact overlaps.
void joinBand(const Globe& globe, Band& band, int piece);

// The band's pieces over a region cell, or nothing outside the region.
struct BandList {
    const PieceWeight* begin{};
    const PieceWeight* end{};
};
BandList listOf(const Band& band, int cell);

// Calls live(piece, area) for every live band piece over a region cell as the substep started
// (forListed), and those with this substep's births after them in the order they were born (forLive);
// nothing outside the region.
template <class Live> void forListed(const Band& band, int cell, Live live) {
    const int32_t r = band.region[size_t(cell)];
    if (r < 0) return;
    for (int32_t k = band.listStart[size_t(r)]; k < band.listStart[size_t(r) + 1]; ++k)
        if (!band.dead[size_t(band.lists[size_t(k)].piece)]) live(band.lists[size_t(k)].piece, band.lists[size_t(k)].area);
}
template <class Live> void forLive(const Band& band, int cell, Live live) {
    forListed(band, cell, live);
    const int32_t r = band.region[size_t(cell)];
    if (r < 0) return;
    for (const PieceWeight& w : band.born[size_t(r)]) live(w.piece, w.area);
}

} // namespace derivean::sim

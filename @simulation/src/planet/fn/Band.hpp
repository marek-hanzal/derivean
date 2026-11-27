#pragma once

#include "geometry/type/SpherePolygon.hpp"
#include "planet/type/Globe.hpp"

namespace derivean::sim {

class Workers;

// The boundary band of one planet step (S3-design.md §4). The core is every cell that has pieces of
// two plates within its ring, read from the coupling at the step's start. Within the step a plate
// moves at most `sweep` rings, so consumptions happen in the core widened by sweep + 1 rings (`decide`)
// and births over a deciding cell or a ring further (a slot of a lattice beside one), and only pieces
// listed within 2 · sweep + 3 rings of the core can reach those: the band's pieces, the only ones
// coupled exactly in the substeps. A piece outside them starts at least sweep + 2 rings from a deciding
// cell and so ends a substep at least 2 rings from one. Deciding
// is never done from cloud-in-cell weights. The weights list every piece in the cell of its centre, and
// `requireBandHeld` checks along every substep's turn that no piece outside the band reaches a deciding
// cell.
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
    // The same overlaps per birth, in birth order: band piece weightStart.size() − 1 + i is birth i.
    std::vector<int32_t> bornStart{0};
    std::vector<CellWeight> bornWeights;
    // The openness of lattice slots inside one plate (planet/fn/Interior.hpp) as last measured, per
    // piece and edge, valid while its stamp is `slotGeneration`: a new substep, a new piece and a
    // consumed one each start a new generation, so nothing measured before them is read.
    std::vector<float> slotOpen;
    std::vector<uint32_t> slotStamp;
    uint32_t slotGeneration{1};
    // Scratch of one interiorSlot call (planet/fn/Interior.cpp): the footprint and cap of each piece it
    // reads, per piece of the globe, valid while its stamp is `footprintCall`.
    std::vector<SpherePolygon> footprint;
    std::vector<PolygonCap> footprintCap;
    std::vector<uint32_t> footprintStamp;
    uint32_t footprintCall{};
    // Per cell, for interiorSlot: whether a slot centred over it is measured (1, 0, or −1 not yet
    // known; a property of `decide` and `region` alone), and the generation in which pieces of two
    // plates were seen over it and its ring: a birth (joinBand) and a consumption (planet/fn/Transfers)
    // each start a new generation, so it holds while the pieces stay those it saw.
    std::vector<int8_t> slotCell;
    std::vector<uint32_t> mixedStamp;
};

// Marks the band from the globe's coupling at the start of a step.
void markBand(const Globe& globe, int sweep, Band& band);

// Marks a band whose core is the given cells, as markBand widens its own (planet/fn/Cohesion: the cells
// a fragment of a plate leaves when it joins the plate around it).
void markBandAt(const Globe& globe, int sweep, const std::vector<int32_t>& cells, Band& band);

// Exact overlaps of every live band piece as the pieces stand; lists per region cell.
void coupleBand(const Globe& globe, Band& band, Workers* workers = nullptr);

// coupleBand when no band piece has moved since the last one (closeSeams' rounds): the overlaps of the
// pieces coupled then are kept and the births' are taken from joinBand, the same bits as coupleBand.
void recoupleBand(Band& band);

// The rings round a point within which every cell that a footprint centred there can reach lies: a
// footprint reaches at most its cell's largest chord to a corner from its centre, and two of those plus
// a quarter of the shortest edge (about two edges, the rhombic cells at the cube's corners included)
// stay under three rings of the narrowest cells on the cubed sphere, while a piece outside the band
// stays at least two rings, 2.5 edges, from a deciding cell (Band), so the check cannot fail falsely.
inline constexpr int GuardRings = 3;

// The pieces outside the band whose centre at the step's start lies within sweep + GuardRings + 1 rings
// of a deciding cell: the only ones a step's turn could bring to one, since the sweep is taken from the
// fastest plate. Read before the pieces outside the band turn.
std::vector<int32_t> nearBand(const Globe& globe, const Band& band);

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
    for (const PieceWeight& w : band.born[size_t(r)])
        if (!band.dead[size_t(w.piece)]) live(w.piece, w.area);   // a birth sutured in its own substep is gone
}

} // namespace derivean::sim

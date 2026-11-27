#pragma once

#include "planet/fn/Regions.hpp"

namespace derivean::sim {

// Per cell, every cloud-in-cell piece whose footprint can overlap it, in key order: the cells
// candidateCells finds from its footprint's cap, so also those its weights leave empty (Codex
// DREV-CLAUDE-008-06). An exactly coupled piece is listed by the coupling itself in every cell it
// overlaps, so the pieces that can reach a cell are its coupling's exact ones and these.
struct CellCandidates {
    std::vector<int32_t> start;    // per cell + 1
    std::vector<int32_t> pieces;
};
CellCandidates candidatesOf(const Globe& globe);

struct OwnerScratch {
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    std::vector<int32_t> pieces;
    std::vector<PieceWeight> entries;
};

// Fills piece k's real weights once: the coupling's where it is coupled exactly, else exactCoupling.
void realOf(const Globe& globe, int32_t k, RealWeights& real, OwnerScratch& scratch);

// Appends to `entries`, in key order, the real overlap with cell c of every piece that can reach it and
// covers some of it.
void realEntriesOf(const Globe& globe, const CellCandidates& candidates, int32_t c, RealWeights& real, OwnerScratch& scratch, std::vector<PieceWeight>& entries);

// Classifies cell c from its pieces' overlaps `[begin, end)` in key order: `dominant`, `owner` (by
// OwnedShare), and in `covered` the owner's pieces' summed overlaps, at most the cell.
void classify(const Globe& globe, int32_t c, const PieceWeight* begin, const PieceWeight* end, PlateRegions& regions, std::vector<double>& covered);

// Classifies every cell as the real overlaps of all its pieces would, before any region is grouped.
// Where bounds of the overlaps decide the owner, the coupling's classification stands, with that owner;
// elsewhere the cell is classified by the real overlaps of every piece that can reach it.
void classifyCells(const Globe& globe, const CellCandidates& candidates, PlateRegions& regions, std::vector<double>& covered, OwnerScratch& scratch);

} // namespace derivean::sim

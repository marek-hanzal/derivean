#include "planet/fn/Owner.hpp"

#include "geometry/fn/Candidates.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

// A cloud-in-cell candidate's overlap with a cell, normalised as exactCoupling normalises it, lies in
// [0, min(a_k, A_c) · (1 + BoundMargin)]: the overlap is the footprint clipped to the cell, so it exceeds
// neither, the normalising scales it by a_k / Σ with Σ within 1e-9 of a_k (exactCoupling), and the margin
// covers that. An exactly coupled piece's overlap is its weight. Codex's default bound is [0, a_k] plus
// a margin (DREV-CLAUDE-008-06); taking the cell's area too decides nothing more (where a_k ≥ A_c either
// bound exceeds any overlap). A narrower bound is a step of its own, with the bound measured.
constexpr double BoundMargin = 1e-6;
constexpr int32_t Undecided = -2;

struct Verdict {
    int32_t owner = Undecided;   // plate, -1 free, or Undecided
    int32_t piece = -1;          // the piece whose lower bound decides a plate
    double least = 0;            // m², that bound
};

double upperOf(const CrustPiece& piece, double cell) { return std::min(piece.area, cell) * (1 + BoundMargin); }

// The owner the bounds decide for cell c: the plate of the piece with the largest lower bound (the lower
// key on a tie) when that bound reaches OwnedShare and no piece of another plate can cover as much (one
// of a higher key may cover as much: it loses the tie), free when no piece can reach OwnedShare.
Verdict verdictOf(const Globe& globe, const CellCandidates& candidates, int32_t c) {
    const Coupling& coupling = globe.coupling;
    const double area = globe.grid->area[size_t(c)], owned = OwnedShare * area;
    Verdict verdict;
    double most = 0;   // the largest upper bound
    for (int32_t s = coupling.cellStart[size_t(c)]; s < coupling.cellStart[size_t(c) + 1]; ++s) {
        const PieceWeight& w = coupling.byCell[size_t(s)];
        if (!coupling.exact[size_t(w.piece)]) continue;
        if (w.area > verdict.least) verdict.least = w.area, verdict.piece = w.piece;
        most = std::max(most, w.area);
    }
    for (int32_t s = candidates.start[size_t(c)]; s < candidates.start[size_t(c) + 1]; ++s)
        most = std::max(most, upperOf(globe.pieces[size_t(candidates.pieces[size_t(s)])], area));
    if (verdict.piece < 0 || verdict.least < owned) {
        verdict.owner = most < owned ? -1 : Undecided;
        return verdict;
    }
    const int32_t plate = globe.pieces[size_t(verdict.piece)].plate;
    const auto loses = [&](int32_t k, double upper) {
        return globe.pieces[size_t(k)].plate == plate || upper < verdict.least || (upper == verdict.least && k > verdict.piece);
    };
    for (int32_t s = coupling.cellStart[size_t(c)]; s < coupling.cellStart[size_t(c) + 1]; ++s) {
        const PieceWeight& w = coupling.byCell[size_t(s)];
        if (coupling.exact[size_t(w.piece)] && !loses(w.piece, w.area)) return verdict;
    }
    for (int32_t s = candidates.start[size_t(c)]; s < candidates.start[size_t(c) + 1]; ++s) {
        const int32_t k = candidates.pieces[size_t(s)];
        if (!loses(k, upperOf(globe.pieces[size_t(k)], area))) return verdict;
    }
    verdict.owner = plate;
    return verdict;
}

} // namespace

CellCandidates candidatesOf(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const size_t n = globe.pieces.size();
    std::vector<int32_t> pieceStart(n + 1, 0), cells;
    std::vector<int> found, work;
    for (size_t k = 0; k < n; ++k) {
        if (!globe.coupling.exact[k]) {
            candidateCells(grid, capOf(footprintOf(grid, globe.pieces[k])), found, work);
            cells.insert(cells.end(), found.begin(), found.end());
        }
        pieceStart[k + 1] = int32_t(cells.size());
    }
    CellCandidates candidates;
    candidates.start.assign(size_t(grid.cells) + 1, 0);
    for (const int32_t c : cells) ++candidates.start[size_t(c) + 1];
    for (int c = 0; c < grid.cells; ++c) candidates.start[size_t(c) + 1] += candidates.start[size_t(c)];
    candidates.pieces.resize(cells.size());
    std::vector<int32_t> fill(candidates.start.begin(), candidates.start.end() - 1);
    for (size_t k = 0; k < n; ++k)   // in key order, so each cell's list is too
        for (int32_t s = pieceStart[k]; s < pieceStart[k + 1]; ++s) candidates.pieces[size_t(fill[size_t(cells[size_t(s)])]++)] = int32_t(k);
    return candidates;
}

void realOf(const Globe& globe, int32_t k, RealWeights& real, OwnerScratch& scratch) {
    if (real.begin[size_t(k)] >= 0) return;
    const Coupling& coupling = globe.coupling;
    real.begin[size_t(k)] = int32_t(real.weights.size());
    if (coupling.exact[size_t(k)]) {
        real.weights.insert(real.weights.end(), coupling.byPiece.begin() + coupling.pieceStart[size_t(k)], coupling.byPiece.begin() + coupling.pieceStart[size_t(k) + 1]);
    } else {
        exactCoupling(*globe.grid, globe.pieces[size_t(k)], scratch.weights, scratch.cells, scratch.work);
        real.weights.insert(real.weights.end(), scratch.weights.begin(), scratch.weights.end());
    }
    real.end[size_t(k)] = int32_t(real.weights.size());
}

void realEntriesOf(const Globe& globe, const CellCandidates& candidates, int32_t c, RealWeights& real, OwnerScratch& scratch, std::vector<PieceWeight>& entries) {
    const Coupling& coupling = globe.coupling;
    std::vector<int32_t>& pieces = scratch.pieces;
    pieces.clear();
    for (int32_t s = coupling.cellStart[size_t(c)]; s < coupling.cellStart[size_t(c) + 1]; ++s)
        if (coupling.exact[size_t(coupling.byCell[size_t(s)].piece)]) pieces.push_back(coupling.byCell[size_t(s)].piece);
    const size_t exact = pieces.size();
    pieces.insert(pieces.end(), candidates.pieces.begin() + candidates.start[size_t(c)], candidates.pieces.begin() + candidates.start[size_t(c) + 1]);
    std::inplace_merge(pieces.begin(), pieces.begin() + std::ptrdiff_t(exact), pieces.end());   // both in key order, disjoint
    for (const int32_t k : pieces) {
        realOf(globe, k, real, scratch);
        for (int32_t s = real.begin[size_t(k)]; s < real.end[size_t(k)]; ++s) {
            const CellWeight& w = real.weights[size_t(s)];
            if (w.cell == c && w.area > 0) entries.push_back({k, w.area});
        }
    }
}

void classify(const Globe& globe, int32_t c, const PieceWeight* begin, const PieceWeight* end, PlateRegions& regions, std::vector<double>& covered) {
    const double area = globe.grid->area[size_t(c)];
    int32_t best = -1;
    double most = 0, own = 0;
    for (const PieceWeight* w = begin; w != end; ++w)
        if (w->area > most) most = w->area, best = w->piece;
    regions.dominant[size_t(c)] = best, regions.owner[size_t(c)] = -1, covered[size_t(c)] = 0;
    if (best < 0 || most < OwnedShare * area) return;
    const int32_t plate = globe.pieces[size_t(best)].plate;
    for (const PieceWeight* w = begin; w != end; ++w)
        if (globe.pieces[size_t(w->piece)].plate == plate) own += w->area;
    regions.owner[size_t(c)] = plate, covered[size_t(c)] = std::min(own, area);
}

// Where the bounds decide the owner the coupling's weights give, the coupling's classification stands,
// its `dominant` and `covered` too: they decide nothing for a plate in one region (its area is not
// weighed), and a plate in two regions is classified again by real overlaps (planet/fn/Regions). A
// cloud-in-cell piece can weigh a cell its footprint does not reach (the corner of its 2 × 2 stencil, or
// across a face edge where the stencil keeps to its face), so the bounds, which only candidates enter,
// can decide another owner than the weights: that cell is classified by real overlaps too.
void classifyCells(const Globe& globe, const CellCandidates& candidates, PlateRegions& regions, std::vector<double>& covered, OwnerScratch& scratch) {
    const Coupling& coupling = globe.coupling;
    for (int32_t c = 0; c < globe.grid->cells; ++c) {
        const PieceWeight* list = coupling.byCell.data();
        classify(globe, c, list + coupling.cellStart[size_t(c)], list + coupling.cellStart[size_t(c) + 1], regions, covered);
        const Verdict verdict = verdictOf(globe, candidates, c);
        if (verdict.owner == Undecided || verdict.owner != regions.owner[size_t(c)]) {
            scratch.entries.clear();
            realEntriesOf(globe, candidates, c, regions.real, scratch, scratch.entries);
            classify(globe, c, scratch.entries.data(), scratch.entries.data() + scratch.entries.size(), regions, covered);
        }
    }
}

} // namespace derivean::sim

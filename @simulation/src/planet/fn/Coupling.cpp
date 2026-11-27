#include "planet/fn/Coupling.hpp"

#include "geometry/fn/Candidates.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Projection.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// Before normalisation the exact overlaps of a footprint sum to its area within this share (Codex V3;
// S3a measured 4e-14 at level 7).
constexpr double CoverageTolerance = 1e-9;

void add(std::vector<CellWeight>& weights, int cell, double area) {
    for (CellWeight& w : weights)
        if (w.cell == cell) {
            w.area += area;
            return;
        }
    weights.push_back({cell, area});
}

} // namespace

double exactWeights(const SphereGrid& grid, const CrustPiece& piece, std::vector<CellWeight>& weights, std::vector<int>& cells, std::vector<int>& work) {
    weights.clear();
    const PreparedPolygon footprint = preparedOf(footprintOf(grid, piece));
    candidateCells(grid, footprint.cap, cells, work);
    double sum = 0;
    for (const int cell : cells) {
        const double area = overlapArea(footprint, polygonOf(grid, cell));
        if (area <= 0) continue;
        weights.push_back({cell, area});
        sum += area;
    }
    return sum;
}

void cicWeights(const SphereGrid& grid, const CrustPiece& piece, std::vector<CellWeight>& weights) {
    weights.clear();
    const FacePoint point = facePointOf(centreOf(grid, piece));
    const double fi = uvToSt(point.u) * grid.side - 0.5, fj = uvToSt(point.v) * grid.side - 0.5;
    const int i0 = int(std::floor(fi)), j0 = int(std::floor(fj));
    const double di = fi - i0, dj = fj - j0;
    for (int b = 0; b < 2; ++b)
        for (int a = 0; a < 2; ++a) {
            const double share = (a ? di : 1 - di) * (b ? dj : 1 - dj);
            if (share <= 0) continue;
            // Within the face of the centre: a square straddling a face edge keeps its weight on this side.
            const int i = std::clamp(i0 + a, 0, grid.side - 1), j = std::clamp(j0 + b, 0, grid.side - 1);
            add(weights, (point.face * grid.side + j) * grid.side + i, share * piece.area);
        }
}

void couple(const SphereGrid& grid, const std::vector<CrustPiece>& pieces, const std::vector<uint8_t>& exact, bool allExact, Coupling& coupling) {
    const size_t n = pieces.size();
    coupling.slots.resize(n * CouplingSlots);
    coupling.counts.assign(n, 0);
    coupling.exact.assign(n, 0);
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    for (size_t k = 0; k < n; ++k) {
        const CrustPiece& piece = pieces[k];
        if (allExact || (k < exact.size() && exact[k])) {
            const double sum = exactWeights(grid, piece, weights, cells, work);
            require(std::abs(sum - piece.area) <= CoverageTolerance * piece.area, "exact overlaps of a footprint miss its area", sum / piece.area - 1);
            for (CellWeight& w : weights) w.area *= piece.area / sum;
            coupling.exact[k] = 1;
        } else {
            cicWeights(grid, piece, weights);
        }
        require(weights.size() <= size_t(CouplingSlots), "a footprint meets too many cells", double(weights.size()));
        std::copy(weights.begin(), weights.end(), coupling.slots.begin() + std::ptrdiff_t(k * CouplingSlots));
        coupling.counts[k] = uint8_t(weights.size());
    }

    coupling.pieceStart.assign(n + 1, 0);
    for (size_t k = 0; k < n; ++k) coupling.pieceStart[k + 1] = coupling.pieceStart[k] + coupling.counts[k];
    coupling.byPiece.resize(size_t(coupling.pieceStart[n]));
    coupling.cellStart.assign(size_t(grid.cells) + 1, 0);
    for (size_t k = 0; k < n; ++k)
        for (int s = 0; s < coupling.counts[k]; ++s) {
            const CellWeight& w = coupling.slots[k * CouplingSlots + size_t(s)];
            coupling.byPiece[size_t(coupling.pieceStart[k] + s)] = w;
            ++coupling.cellStart[size_t(w.cell) + 1];
        }
    for (int c = 0; c < grid.cells; ++c) coupling.cellStart[size_t(c) + 1] += coupling.cellStart[size_t(c)];
    // Scattering pieces in key order keeps each cell's list in key order.
    coupling.byCell.resize(coupling.byPiece.size());
    std::vector<int32_t> fill(coupling.cellStart.begin(), coupling.cellStart.end() - 1);
    coupling.covered.assign(size_t(grid.cells), 0.0);
    for (size_t k = 0; k < n; ++k)
        for (int32_t s = coupling.pieceStart[k]; s < coupling.pieceStart[k + 1]; ++s) {
            const CellWeight& w = coupling.byPiece[size_t(s)];
            coupling.byCell[size_t(fill[size_t(w.cell)]++)] = {int32_t(k), w.area};
            coupling.covered[size_t(w.cell)] += w.area;
        }
}

int dominantPiece(const Coupling& coupling, int cell) {
    int best = -1;
    double most = 0;
    for (int32_t k = coupling.cellStart[size_t(cell)]; k < coupling.cellStart[size_t(cell) + 1]; ++k)
        if (coupling.byCell[size_t(k)].area > most) most = coupling.byCell[size_t(k)].area, best = coupling.byCell[size_t(k)].piece;
    return best;
}

double bookCell(const Coupling& coupling, std::vector<CrustPiece>& pieces, int cell, double volume) {
    const double covered = coupling.covered[size_t(cell)];
    if (covered <= 0) return 0;
    Sum booked;
    for (int32_t k = coupling.cellStart[size_t(cell)]; k < coupling.cellStart[size_t(cell) + 1]; ++k) {
        const PieceWeight& w = coupling.byCell[size_t(k)];
        CrustPiece& piece = pieces[size_t(w.piece)];
        const double share = volume * (w.area / covered);
        if (share >= 0) {
            giveFelsic(piece, share);
            booked.add(share);
        } else {
            booked.add(-takeFelsic(piece, -share));
        }
    }
    return booked.value();
}

} // namespace derivean::sim

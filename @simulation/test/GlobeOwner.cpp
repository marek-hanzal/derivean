// Every cell's owner is the one the exact coupling of every piece gives (planet/fn/Owner; Codex
// DREV-CLAUDE-008-06), on level-6 globes (test GlobeEpochs holds it through the steps of a history):
// - Free corridor (Codex's fixture): plate 1 holds both ends of a closed corridor of free cells; a wide
//   piece of plate 0 outside the band covers 1.19 % of the corridor's middle cell, where its
//   cloud-in-cell weights put nothing. The cell is plate 0's, so plate 1 is in two regions and splits.
// - Stranded: a plate whose one piece the cloud-in-cell weights put on a cell another plate's piece
//   holds, while its footprint covers a free cell across a face edge: it owns that cell and stays a plate.
#include "GlobeCases.hpp"
#include "GlobeFixture.hpp"

#include "chronology/constant/Timeline.hpp"
#include "planet/fn/Band.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Cohesion.hpp"
#include "planet/fn/Rotate.hpp"

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

Globe platesGlobe(int level, size_t plates) {
    Globe globe = makeGlobe(42, level, PlateSpeeds::Today, TectonicsStart);
    enterGlobeEpoch(globe);
    globe.epoch.plates.resize(plates), globe.epoch.sites = plates, globe.motion.resize(plates), globe.applied.resize(plates);
    return globe;
}

double weightIn(const Globe& globe, int cell, size_t piece) {
    double sum = 0;
    for (int32_t s = globe.coupling.cellStart[size_t(cell)]; s < globe.coupling.cellStart[size_t(cell) + 1]; ++s)
        if (globe.coupling.byCell[size_t(s)].piece == int32_t(piece)) sum += globe.coupling.byCell[size_t(s)].area;
    return sum;
}

size_t indexOfBirth(const Globe& globe, const CrustPiece& piece) {
    return size_t(std::find_if(globe.pieces.begin(), globe.pieces.end(), [&](const CrustPiece& p) { return p.birth == piece.birth; }) - globe.pieces.begin());
}

void checkFreeCorridor() {
    Globe globe = platesGlobe(6, 2);
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = std::move(globe.pieces);
    const auto cell = [&](int i, int j) { return cellAtFace0(grid, i, j); };
    const auto put = [&](int i, int j, uint16_t plate) { globe.pieces.push_back(original[size_t(cell(i, j))]), globe.pieces.back().plate = plate; };
    globe.pieces.clear();
    for (int i = 2; i <= 49; ++i) {
        put(i, 5, 0);
        if (i != 21) put(i, 7, 0);
    }
    put(2, 6, 0), put(49, 6, 0);
    for (int i = 3; i <= 11; ++i) put(i, 6, 1);
    for (int i = 31; i <= 48; ++i) put(i, 6, 1);
    CrustPiece wide = turnedTo(globe, original[size_t(cell(31, 31))], grid.centre[size_t(cell(21, 7))]);
    wide.plate = 0;
    globe.pieces.push_back(wide);
    sortPieces(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    Band band;
    markBand(globe, 1, band);
    settleBand(globe, band, false);
    const int mid = cell(21, 6), left = cell(9, 6), right = cell(35, 6);
    const size_t k = indexOfBirth(globe, wide);
    const double area = grid.area[size_t(mid)];
    expect(k < globe.pieces.size() && !globe.coupling.exact[k] && weightIn(globe, mid, k) < OwnedShare * area && cellCover(globe, mid, {wide}) > 0.01 * area,
           "the free corridor's wide piece is coupled exactly or weighted on the corridor");
    const PlateRegions regions = plateRegionsOf(globe), exact = exactRegionsOf(globe);
    expect(sameRegions(regions, exact), "the free corridor's classification is not the exact coupling's");
    const int32_t l = regions.region[size_t(left)], r = regions.region[size_t(right)];
    expect(regions.owner[size_t(mid)] == 0 && l >= 0 && r >= 0 && l != r && regions.plate[size_t(l)] == 1 && regions.plate[size_t(r)] == 1,
           "crust another plate stands on does not close the free corridor", regions.owner[size_t(mid)]);
    expect(std::abs(regions.area[size_t(l)] - 0.00430650271751) < 1e-12 && std::abs(regions.area[size_t(r)] - 0.00878609356303) < 1e-12,
           "the free corridor's regions are not Codex's", regions.area[size_t(l)]);
    Globe reference = globe;
    couple(grid, reference.pieces, {}, true, reference.coupling);
    const CrustStock stock = totalStock(globe.pieces);
    keepPlatesWhole(globe, 1), keepPlatesWhole(reference, 1);
    expect(globe.splits.size() == 1 && globe.stats.fragmentsJoined == 0 && stockCloses(globe, stock) && platesValid(globe), "the free corridor does not split once and close");
    expect(ownerId(globe, left) == 16 && ownerId(globe, right) == 1 && ownerId(reference, left) == 16 && ownerId(reference, right) == 1,
           "the free corridor's smaller side is not plate 16 and its larger plate 1", ownerId(globe, left));
}

void checkStranded() {
    Globe globe = platesGlobe(6, 2);
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = std::move(globe.pieces);
    globe.pieces.clear();
    const int edge = cellAtFace0(grid, grid.side - 1, 20);
    int across = -1;   // the cell over face 0's edge
    for (int n = 0; n < 4; ++n)
        if (neighbourOf(grid, edge, n) / (grid.side * grid.side) != 0) across = neighbourOf(grid, edge, n);
    for (int j = 17; j <= 23; ++j)
        for (int i = grid.side - 4; i < grid.side; ++i) globe.pieces.push_back(original[size_t(cellAtFace0(grid, i, j))]), globe.pieces.back().plate = 0;
    // A small piece, born at a cube corner, 0.45 of the way from the edge cell's centre towards the cell
    // across: its weights stay on face 0 (cloud-in-cell keeps to its centre's face), its crust crosses.
    const Vec3 from = grid.centre[size_t(edge)], to = grid.centre[size_t(across)];
    CrustPiece small = turnedTo(globe, original[size_t(cellAtFace0(grid, 0, 0))], normalized(from + (to - from) * 0.45));
    small.plate = 1;
    globe.pieces.push_back(small);
    sortPieces(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    const size_t k = indexOfBirth(globe, small);
    bool weighted = false;   // the coupling gives plate 1 a cell
    for (int c = 0; c < grid.cells; ++c) weighted = weighted || dominantPiece(globe.coupling, c) == int(k);
    expect(across >= 0 && !weighted && weightIn(globe, across, k) == 0 && cellCover(globe, across, {small}) > 0.1 * grid.area[size_t(across)],
           "the stranded fixture's piece holds a cell by weight or covers none across the edge");
    const PlateRegions regions = plateRegionsOf(globe), exact = exactRegionsOf(globe);
    expect(sameRegions(regions, exact), "the stranded fixture's classification is not the exact coupling's");
    expect(regions.owner[size_t(across)] == 1 && regions.largest.size() > 1 && regions.largest[1] >= 0, "a plate whose crust stands on a cell only by its footprint is left with none");
    const CrustStock stock = totalStock(globe.pieces);
    keepPlatesWhole(globe, 1);
    expect(globe.pieces[indexOfBirth(globe, small)].plate == 1 && globe.plateEnds.empty() && stockCloses(globe, stock) && platesValid(globe), "the stranded fixture's plate is dealt away");
}

} // namespace

void testGlobeOwner() {
    checkFreeCorridor();
    checkStranded();
}

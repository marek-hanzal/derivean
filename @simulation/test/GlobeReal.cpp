// Cells owned by real overlaps on hand-made level-6 globes (planet/fn/Regions; lead, 27 Nov), coupled by
// cloud-in-cell weights only:
// - Cascade: a cell that a plate's thin piece holds by its weight and another plate's wide piece covers
//   more of goes to the wide piece's plate in two regions, and so cuts the thin piece's plate, which is
//   then measured too.
// - Deal: a piece that covers no cell most, centred on another plate's cell, whose weight lies there
//   alone, goes with the region its footprint reaches.
// - Cube corner: a patch at a cube vertex holds the cells of three faces its crust stands on.
#include "GlobeCases.hpp"
#include "GlobeFixture.hpp"

#include "chronology/constant/Timeline.hpp"
#include "planet/fn/Cohesion.hpp"
#include "planet/fn/Rotate.hpp"

#include <bit>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

Globe platesGlobe(size_t plates) {
    Globe globe = makeGlobe(42, 6, PlateSpeeds::Today, TectonicsStart);
    enterGlobeEpoch(globe);
    globe.epoch.plates.resize(plates), globe.epoch.sites = plates, globe.motion.resize(plates), globe.applied.resize(plates);
    return globe;
}

double angleBetween(Vec3 a, Vec3 b) { return std::acos(std::clamp(dot(a, b), -1.0, 1.0)); }

// m² of a cell's coupling weights of plate `plate`.
double weightOf(const Globe& globe, int cell, uint16_t plate) {
    double sum = 0;
    for (int32_t s = globe.coupling.cellStart[size_t(cell)]; s < globe.coupling.cellStart[size_t(cell) + 1]; ++s)
        if (globe.pieces[size_t(globe.coupling.byCell[size_t(s)].piece)].plate == plate) sum += globe.coupling.byCell[size_t(s)].area;
    return sum;
}

// Plate 0's second region: a strip far from the fixtures, so plate 0 is in two regions and measured.
void addStrip(Globe& globe, const std::vector<CrustPiece>& original) {
    for (int x = 13; x < 25; ++x) globe.pieces.push_back(original[size_t(cellAtFace0(*globe.grid, x, 58))]), globe.pieces.back().plate = 0;
}

// Codex's 2 × 2 patch of plate 0, born at (16, 31) mid-face and standing unmoved on the cells from (i, j).
std::vector<CrustPiece> patchAt(const Globe& globe, const std::vector<CrustPiece>& original, int i, int j) {
    std::vector<CrustPiece> patch;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x) {
            patch.push_back(turnedTo(globe, original[size_t(cellAtFace0(*globe.grid, 16 + x, 31 + y))], globe.grid->centre[size_t(cellAtFace0(*globe.grid, i + x, j + y))]));
            patch.back().plate = 0;
        }
    return patch;
}

// Plate 2 runs along row 6 between walls of plate 1 (rows 5 and 7, closed at columns 2 and 10). At X =
// (6, 6) it has no piece of its own, only a hundredth of a cell of the piece east of it, moved towards X;
// the wall's cell beside X is a piece of plate 0 born mid-face, whose wider footprint reaches more of X.
void checkCascade() {
    Globe globe = platesGlobe(3);
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = globe.pieces;
    const auto cell = [&](int i, int j) { return cellAtFace0(grid, i, j); };
    const auto add = [&](int i, int j, uint16_t plate) { globe.pieces.push_back(original[size_t(cell(i, j))]), globe.pieces.back().plate = plate; };
    globe.pieces.clear();
    for (int i = 2; i <= 10; ++i) {
        add(i, 5, 1);
        if (i != 6) add(i, 7, 1);
    }
    add(2, 6, 1), add(10, 6, 1);
    for (int i = 5; i <= 7; ++i) add(i, 8, 1);   // closes the wide piece off the free cells, which would join it to the strip
    for (const int i : {4, 5, 8, 9}) add(i, 6, 2);
    // West's end holds two pieces over the same seven tenths of (3, 6), so its plate's summed overlaps
    // and their union differ.
    const Vec3 end = grid.centre[size_t(cell(3, 6))], wall = grid.centre[size_t(cell(2, 6))], at = normalized(end + (wall - end) * 0.3);
    for (const int born : {cell(3, 6), cell(20, 40)}) globe.pieces.push_back(turnedTo(globe, original[size_t(born)], at)), globe.pieces.back().plate = 2;
    const int x = cell(6, 6);
    const Vec3 east = grid.centre[size_t(cell(7, 6))];
    CrustPiece thin = turnedTo(globe, original[size_t(cell(7, 6))], normalized(east + (grid.centre[size_t(x)] - east) * 0.01));
    CrustPiece wide = turnedTo(globe, original[size_t(cell(31, 31))], grid.centre[size_t(cell(6, 7))]);
    thin.plate = 2, wide.plate = 0;
    globe.pieces.push_back(thin), globe.pieces.push_back(wide);
    addStrip(globe, original);
    sortPieces(globe.pieces);
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    const double area = grid.area[size_t(x)], held = weightOf(globe, x, 2), wider = cellCover(globe, x, {wide}), thinner = cellCover(globe, x, {thin});
    expect(held >= OwnedShare * area && held > weightOf(globe, x, 0) && wider > thinner && thinner > 0, "the cascade fixture's cell is not plate 2's by weight and plate 0's by crust", wider / area);
    const PlateRegions regions = plateRegionsOf(globe);
    const int32_t west = regions.region[size_t(cell(5, 6))], rest = regions.region[size_t(cell(8, 6))];
    expect(regions.owner[size_t(x)] == 0 && west >= 0 && rest >= 0 && west != rest && regions.plate[size_t(west)] == 2 && regions.plate[size_t(rest)] == 2,
           "a cell another plate's crust covers more of does not cut the plate that held it", regions.owner[size_t(x)]);
    std::vector<CrustPiece> two;
    bool real = true;   // plate 2 is classified by real overlaps: its pieces' are read
    for (size_t k = 0; k < globe.pieces.size(); ++k)
        if (globe.pieces[k].plate == 2) two.push_back(globe.pieces[k]), real = real && regions.real.begin[k] >= 0;
    expect(real, "a plate the cascade cuts is not classified by real overlaps");
    const double cover = coverOf(globe, regions, west, two);
    double summed = 0;   // sr of plate 2's overlaps summed per cell of west, each cell at most whole
    for (int c = 0; c < grid.cells; ++c) {
        if (regions.region[size_t(c)] != west) continue;
        double sum = 0;
        for (const CrustPiece& piece : two) sum += cellCover(globe, c, {piece});
        summed += std::min(sum, grid.area[size_t(c)]) / (PlanetRadius * PlanetRadius);
    }
    expect(summed - cover > 1e-3 * cover, "the cascade fixture's west does not overlap itself", summed / cover - 1);
    expect(std::abs(regions.area[size_t(west)] - cover) <= 1e-9 * cover, "a plate the cascade cuts is not measured by its footprints", regions.area[size_t(west)] / cover - 1);
    keepPlatesWhole(globe, 1);
    expect(globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe), "the cascade fixture does not close");
}

// The unmoved patch at (5, 5), ringed by plate 1 two to four cells round, with a wide piece of plate 1
// on (7, 5), whole pieces of plate 1 on the cells east of it, and a piece of plate 0 centred on (7, 5)
// too: the coupling gives that piece's weight to (7, 5), which the wider one holds, and none to the
// patch's region, which its footprint reaches; the patch covers those cells most.
void checkRealDeal() {
    Globe globe = platesGlobe(2);
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = globe.pieces;
    constexpr int i = 5, j = 5;
    globe.pieces = patchAt(globe, original, i, j);
    for (int a = i - 3; a <= i + 4; ++a)
        for (int b = j - 3; b <= j + 4; ++b)
            if (a == i - 3 || a == i + 4 || b == j - 3 || b == j + 4) globe.pieces.push_back(original[size_t(cellAtFace0(grid, a, b))]), globe.pieces.back().plate = 1;
    const int under = cellAtFace0(grid, 7, 5);
    CrustPiece wide = turnedTo(globe, original[size_t(cellAtFace0(grid, 31, 31))], grid.centre[size_t(under)]);
    CrustPiece lone = turnedTo(globe, original[size_t(cellAtFace0(grid, 16, 29))], grid.centre[size_t(under)]);
    wide.plate = 1, lone.plate = 0;
    globe.pieces.push_back(wide), globe.pieces.push_back(lone);
    for (const auto [a, b] : {std::pair{7, 4}, {7, 6}, {8, 4}, {8, 5}, {8, 6}}) globe.pieces.push_back(original[size_t(cellAtFace0(grid, a, b))]), globe.pieces.back().plate = 1;
    addStrip(globe, original);
    sortPieces(globe.pieces);
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    const auto find = [&] {
        for (size_t k = 0; k < globe.pieces.size(); ++k)
            if (globe.pieces[k].birth == lone.birth) return k;
        return globe.pieces.size();
    };
    const size_t k = find();
    const PlateRegions regions = plateRegionsOf(globe);
    const int32_t moved = regions.region[size_t(cellAtFace0(grid, i, j))];
    bool weighed = true, reaches = false;
    for (int32_t w = globe.coupling.pieceStart[k]; w < globe.coupling.pieceStart[k + 1]; ++w) {
        const CellWeight& weight = globe.coupling.byPiece[size_t(w)];
        weighed = weighed && (regions.region[size_t(weight.cell)] != moved || weight.area == 0);
    }
    for (int32_t w = regions.real.begin[k]; w >= 0 && w < regions.real.end[k]; ++w)
        reaches = reaches || (regions.region[size_t(regions.real.weights[size_t(w)].cell)] == moved && regions.real.weights[size_t(w)].area >= OwnedShare * grid.area[size_t(regions.real.weights[size_t(w)].cell)]);
    const double margin = cellCover(globe, under, {wide}) - cellCover(globe, under, {lone});   // the wide piece holds (7, 5) by more than rounding
    expect(moved >= 0 && regions.largest[0] != moved && margin > 1e-6 * grid.area[size_t(under)] && regions.owner[size_t(under)] == 1 && std::find(regions.dominant.begin(), regions.dominant.end(), int32_t(k)) == regions.dominant.end() && weighed && reaches,
           "the deal fixture's piece covers a cell most, lies off plate 1 or misses the patch", double(k));
    const int strip = cellAtFace0(grid, 13, 58);
    keepPlatesWhole(globe, 1);
    const size_t after = find();
    expect(after < globe.pieces.size() && globe.motion[globe.pieces[after].plate].id == ownerId(globe, cellAtFace0(grid, i, j)) && ownerId(globe, cellAtFace0(grid, i, j)) != ownerId(globe, strip),
           "a piece that covers no cell most does not go with the region its footprint reaches", double(after));
    expect(globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe), "the deal fixture does not close");
}

// The unmoved patch on the cells (0..1, 0..1) of face 0, at a cube vertex, plate 1 more than seven
// cells away: the region is the cells of the three faces its footprints stand on, found by clipping
// every cell within five cells, and its area their union. The candidate lists are tightest here
// (planet/fn/Regions.cpp); a piece they miss is fatal.
void checkCubeCorner() {
    Globe globe = platesGlobe(2);
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = globe.pieces;
    const int corner = cellAtFace0(grid, 0, 0);
    const Vec3 at = grid.centre[size_t(corner)];
    const double step = std::sqrt(grid.area[size_t(corner)]) / PlanetRadius;
    globe.pieces = patchAt(globe, original, 0, 0);
    const std::vector<CrustPiece> patch = globe.pieces;
    for (const CrustPiece& piece : original) {
        const Cell born = cellAt(grid, piece.birth);
        if (angleBetween(grid.centre[size_t(piece.birth)], at) <= 7 * step || (born.face == 0 && born.i >= 16 && born.i < 18 && born.j >= 31 && born.j < 33)) continue;   // the patch's own
        globe.pieces.push_back(piece), globe.pieces.back().plate = born.face == 0 && born.j == 58 && born.i >= 13 && born.i < 25 ? 0 : 1;   // the strip
    }
    sortPieces(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    const PlateRegions regions = plateRegionsOf(globe);
    const int32_t moved = regions.region[size_t(corner)];
    int cic = 0, standing = 0, faces = 0;
    double cover = 0;
    for (int c = 0; c < grid.cells; ++c) {
        if (angleBetween(grid.centre[size_t(c)], at) > 5 * step) continue;
        const double area = cellCover(globe, c, patch);
        cic += weightOf(globe, c, 0) >= OwnedShare * grid.area[size_t(c)];
        if (!standsOn(globe, c, patch)) continue;
        ++standing, cover += area / (PlanetRadius * PlanetRadius);
        faces |= 1 << cellAt(grid, c).face;
    }
    expect(moved >= 0 && standing > cic && std::popcount(unsigned(faces)) == 3, "the corner patch's crust does not reach three faces beyond its weights", double(standing));
    expect(regions.cells[size_t(moved)] == standing && std::abs(regions.area[size_t(moved)] - cover) <= 1e-9 * cover,
           "a region at a cube vertex does not hold the cells its crust stands on", double(regions.cells[size_t(moved)]));
}

} // namespace

void testGlobeReal() {
    checkCascade();
    checkRealDeal();
    checkCubeCorner();
}

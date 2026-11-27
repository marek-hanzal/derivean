// Targeted cases of keepPlatesWhole on hand-made globes (Codex DREV-CLAUDE-008-01 and -02, whose
// fixtures these follow; lead's decisions of 27 Nov). 260 lines: one fixture per case, each a few
// dozen lines of set-up that only that case reads.
// - A one-cell fragment bordered by four plates joins the one with the longest border in metres.
// - A fragment parted from its plate's other cells by free cells alone is one region with them: nothing
//   joins, splits or is reported unsettled.
// - A piece whose exact overlaps reach its free neighbours by rounding (1e-17 of a cell) owns its cell
//   only: cut off by another plate's ring it is one cell and its own cover, so it joins that plate and
//   nothing splits off.
// - A region of the covered area and cells splits off with a new id; a plate left with no piece ends
//   once; ids are unique and given in order; a second pass repeats no event; the stock closes.
// - Pieces of one plate that overlap one another are measured by their union, exactly and with
//   cloud-in-cell weights: their summed overlaps reach 0.002 sr, the union does not, and nothing splits.
#include "GlobeFixture.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Cohesion.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

Globe globeOf(int plates) {
    Globe globe = makeGlobe(42, 5, PlateSpeeds::Today, TectonicsStart);
    enterGlobeEpoch(globe);
    globe.epoch.plates.resize(size_t(plates));
    globe.epoch.sites = size_t(plates);
    globe.motion.resize(size_t(plates));
    globe.applied.resize(size_t(plates));
    return globe;
}

int cellAtFace0(const SphereGrid& grid, int i, int j) {
    Cell cell = cellAt(grid, 0);
    cell.i = i, cell.j = j;
    return indexOf(grid, cell);
}

int ownerId(const Globe& globe, int cell) {
    const int piece = dominantPiece(globe.coupling, cell);
    return piece >= 0 ? globe.motion[globe.pieces[size_t(piece)].plate].id : -1;
}

// The stock with the interior closes, each part to 1e-15 of itself: the interior's ledger adds a seam's
// births and consumptions one by one in plain doubles (planet/fn/Lay, Transfers), so a closing may leave
// an ulp or two of it (the rounding fixture: 4 m³ of basalt, a few ulps of its sum), where the fixtures with no
// seam close exactly.
bool stockCloses(const Globe& globe, CrustStock before) {
    const CrustStock after = totalStock(globe.pieces);
    return std::abs(after.felsic + globe.ledger.interior.felsic - before.felsic) <= 1e-15 * before.felsic &&
           std::abs(after.basalt + globe.ledger.interior.basalt - before.basalt) <= 1e-15 * before.basalt;
}

// Whether every piece names a plate there is.
bool platesValid(const Globe& globe) {
    return std::all_of(globe.pieces.begin(), globe.pieces.end(), [&](const CrustPiece& piece) { return piece.plate < globe.epoch.plates.size(); }) &&
           globe.motion.size() == globe.epoch.plates.size() && globe.applied.size() == globe.epoch.plates.size();
}

// Only the pieces born in the given cells, with their plates.
void keepOnly(Globe& globe, const std::vector<std::pair<int, int>>& cellPlates) {
    std::vector<CrustPiece> kept;
    for (const auto& [cell, plate] : cellPlates) {
        CrustPiece piece = globe.pieces[size_t(cell)];
        piece.plate = uint16_t(plate);
        kept.push_back(piece);
    }
    globe.pieces = std::move(kept);
}

void checkLongestBorder() {
    Globe globe = globeOf(5);
    const SphereGrid& grid = *globe.grid;
    const int fragment = cellAtFace0(grid, 7, 9);
    std::array<int, 4> sides{0, 1, 2, 3};
    const auto edgeAt = [&](int k) { return grid.edge[size_t(grid.edges[size_t(fragment)][size_t(k)])]; };
    std::sort(sides.begin(), sides.end(), [&](int a, int b) { return edgeAt(a).length < edgeAt(b).length; });
    expect(edgeAt(sides[2]).length < edgeAt(sides[3]).length, "the border fixture has no one longest edge");
    for (CrustPiece& piece : globe.pieces) piece.plate = 3;
    globe.pieces[size_t(fragment)].plate = 4;
    for (int i = 18; i <= 20; ++i) globe.pieces[size_t(cellAtFace0(grid, i, 19))].plate = 4;
    for (size_t i = 0; i < sides.size(); ++i) globe.pieces[size_t(neighbourOf(grid, fragment, sides[i]))].plate = uint16_t(i);
    const int longest = globe.motion[3].id;
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, true, globe.coupling);
    keepPlatesWhole(globe, 1);
    expect(ownerId(globe, fragment) == longest, "a fragment does not join the plate of its longest border", ownerId(globe, fragment));
    expect(globe.stats.fragmentsJoined == 1 && globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe), "the border fixture does not settle whole", double(globe.stats.fragmentsJoined));
}

// The first cell from (4, 4) whose piece's weights hold it alone (CIC), or hold it and only rounding
// beyond it (exact).
int loneCell(const Globe& globe, bool exact) {
    const SphereGrid& grid = *globe.grid;
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    for (int x = 4; x <= 12; ++x)
        for (int y = 4; y <= 25; ++y) {
            const int c = cellAtFace0(grid, x, y);
            const CrustPiece& piece = globe.pieces[size_t(c)];
            if (exact) exactWeights(grid, piece, weights, cells, work);
            else cicWeights(grid, piece, weights);
            if (!exact && weights.size() == 1 && weights.front().cell == c) return c;
            if (exact && weights.size() > 1 && std::all_of(weights.begin(), weights.end(), [&](const CellWeight& w) { return w.cell == c || w.area < 1e-10 * piece.area; }))
                return c;
        }
    return -1;
}

void checkParted() {
    Globe globe = globeOf(2);
    const SphereGrid& grid = *globe.grid;
    const int fragment = loneCell(globe, false);
    expect(fragment >= 0, "no cell holds its piece alone");
    const Cell at = cellAt(grid, fragment);
    const int core = cellAtFace0(grid, at.i + 5, at.j), foreign = cellAtFace0(grid, at.i + 14, at.j);
    keepOnly(globe, {{fragment, 0}, {core, 0}, {cellAtFace0(grid, at.i + 6, at.j), 0}, {foreign, 1}});
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    const PlateRegions regions = plateRegionsOf(globe);
    expect(regions.region[size_t(fragment)] == regions.region[size_t(core)], "free cells part a plate into regions");
    keepPlatesWhole(globe, 1);
    expect(globe.stats.wholeUnsettled == 0 && globe.stats.fragmentsJoined == 0 && globe.splits.empty() && stockCloses(globe, stock),
           "a plate parted by free cells alone does not stay whole", double(globe.stats.wholeUnsettled));
}

void checkRoundingOverlap() {
    Globe globe = globeOf(2);
    const SphereGrid& grid = *globe.grid;
    const int fragment = loneCell(globe, true);
    expect(fragment >= 0, "no piece reaches beyond its cell by rounding alone");
    const Cell at = cellAt(grid, fragment);
    std::vector<std::pair<int, int>> kept{{fragment, 0}};
    for (int a = -2; a <= 2; ++a)
        for (int b = -2; b <= 2; ++b)
            if (std::max(std::abs(a), std::abs(b)) == 2) kept.push_back({cellAtFace0(grid, at.i + a, at.j + b), 1});
    for (int a = 8; a <= 10; ++a) kept.push_back({cellAtFace0(grid, at.i + a, at.j), 0});
    keepOnly(globe, kept);
    const int foreign = globe.motion[1].id;
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, true, globe.coupling);
    const PlateRegions regions = plateRegionsOf(globe);
    const size_t region = size_t(regions.region[size_t(fragment)]);
    const double cell = grid.area[size_t(fragment)] / (PlanetRadius * PlanetRadius);
    for (int k = 0; k < 4; ++k) expect(regions.owner[size_t(neighbourOf(grid, fragment, k))] < 0, "a rounding overlap owns a free cell", k);
    expect(regions.cells[region] == 1 && regions.area[region] <= cell && regions.area[region] > (1 - 1e-9) * cell, "a piece's rounding counts free cells to its region",
           regions.area[region] / cell);
    keepPlatesWhole(globe, 1);
    expect(globe.splits.empty(), "a piece's rounding overlap splits a plate off", globe.splits.empty() ? 0 : globe.splits.front().area);
    expect(ownerId(globe, fragment) == foreign && globe.stats.fragmentsJoined >= 1 && globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock),
           "a one-cell fragment ringed by another plate does not join it", double(globe.stats.fragmentsJoined));
}

void checkIdentity() {
    Globe globe = globeOf(3);
    const SphereGrid& grid = *globe.grid;
    for (CrustPiece& piece : globe.pieces) piece.plate = 1;
    for (int i = 13; i <= 16; ++i) globe.pieces[size_t(cellAtFace0(grid, i, 19))].plate = 0;
    for (int i = 7; i <= 8; ++i) globe.pieces[size_t(cellAtFace0(grid, i, 9))].plate = 0;
    const int parent = globe.motion[0].id, neighbour = globe.motion[1].id, dead = globe.motion[2].id, next = globe.platesMade;
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, true, globe.coupling);
    const PlateRegions regions = plateRegionsOf(globe);
    const size_t cut = size_t(regions.region[size_t(cellAtFace0(grid, 7, 9))]);
    expect(regions.cells[cut] == MicroplateCells && regions.area[cut] >= MicroplateArea, "the identity fixture's region is not a microplate", regions.area[cut]);
    keepPlatesWhole(globe, 1);
    expect(globe.splits.size() == 1 && globe.splits[0].parent == parent && globe.splits[0].child == next && globe.platesMade == next + 1,
           "the two-cell region does not split off with the next id", double(globe.splits.size()));
    expect(globe.splits[0].area == regions.area[cut], "a split records another area than its region's cover", globe.splits[0].area);
    expect(globe.plateEnds.size() == 1 && globe.plateEnds[0].id == dead, "the plate with no piece does not end once", double(globe.plateEnds.size()));
    expect(ownerId(globe, cellAtFace0(grid, 7, 9)) == next && ownerId(globe, cellAtFace0(grid, 13, 19)) == parent && ownerId(globe, cellAtFace0(grid, 0, 0)) == neighbour,
           "a split or its parent lost its cells");
    std::vector<int32_t> ids;
    for (const GlobeMotion& motion : globe.motion) ids.push_back(motion.id);
    std::sort(ids.begin(), ids.end());
    expect(std::adjacent_find(ids.begin(), ids.end()) == ids.end() && ids.back() < globe.platesMade && globe.epoch.sites == 2, "plate ids repeat or run past those given");
    keepPlatesWhole(globe, 2);
    expect(globe.splits.size() == 1 && globe.plateEnds.size() == 1 && globe.platesMade == next + 1, "a second pass repeats a split or an end", double(globe.splits.size()));
    expect(globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe), "the identity fixture does not close");
}

// Codex's covered-area fixture (DREV-CLAUDE-008-02): four whole-cell pieces of plate 0 on level 6, each
// moved by part of a cell so that they overlap one another, ringed by plate 1, coupled exactly or by
// cloud-in-cell weights. Their overlaps summed per cell reach 0.002 sr, the union of their footprints
// does not: the region's area is that union, and the fragment joins rather than splits.
void checkOverlappingCover(bool exact) {
    Globe globe = makeGlobe(42, 6, PlateSpeeds::Today, TectonicsStart);
    enterGlobeEpoch(globe);
    globe.epoch.plates.resize(2), globe.epoch.sites = 2, globe.motion.resize(2), globe.applied.resize(2);
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = globe.pieces;
    constexpr int i = 9, j = 27;
    globe.pieces.clear();
    for (int a = i - 3; a <= i + 4; ++a)
        for (int b = j - 3; b <= j + 4; ++b)
            if (a == i - 3 || a == i + 4 || b == j - 3 || b == j + 4) globe.pieces.push_back(original[size_t(cellAtFace0(grid, a, b))]), globe.pieces.back().plate = 1;
    for (int a = 48; a < 56; ++a) globe.pieces.push_back(original[size_t(cellAtFace0(grid, a, 57))]), globe.pieces.back().plate = 0;
    int fragment = -1;
    const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(cellAtFace0(grid, i, j))]);
    std::vector<PlanePolygon> footprints;
    for (int k = 0; k < 4; ++k) {
        const int cell = cellAtFace0(grid, i + (k & 1), j + (k >> 1));
        const Cell birth = cellAt(grid, cell);
        CrustPiece piece = original[size_t(cell)];
        const Vec3 p = grid.centre[size_t(cell)];
        const Vec3 along = grid.centre[size_t(cellAtFace0(grid, birth.i + 1, birth.j))] - p, across = grid.centre[size_t(cellAtFace0(grid, birth.i, birth.j + 1))] - p;
        const Vec3 target = normalized(p + along * ((k & 1) ? 0.32 : 0.45) + across * 0.46), axis = cross(p, target);
        piece.rotation = rotationAbout(axis, std::atan2(length(axis), dot(p, target)));
        piece.plate = 0;
        if (k == 0) fragment = indexOf(grid, cellOf(centreOf(grid, piece), grid.level));
        footprints.push_back(projected(plane, footprintOf(grid, piece)));
        globe.pieces.push_back(piece);
    }
    const double cover = unionArea(footprints).area / (PlanetRadius * PlanetRadius);   // 0.00193172 sr (Codex)
    sortPieces(globe.pieces);
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, exact, globe.coupling);
    const PlateRegions regions = plateRegionsOf(globe);
    const int32_t r = regions.region[size_t(fragment)];
    expect(r >= 0 && regions.plate[size_t(r)] == 0 && regions.largest[0] != r, "the covered-area fixture's fragment is not a fragment of plate 0", exact);
    double bound = 0;
    for (size_t c = 0; c < regions.region.size(); ++c) {
        if (regions.region[c] != r || regions.owner[c] != 0) continue;
        double sum = 0;
        for (int32_t s = globe.coupling.cellStart[c]; s < globe.coupling.cellStart[c + 1]; ++s)
            if (globe.pieces[size_t(globe.coupling.byCell[size_t(s)].piece)].plate == 0) sum += globe.coupling.byCell[size_t(s)].area;
        bound += std::min(sum, grid.area[c]) / (PlanetRadius * PlanetRadius);
    }
    expect(bound >= MicroplateArea && cover < MicroplateArea - 1e-5 && regions.cells[size_t(r)] >= MicroplateCells, "the covered-area fixture is not between bound and union", bound);
    expect(std::abs(regions.area[size_t(r)] - cover) <= 1e-6 * cover, "a fragment's area counts overlapping crust twice", regions.area[size_t(r)] / cover - 1);
    keepPlatesWhole(globe, 1);
    expect(globe.splits.empty() && globe.stats.fragmentsJoined > 0 && globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe),
           "overlapping crust under 0.002 sr splits off a plate", globe.splits.empty() ? 0 : globe.splits.front().area);
}

} // namespace

void testGlobeCohesion() {
    checkLongestBorder();
    checkParted();
    checkRoundingOverlap();
    checkIdentity();
    checkOverlappingCover(true);
    checkOverlappingCover(false);
}

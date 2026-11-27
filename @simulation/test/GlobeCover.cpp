// The covered area of a plate's regions on hand-made level-6 globes, after Codex's fixtures
// (DREV-CLAUDE-008-02 and -05; lead, 27 Nov): a plate in two regions or more is measured by the union of
// its real footprints, exactly coupled or by cloud-in-cell weights.
// - Four pieces that overlap one another sum to 0.002 sr, their union does not: the fragment joins.
// - A moved 2 × 2 patch whose cloud-in-cell weights sum under 0.002 sr covers more: it splits off.
// - Of two regions, the one whose cloud-in-cell weights sum less but whose crust covers more is the
//   plate's largest: it keeps the plate's id and the other splits off.
#include "GlobeCases.hpp"
#include "GlobeFixture.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Cohesion.hpp"
#include "planet/fn/Rotate.hpp"

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

Globe twoPlates() {
    Globe globe = makeGlobe(42, 6, PlateSpeeds::Today, TectonicsStart);
    enterGlobeEpoch(globe);
    globe.epoch.plates.resize(2), globe.epoch.sites = 2, globe.motion.resize(2), globe.applied.resize(2);
    return globe;
}

// The ring of plate 1 two to four cells round (i, j) on face 0.
void ringOf(Globe& globe, const std::vector<CrustPiece>& original, int i, int j) {
    for (int a = i - 3; a <= i + 4; ++a)
        for (int b = j - 3; b <= j + 4; ++b)
            if (a == i - 3 || a == i + 4 || b == j - 3 || b == j + 4) globe.pieces.push_back(original[size_t(cellAtFace0(*globe.grid, a, b))]), globe.pieces.back().plate = 1;
}

// sr a region's cells hold of the given pieces' footprints, by clipping and union cell by cell.
double coverOf(const Globe& globe, const PlateRegions& regions, int32_t region, const std::vector<CrustPiece>& pieces) {
    const SphereGrid& grid = *globe.grid;
    double cover = 0;
    for (int c = 0; c < grid.cells; ++c) {
        if (regions.region[size_t(c)] != region) continue;
        const TangentPlane plane = tangentPlaneAt(grid.centre[size_t(c)]);
        const PlanePolygon cell = projected(plane, polygonOf(grid, c));
        std::vector<PlanePolygon> parts;
        for (const CrustPiece& piece : pieces) {
            const PlanePolygon part = clipped(projected(plane, footprintOf(grid, piece)), cell);
            if (part.count >= 3) parts.push_back(part);
        }
        if (!parts.empty()) cover += unionArea(parts).area;
    }
    return cover / (PlanetRadius * PlanetRadius);
}

// sr of a region's plate-0 weights, each cell at most whole.
double summedOf(const Globe& globe, const PlateRegions& regions, int32_t region) {
    const SphereGrid& grid = *globe.grid;
    double area = 0;
    for (size_t c = 0; c < regions.region.size(); ++c) {
        if (regions.region[c] != region || regions.owner[c] != 0) continue;
        double sum = 0;
        for (int32_t s = globe.coupling.cellStart[c]; s < globe.coupling.cellStart[c + 1]; ++s)
            if (globe.pieces[size_t(globe.coupling.byCell[size_t(s)].piece)].plate == 0) sum += globe.coupling.byCell[size_t(s)].area;
        area += std::min(sum, grid.area[c]) / (PlanetRadius * PlanetRadius);
    }
    return area;
}

// Codex's covered-area fixture (DREV-CLAUDE-008-02): four whole-cell pieces of plate 0 on level 6, each
// moved by part of a cell so that they overlap one another, ringed by plate 1, coupled exactly or by
// cloud-in-cell weights. Their overlaps summed per cell reach 0.002 sr, the union of their footprints
// does not: the region's area is that union, and the fragment joins rather than splits.
void checkOverlappingCover(bool exact) {
    Globe globe = twoPlates();
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = globe.pieces;
    constexpr int i = 9, j = 27;
    globe.pieces.clear();
    ringOf(globe, original, i, j);
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
    const double bound = summedOf(globe, regions, r);
    expect(bound >= MicroplateArea && cover < MicroplateArea - 1e-5 && regions.cells[size_t(r)] >= MicroplateCells, "the covered-area fixture is not between bound and union", bound);
    expect(std::abs(regions.area[size_t(r)] - cover) <= 1e-6 * cover, "a fragment's area counts overlapping crust twice", regions.area[size_t(r)] / cover - 1);
    keepPlatesWhole(globe, 1);
    expect(globe.splits.empty() && globe.stats.fragmentsJoined > 0 && globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe),
           "overlapping crust under 0.002 sr splits off a plate", globe.splits.empty() ? 0 : globe.splits.front().area);
}

// Codex's cloud-in-cell fixtures (DREV-CLAUDE-008-05): a 2 × 2 patch of plate 0 moved to (5, 5) by 0.03
// of a cell and ringed by plate 1, against a strip of plate 0 elsewhere (threshold) or a 2 × 2 block of
// 0.002005–0.00202 sr (largest). The patch's cloud-in-cell weights sum under 0.002 sr and its real crust
// covers more; the block's weights sum more than the patch's and its crust covers less.
void checkCloudCover(bool largest) {
    Globe globe = twoPlates();
    const SphereGrid& grid = *globe.grid;
    const std::vector<CrustPiece> original = globe.pieces;
    constexpr int i = 5, j = 5;
    std::vector<CrustPiece> patch;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x) {
            CrustPiece piece = original[size_t(cellAtFace0(grid, 16 + x, 31 + y))];
            const Vec3 born = grid.centre[size_t(piece.birth)], p = grid.centre[size_t(cellAtFace0(grid, i + x, j + y))];
            const Vec3 u = grid.centre[size_t(cellAtFace0(grid, i + x + 1, j + y))] - p, v = grid.centre[size_t(cellAtFace0(grid, i + x, j + y + 1))] - p;
            const Vec3 target = normalized(p + u * 0.03 + v * 0.03), axis = cross(born, target);
            piece.rotation = rotationAbout(axis, std::atan2(length(axis), dot(born, target)));
            piece.plate = 0;
            patch.push_back(piece);
        }
    globe.pieces = patch;
    ringOf(globe, original, i, j);
    std::vector<CrustPiece> other;
    int otherCell = cellAtFace0(grid, 13, 58);
    if (!largest)
        for (int x = 13; x < 25; ++x) other.push_back(original[size_t(cellAtFace0(grid, x, 58))]);
    for (int a = 22; a < 58 && largest && other.empty(); ++a)
        for (int b = 18; b < 56 && other.empty(); ++b) {
            const int corners[] = {cellAtFace0(grid, a, b), cellAtFace0(grid, a + 1, b), cellAtFace0(grid, a, b + 1), cellAtFace0(grid, a + 1, b + 1)};
            double area = 0;
            for (const int cell : corners) area += grid.area[size_t(cell)] / (PlanetRadius * PlanetRadius);
            if (area < 0.002005 || area > 0.002020) continue;
            for (const int cell : corners) other.push_back(original[size_t(cell)]);
            otherCell = corners[0];
        }
    expect(!other.empty(), "no 2 x 2 block of 0.002005-0.00202 sr on face 0");
    for (CrustPiece piece : other) globe.pieces.push_back(piece), globe.pieces.back().plate = 0;
    sortPieces(globe.pieces);
    const CrustStock stock = totalStock(globe.pieces);
    couple(grid, globe.pieces, {}, false, globe.coupling);
    const PlateRegions regions = plateRegionsOf(globe);
    const int32_t moved = regions.region[size_t(cellAtFace0(grid, i, j))], rest = regions.region[size_t(otherCell)];
    expect(moved >= 0 && rest >= 0 && moved != rest && regions.plate[size_t(moved)] == 0 && regions.plate[size_t(rest)] == 0 && regions.cells[size_t(moved)] >= MicroplateCells, "the cloud-in-cell fixture's regions are not two of plate 0", largest);
    const double movedCover = coverOf(globe, regions, moved, patch), restCover = coverOf(globe, regions, rest, other);
    const double movedSum = summedOf(globe, regions, moved), restSum = summedOf(globe, regions, rest);
    expect(movedSum < MicroplateArea && movedCover > MicroplateArea && (!largest || (restSum > movedSum && restCover > MicroplateArea && restCover < movedCover)),
           "the cloud-in-cell fixture's weights do not misjudge it", movedSum);
    expect(std::abs(regions.area[size_t(moved)] - movedCover) <= 1e-9 * movedCover && regions.largest[0] == (largest ? moved : rest),
           "a region's cloud-in-cell weights decide its plate's largest or a microplate", regions.area[size_t(moved)]);
    const int parent = globe.motion[0].id, child = globe.platesMade;
    keepPlatesWhole(globe, 1);
    const int movedId = ownerId(globe, cellAtFace0(grid, i, j)), restId = ownerId(globe, otherCell);
    expect(globe.splits.size() == 1 && globe.stats.fragmentsJoined == 0 && (largest ? movedId == parent && restId == child : movedId == child && restId == parent),
           "the crust that stands there does not decide the split and the parent's id", double(movedId));
    expect(globe.stats.wholeUnsettled == 0 && stockCloses(globe, stock) && platesValid(globe), "the cloud-in-cell fixture does not close", largest);
}

} // namespace

void testGlobeCover() {
    checkOverlappingCover(true);
    checkOverlappingCover(false);
    checkCloudCover(false);
    checkCloudCover(true);
}

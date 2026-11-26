// Overlaps of geodesic polygons and the candidate cells of a rotated footprint (S3a): exact cases,
// Codex's counterexample to "centre and neighbours" (DREV-CLAUDE-003-02) and coverage before any
// normalisation over thousands of rotations inside faces, across face edges and round cube vertices.
#include "GeometrySphereFixture.hpp"

#include "geometry/fn/Candidates.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Projection.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"

#include <algorithm>
#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::geometry_sphere_test;

namespace {

constexpr double Pi = std::numbers::pi;

bool listed(const std::vector<int>& cells, int cell) { return std::find(cells.begin(), cells.end(), cell) != cells.end(); }

// A footprint at a random birth cell, carried to a random point of one kind and spun at random:
// place 0 inside a face, 1 within one and a half cells of a face edge, 2 of a cube vertex.
SpherePolygon randomFootprint(const SphereGrid& grid, Random& random, int place, int& birth) {
    birth = std::min(grid.cells - 1, int(random.next() * grid.cells));
    const int face = std::min(5, int(random.next() * 6));
    const double band = 1.5 / grid.side;
    auto edgeward = [&] {
        const double d = band * random.next();
        return random.next() < 0.5 ? d : 1 - d;
    };
    double s = 0.1 + 0.8 * random.next(), t = 0.1 + 0.8 * random.next();
    if (place >= 1) s = edgeward();
    if (place == 2) t = edgeward();
    if (random.next() < 0.5) std::swap(s, t);
    const Vec3 target = directionOf({face, stToUv(s), stToUv(t)});
    return rotated(polygonOf(grid, birth), carryAndSpin(grid.centre[size_t(birth)], target, 2 * Pi * random.next()));
}

double coveredArea(const SphereGrid& grid, const SpherePolygon& footprint, const std::vector<int>& cells) {
    double sum = 0;
    for (int cell : cells) sum += overlapArea(footprint, polygonOf(grid, cell));
    return sum;
}

} // namespace

namespace derivean::sim::geometry_sphere_test {

// A cell with itself, with its neighbours across edges and corners, with cells two rows away, with
// its parent; an offset 2 × 2 block with the parents it straddles; symmetry in the arguments.
void checkOverlap() {
    for (int level : {2, 5, 7, 10})
        for (const Cell& cell : geometry_test::samplesAt(level)) {
            const SpherePolygon polygon = polygonOf(cell);
            const double area = areaOf(cell);
            expect(near(areaOf(polygon) / area, 1, 1e-12), "polygon area is not the cell's", areaOf(polygon) / area - 1);
            expect(near(overlapArea(polygon, polygon) / area, 1, 1e-12), "a cell does not overlap itself in its whole area",
                   overlapArea(polygon, polygon) / area - 1);
            for (int k = 0; k < 4; ++k) {
                const Neighbour across = neighbourAcross(cell, k);
                const SpherePolygon side = polygonOf(across.cell);
                expect(overlapArea(polygon, side) < 1e-12 * area && overlapArea(side, polygon) < 1e-12 * area, "cells sharing an edge overlap",
                       overlapArea(polygon, side) / area);
                const CornerCells around = cellsAround(cell, k);
                for (int n = 0; n < around.count; ++n)
                    expect(overlapArea(polygon, polygonOf(around.cells[n])) < 1e-12 * area, "cells sharing a corner overlap", level);
                const Neighbour beyond = neighbourAcross(across.cell, (across.edge + 2) % 4);
                if (level >= 2) expect(overlapArea(polygon, polygonOf(beyond.cell)) == 0, "cells two rows apart overlap", level);
            }
            if (level >= 3) {
                const double inParent = overlapArea(polygonOf(parentOf(cell, level - 1)), polygon);
                expect(near(inParent / area, 1, 1e-12), "a cell does not overlap its parent in its whole area", inParent / area - 1);
            }
        }
    for (int face = 0; face < FaceCount; ++face)
        for (int level : {5, 7}) {
            const int i = 2 * (face + 3) + 1, j = cellsPerSide(level) - 3;   // odd, so the block straddles four parents
            SpherePolygon block;
            block.count = 4;
            block.corner = {cornerOf({face, level, i, j}, 0), cornerOf({face, level, i + 1, j}, 1), cornerOf({face, level, i + 1, j + 1}, 2),
                            cornerOf({face, level, i, j + 1}, 3)};
            double whole = 0;
            for (int di = 0; di < 2; ++di)
                for (int dj = 0; dj < 2; ++dj) {
                    const Cell child{face, level, i + di, j + dj};
                    const double part = overlapArea(block, polygonOf(parentOf(child, level - 1)));
                    expect(near(part / areaOf(child), 1, 1e-12), "a block does not overlap a parent in its one child", part / areaOf(child) - 1);
                    whole += areaOf(child);
                }
            expect(near(overlapArea(block, block) / whole, 1, 1e-12), "a block does not overlap itself in its four cells", level);
        }
    Random random{11};
    const SphereGrid grid = makeSphereGrid(5);
    std::vector<int> cells, work;
    for (int n = 0; n < 300; ++n) {
        int birth = 0;
        const SpherePolygon footprint = randomFootprint(grid, random, n % 3, birth);
        candidateCells(grid, footprint, cells, work);
        for (int cell : cells) {
            const SpherePolygon other = polygonOf(grid, cell);
            expect(near(overlapArea(footprint, other), overlapArea(other, footprint), 1e-12 * grid.area[size_t(birth)]), "overlap is not symmetric", n);
        }
    }
}

// Codex's fixture, then coverage: the exact overlaps over the candidates sum to the footprint's area
// to 1e-12 before any normalisation, and no cell outside the candidates overlaps it.
void checkCandidates() {
    std::vector<int> cells, work;
    {
        const SphereGrid grid = makeSphereGrid(7);
        const Cell born{0, 7, 127, 127};
        const int birth = indexOf(grid, born);
        const Vec3 centre = grid.centre[size_t(birth)], corner = cornerOf(born, 1);   // (s, t) = (1, 127/128)
        expect(near(distanceBetween(centre, corner), 63577.254, 1e-3), "the fixture's corner is not 63 577.254 m from the centre",
               distanceBetween(centre, corner));
        const Vec3 target = directionOf({0, stToUv(0.995), stToUv(0.5 - 1e-7)});
        const Vec3 toCorner = normalized(corner - centre * dot(corner, centre)), north = normalized(Vec3{0, 0, 1} - target * target.z);
        const Vec3 carried = rotate(carryAndSpin(centre, target, 0), toCorner);
        const Rotation q = carryAndSpin(centre, target, std::atan2(dot(target, cross(carried, north)), dot(carried, north)));
        expect(cellOf(rotate(q, centre), 7) == Cell{0, 7, 127, 63}, "the fixture's centre does not land in (127, 63)");
        expect(cellOf(rotate(q, corner), 7) == Cell{0, 7, 127, 65}, "the fixture's corner does not land in (127, 65)");
        const SpherePolygon footprint = rotated(polygonOf(grid, birth), q);
        candidateCells(grid, footprint, cells, work);
        const int far = indexOf(grid, {0, 7, 127, 65});
        expect(listed(cells, far), "the cell the rotated corner lands in is not a candidate");
        expect(overlapArea(footprint, polygonOf(grid, far)) > 0, "the fixture's far cell does not overlap the footprint");
        const double area = grid.area[size_t(birth)], covered = coveredArea(grid, footprint, cells);
        expect(near(covered / area, 1, 1e-12), "the fixture's candidates do not cover the footprint", covered / area - 1);
    }
    for (int level : {5, 7}) {
        const SphereGrid grid = makeSphereGrid(level);
        Random random{std::uint64_t(level)};
        double worst = 0;
        for (int n = 0; n < 6000; ++n) {
            int birth = 0;
            const SpherePolygon footprint = randomFootprint(grid, random, n % 3, birth);
            candidateCells(grid, footprint, cells, work);
            worst = std::max(worst, std::abs(coveredArea(grid, footprint, cells) / grid.area[size_t(birth)] - 1));
            if (level == 5 && n < 90) {
                const double area = grid.area[size_t(birth)];
                for (int cell = 0; cell < grid.cells; ++cell)
                    if (!listed(cells, cell))
                        expect(overlapArea(footprint, polygonOf(grid, cell)) <= 1e-14 * area, "a cell outside the candidates overlaps the footprint", cell);
            }
        }
        expect(worst < 1e-12, "candidates do not cover rotated footprints before normalisation", worst);
    }
}

} // namespace derivean::sim::geometry_sphere_test

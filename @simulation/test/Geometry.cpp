// The grid of the sphere: lengths, areas, neighbours, keys and round trips. Frames, transport and the
// tangent budget are in GeometryFrame.cpp.
#include "GeometryFixture.hpp"

#include "geometry/fn/Frame.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"
#include "geometry/fn/Projection.hpp"

#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::geometry_test;

namespace {

constexpr double Pi = std::numbers::pi;

// Sum of all cell areas against 4πR² on every level that can be enumerated (6 × 4^10 cells), and the
// finer levels of the tile ladder through their parents: 4^4 descendants sum to their ancestor.
void checkAreas() {
    const double sphere = 4 * Pi * PlanetRadius * PlanetRadius;
    for (int level = 0; level <= 10; ++level) {
        double sum = 0, carry = 0;   // Neumaier summation
        const int count = cellsPerSide(level);
        for (int face = 0; face < FaceCount; ++face)
            for (int i = 0; i < count; ++i)
                for (int j = 0; j < count; ++j) {
                    const double area = areaOf({face, level, i, j}), next = sum + area;
                    carry += std::abs(sum) >= area ? (sum - next) + area : (area - next) + sum;
                    sum = next;
                }
        expect(near((sum + carry) / sphere, 1, 1e-9), "cell areas do not sum to 4πR²", (sum + carry) / sphere - 1);
    }
    for (int level : {SiteLevel, GroundLevel, GroundCellLevel, MaxLevel})
        for (const Cell& cell : samplesAt(level - 4)) {
            double sum = 0;
            for (int i = 0; i < 16; ++i)
                for (int j = 0; j < 16; ++j) sum += areaOf({cell.face, level, cell.i * 16 + i, cell.j * 16 + j});
            expect(near(sum / areaOf(cell), 1, 1e-12), "descendants do not sum to their ancestor's area", sum / areaOf(cell) - 1);
        }
    const double mean = sphere / (6.0 * cellsPerSide(10) * cellsPerSide(10));
    expect(near(areaOf({0, 10, 512, 512}) / mean, 1.1781, 1e-3), "face-centre cell is not 1.178 of the mean area");
    expect(near(areaOf({0, 10, 0, 512}) / mean, 0.8337, 1e-3), "cell at a face edge's middle is not 0.834 of the mean area");
    expect(near(areaOf({0, 10, 0, 0}) / mean, 0.9069, 1e-3), "face-corner cell is not 0.907 of the mean area");
}

// Face edge vertex to vertex 7 842.4 km, arc across the face centre 10 007.5 km, Ground cell 0.298 m,
// edge lengths against the angle between the corners.
void checkLengths() {
    const Cell face{0, 0, 0, 0};
    expect(near(edgeLengthOf(face, 0), 7842.4e3, 50), "face edge is not 7 842.4 km", edgeLengthOf(face, 0));
    const double across = distanceBetween(faceUvToCube(0, -1, 0), faceUvToCube(0, 1, 0));
    expect(near(across, 10007.5e3, 50), "arc across the face centre is not 10 007.5 km", across);
    expect(near(across / cellsPerSide(GroundCellLevel), 0.298, 5e-4), "Ground cell is not nominally 0.298 m");
    for (int level : {0, 5, 13, GroundCellLevel})
        for (const Cell& cell : samplesAt(level))
            for (int edge = 0; edge < 4; ++edge) {
                const double chord = distanceBetween(cornerOf(cell, edge), cornerOf(cell, (edge + 1) % 4));
                expect(near(edgeLengthOf(cell, edge) / chord, 1, level < 20 ? 1e-12 : 1e-7), "edge length disagrees with its corners");
            }
}

// Symmetry and orientation of every edge neighbour: crossing back returns the cell and edge, and the
// shared edge runs the other way (our corner k is the neighbour's corner edge + 1). All cells of level
// 4 and the samples of SampleLevels.
void checkNeighbours() {
    std::vector<Cell> cells;
    for (int face = 0; face < FaceCount; ++face)
        for (int i = 0; i < 16; ++i)
            for (int j = 0; j < 16; ++j) cells.push_back({face, 4, i, j});
    for (int level : SampleLevels)
        for (const Cell& cell : samplesAt(level)) cells.push_back(cell);
    for (const Cell& cell : cells)
        for (int edge = 0; edge < 4; ++edge) {
            const Neighbour next = neighbourAcross(cell, edge), back = neighbourAcross(next.cell, next.edge);
            expect(isValid(next.cell) && !(next.cell == cell), "neighbour is not a valid other cell");
            expect(back.cell == cell && back.edge == edge, "neighbours are not symmetric");
            expect(same(cornerOf(next.cell, (next.edge + 1) % 4), cornerOf(cell, edge), 1e-15) &&
                   same(cornerOf(next.cell, next.edge), cornerOf(cell, (edge + 1) % 4), 1e-15), "shared edge is not oriented back");
        }
    for (const Cell& cell : cells)
        for (int corner = 0; corner < 4; ++corner) {
            const CornerCells around = cellsAround(cell, corner);
            expect(around.count == (atCubeVertex(cell, corner) ? 2 : 3), "wrong number of cells round a corner", around.count);
            for (int k = 0; k < around.count; ++k)
                expect(same(cornerOf(around.cells[k], around.corner[k]), cornerOf(cell, corner), 1e-15), "cell round a corner does not share it");
        }
    // Codex's seam counterexample: with 32 tiles a face, (0, 0, 1) and (4, 30, 31) touch across face 0's edge 3.
    const Neighbour seam = neighbourAcross({0, 5, 0, 1}, 3);
    expect(seam.cell == Cell{4, 5, 30, 31} && seam.edge == 2, "face 0 edge 3 does not lead to (4, 30, 31)");
    expect(faceEdgeOf(0, 3).face == 4 && faceEdgeOf(0, 3).edge == 2 && faceEdgeOf(0, 3).reversed, "face edge table is not exposed");
}

// Face point <-> ECEF <-> (face, level, i, j), heights, keys and the quadtree; S2's face tie rule.
void checkRoundTrips() {
    for (double s : {0.0, 1e-9, 0.25, 0.5, 0.731, 1.0})
        expect(near(uvToSt(stToUv(s)), s, 1e-15), "ST -> UV -> ST does not return");
    expect(facePointOf({1, 1, 0.5}).face == 1 && facePointOf({1, 1, 1}).face == 2 && facePointOf({-1, -1, 0}).face == 4 &&
           facePointOf({-1, 0.5, -1}).face == 5, "face ties do not go to the higher axis");
    for (int level : SampleLevels)
        for (const Cell& cell : samplesAt(level)) {
            const Vec3 centre = centreOf(cell);
            for (double height : {-16000.0, 0.0, 0.125, 8848.0}) {
                const Vec3 ecef = ecefOf(centre, height);
                expect(near(heightOf(ecef), height, 1e-8), "radial height does not return", heightOf(ecef) - height);
                expect(cellOf(ecef, level) == cell, "ECEF does not return to its cell");
            }
            const FacePoint point = facePointOf(centre);
            const double count = cellsPerSide(level);
            expect(point.face == cell.face, "centre is not on its face");
            expect(near(uvToSt(point.u), (cell.i + 0.5) / count, 1e-14) && near(uvToSt(point.v), (cell.j + 0.5) / count, 1e-14),
                   "centre is not the middle of the cell's ST square");
            expect(same(directionOf(point), centre, 1e-15), "face point does not return to its direction");
            expect(cellOfKey(keyOf(cell)) == cell, "key does not return to its cell");
            if (level == 0) continue;
            const Cell parent = parentOf(cell, level - 1);
            expect(childOf(parent, (cell.i & 1) | ((cell.j & 1) << 1)) == cell, "child of parent is not the cell");
            const std::uint64_t key = keyOf(cell), up = keyOf(parent), half = up & (~up + 1);
            expect(key > up - half && key < up + half, "key is outside its parent's range");
        }
    // The layout of GeometryVersion 1, fixed.
    const int last = cellsPerSide(MaxLevel) - 1;
    expect(keyOf({0, 0, 0, 0}) == 0x1000000000000000ull && keyOf({0, 1, 1, 0}) == 0x1400000000000000ull &&
           keyOf({0, 1, 0, 1}) == 0x0C00000000000000ull && keyOf({5, MaxLevel, last, last}) == 0xBFFFFFFFFFFFFFFFull,
           "key layout changed without a GeometryVersion bump");
    // A point inside a face exactly on a cell edge in ST (the face centre) goes to the cell above it.
    expect(cellOf(FacePoint{0, 0.0, 0.0}, 2) == Cell{0, 2, 2, 2}, "a point on a cell edge does not go to the cell above");
}

} // namespace

void testGeometry() {
    checkLengths();
    checkNeighbours();
    checkRoundTrips();
    checkFrames();
    checkAreas();
}

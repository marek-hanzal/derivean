#include "atmosphere/fn/SphereGrid.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Edge.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"

#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// The gnomonic projection onto the tangent plane at `at` (unit), in metres: great circles map to
// straight lines, so the chords of a diamond stay straight.
Vec3 tangentPoint(Vec3 point, Vec3 at) { return (point * (1.0 / dot(point, at)) - at) * PlanetRadius; }

// Least squares of value = α + β · p over the cells' tangent points p (vertex at the origin): the
// weights are α's row of (AᵀA)⁻¹Aᵀ with rows A = (1, p). Three cells (a cube vertex) are an exact
// interpolation.
void fitVertex(const SphereGrid& grid, SphereVertex& vertex, Vec3 at) {
    const Vec3 east = normalized(cross(std::abs(at.z) < 0.9 ? Vec3{0, 0, 1} : Vec3{1, 0, 0}, at)), north = cross(at, east);
    double x[4], y[4];
    double m[3][3] = {};
    for (int k = 0; k < vertex.count; ++k) {
        const Vec3 p = tangentPoint(grid.centre[size_t(vertex.cells[size_t(k)])], at) * (1.0 / PlanetRadius);
        x[k] = dot(p, east);
        y[k] = dot(p, north);
        const double row[3] = {1, x[k], y[k]};
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) m[a][b] += row[a] * row[b];
    }
    // First row of the inverse of the symmetric 3 × 3 normal matrix, by cofactors.
    const double c0 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    const double c1 = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    const double c2 = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    const double determinant = m[0][0] * c0 + m[0][1] * c1 + m[0][2] * c2;
    for (int k = 0; k < vertex.count; ++k) vertex.weight[size_t(k)] = (c0 + c1 * x[k] + c2 * y[k]) / determinant;
}

void numberVertices(SphereGrid& grid) {
    for (auto& corners : grid.corners) corners = {-1, -1, -1, -1};
    for (int c = 0; c < grid.cells; ++c) {
        const Cell cell = cellAt(grid, c);
        for (int k = 0; k < 4; ++k) {
            if (grid.corners[size_t(c)][size_t(k)] >= 0) continue;
            const int id = int(grid.vertex.size());
            SphereVertex vertex;
            vertex.cells[0] = c;
            vertex.count = 1;
            grid.corners[size_t(c)][size_t(k)] = id;
            const CornerCells around = cellsAround(cell, k);
            for (int n = 0; n < around.count; ++n) {
                const int other = indexOf(grid, around.cells[n]);
                grid.corners[size_t(other)][size_t(around.corner[n])] = id;
                vertex.cells[size_t(vertex.count++)] = other;
            }
            fitVertex(grid, vertex, cornerOf(cell, k));
            grid.vertex.push_back(vertex);
        }
    }
}

SphereEdge edgeBetween(const SphereGrid& grid, int left, int k, int right) {
    const CellEdge shape = edgeOf(cellAt(grid, left), k);
    SphereEdge edge;
    edge.left = left;
    edge.right = right;
    edge.from = grid.corners[size_t(left)][size_t(k)];
    edge.to = grid.corners[size_t(left)][size_t((k + 1) % 4)];
    edge.length = shape.length;
    edge.normal = shape.normal;
    const Vec3 l = grid.centre[size_t(left)], r = grid.centre[size_t(right)];
    edge.diamond = (sphericalExcess(l, shape.from, shape.to) + sphericalExcess(r, shape.to, shape.from)) * PlanetRadius * PlanetRadius;

    // Dual basis of the two chords in the tangent plane at the midpoint (rotating a chord a quarter
    // turn about the midpoint's up gives the vector orthogonal to it).
    const Vec3 m = shape.midpoint;
    const Vec3 across = tangentPoint(r, m) - tangentPoint(l, m);
    const Vec3 along = tangentPoint(shape.to, m) - tangentPoint(shape.from, m);
    const Vec3 normalToAlong = cross(m, along), normalToAcross = cross(m, across);
    edge.across = normalToAlong * (1.0 / dot(across, normalToAlong));
    edge.along = normalToAcross * (1.0 / dot(along, normalToAcross));
    edge.chord = along;
    edge.offset = (tangentPoint(l, m) + tangentPoint(r, m)) * -0.5;
    return edge;
}

} // namespace

SphereGrid makeSphereGrid(int level) {
    assert(level >= 0 && level <= 12);
    SphereGrid grid;
    grid.level = level;
    grid.side = cellsPerSide(level);
    grid.cells = FaceCount * grid.side * grid.side;
    grid.area.resize(size_t(grid.cells));
    grid.centre.resize(size_t(grid.cells));
    grid.latitudeSine.resize(size_t(grid.cells));
    grid.edges.resize(size_t(grid.cells));
    grid.corners.resize(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        const Cell cell = cellAt(grid, c);
        grid.area[size_t(c)] = areaOf(cell);
        grid.centre[size_t(c)] = centreOf(cell);
        grid.latitudeSine[size_t(c)] = grid.centre[size_t(c)].z;
    }
    numberVertices(grid);
    grid.edge.reserve(size_t(2 * grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        const Cell cell = cellAt(grid, c);
        for (int k = 0; k < 4; ++k) {
            const Neighbour next = neighbourAcross(cell, k);
            const int n = indexOf(grid, next.cell);
            if (n < c) continue;
            const int id = int(grid.edge.size());
            grid.edge.push_back(edgeBetween(grid, c, k, n));
            grid.edges[size_t(c)][size_t(k)] = id;
            grid.edges[size_t(n)][size_t(next.edge)] = id;
        }
    }
    for (int c = 0; c < grid.cells; ++c)
        for (int k = 0; k < 4; ++k) {
            SphereEdge& edge = grid.edge[size_t(grid.edges[size_t(c)][size_t(k)])];
            const SphereEdge& opposite = grid.edge[size_t(grid.edges[size_t(c)][size_t((k + 2) % 4)])];
            const int behind = opposite.left == c ? opposite.right : opposite.left;
            (edge.left == c ? edge.behindLeft : edge.behindRight) = behind;
        }
    return grid;
}

SpherePlace placeOf(const SphereGrid& grid, int cell) {
    const Cell at = cellAt(grid, cell);
    for (int k = 0; k < 4; ++k)
        if (atCubeVertex(at, k)) return SpherePlace::CubeCorner;
    const int last = grid.side - 1;
    if (at.i == 0 || at.j == 0 || at.i == last || at.j == last) return SpherePlace::FaceEdge;
    return SpherePlace::Interior;
}

} // namespace derivean::sim

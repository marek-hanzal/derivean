#include "atmosphere/fn/SphereStencil.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

// The gnomonic projection onto the tangent plane at `at` (unit), in metres: great circles map to
// straight lines, so the chords of a diamond stay straight.
Vec3 tangentPoint(Vec3 point, Vec3 at) { return (point * (1.0 / dot(point, at)) - at) * PlanetRadius; }

// Least squares of value = α + β · p over the cells' tangent points p (vertex at the origin): the
// weights are α's row of (AᵀA)⁻¹Aᵀ with rows A = (1, p). Three cells (a cube vertex) are an exact
// interpolation.
std::array<double, 4> fitVertex(const SphereGrid& grid, const SphereVertex& vertex) {
    const Vec3 at = vertex.point;
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
    std::array<double, 4> weight{};
    for (int k = 0; k < vertex.count; ++k) weight[size_t(k)] = (c0 + c1 * x[k] + c2 * y[k]) / determinant;
    return weight;
}

// The edge as its left cell numbers it (k): its corners and the midpoint of its arc exactly as
// geometry/fn/Edge's edgeOf gives them, without the normal and length the grid already holds.
EdgeStencil stencilOf(const SphereGrid& grid, const SphereEdge& geometric, int k) {
    const Cell cell = cellAt(grid, geometric.left);
    const Vec3 from = cornerOf(cell, k), to = cornerOf(cell, (k + 1) % 4), m = normalized(from + to);
    EdgeStencil edge;
    const Vec3 l = grid.centre[size_t(geometric.left)], r = grid.centre[size_t(geometric.right)];
    edge.diamond = (sphericalExcess(l, from, to) + sphericalExcess(r, to, from)) * PlanetRadius * PlanetRadius;

    // Dual basis of the two chords in the tangent plane at the midpoint (rotating a chord a quarter
    // turn about the midpoint's up gives the vector orthogonal to it).
    const Vec3 across = tangentPoint(r, m) - tangentPoint(l, m);
    const Vec3 along = tangentPoint(to, m) - tangentPoint(from, m);
    const Vec3 normalToAlong = cross(m, along), normalToAcross = cross(m, across);
    edge.across = normalToAlong * (1.0 / dot(across, normalToAlong));
    edge.along = normalToAcross * (1.0 / dot(along, normalToAcross));
    edge.chord = along;
    edge.offset = (tangentPoint(l, m) + tangentPoint(r, m)) * -0.5;
    return edge;
}

} // namespace

SphereStencil makeSphereStencil(const SphereGrid& grid) {
    SphereStencil stencil;
    stencil.weight.reserve(grid.vertex.size());
    for (const SphereVertex& vertex : grid.vertex) stencil.weight.push_back(fitVertex(grid, vertex));
    stencil.edge.resize(grid.edge.size());
    stencil.behind.resize(grid.edge.size());
    for (int c = 0; c < grid.cells; ++c)
        for (int k = 0; k < 4; ++k) {
            const int e = grid.edges[size_t(c)][size_t(k)];
            if (grid.edge[size_t(e)].left == c) stencil.edge[size_t(e)] = stencilOf(grid, grid.edge[size_t(e)], k);
        }
    for (int c = 0; c < grid.cells; ++c)
        for (int k = 0; k < 4; ++k) {
            const int e = grid.edges[size_t(c)][size_t(k)];
            stencil.behind[size_t(e)][grid.edge[size_t(e)].left == c ? 0 : 1] = neighbourOf(grid, c, (k + 2) % 4);
        }
    stencil.carry.reserve(stencil.edge.size());
    for (const EdgeStencil& edge : stencil.edge) stencil.carry.push_back({dot(edge.offset, edge.across), dot(edge.offset, edge.along)});
    stencil.topology.reserve(grid.edge.size());
    for (const SphereEdge& edge : grid.edge) stencil.topology.push_back({edge.left, edge.right, edge.from, edge.to});
    stencil.vertexCells.reserve(grid.vertex.size());
    for (const SphereVertex& vertex : grid.vertex) {
        std::array<int, 4> cells = vertex.cells;
        for (int k = vertex.count; k < 4; ++k) cells[size_t(k)] = cells[0];
        stencil.vertexCells.push_back(cells);
    }
    return stencil;
}

void releaseEdgeTables(SphereStencil& stencil) { std::vector<EdgeStencil>().swap(stencil.edge); }

} // namespace derivean::sim

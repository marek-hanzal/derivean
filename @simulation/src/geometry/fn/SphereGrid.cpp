#include "geometry/fn/SphereGrid.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Edge.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"

#include <algorithm>
#include <cassert>

namespace derivean::sim {
namespace {

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
            vertex.point = cornerOf(cell, k);
            grid.corners[size_t(c)][size_t(k)] = id;
            const CornerCells around = cellsAround(cell, k);
            for (int n = 0; n < around.count; ++n) {
                const int other = indexOf(grid, around.cells[n]);
                grid.corners[size_t(other)][size_t(around.corner[n])] = id;
                vertex.cells[size_t(vertex.count++)] = other;
            }
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
    grid.reach.resize(size_t(grid.cells));
    grid.edges.resize(size_t(grid.cells));
    grid.corners.resize(size_t(grid.cells));
    for (int c = 0; c < grid.cells; ++c) {
        const Cell cell = cellAt(grid, c);
        grid.area[size_t(c)] = areaOf(cell);
        grid.centre[size_t(c)] = centreOf(cell);
    }
    numberVertices(grid);
    for (int c = 0; c < grid.cells; ++c) {
        double reach = 0;
        for (int vertex : grid.corners[size_t(c)]) reach = std::max(reach, length(grid.vertex[size_t(vertex)].point - grid.centre[size_t(c)]));
        grid.reach[size_t(c)] = reach;
    }
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

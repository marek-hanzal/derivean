#include "geometry/fn/Neighbour.hpp"

#include "geometry/fn/Cell.hpp"

namespace derivean::sim {

namespace {

constexpr FaceEdge FaceEdges[6][4] = {
    {{5, 2, false}, {1, 3, false}, {2, 3, true}, {4, 2, true}},
    {{5, 1, true}, {3, 0, true}, {2, 0, false}, {0, 1, false}},
    {{1, 2, false}, {3, 3, false}, {4, 3, true}, {0, 2, true}},
    {{1, 1, true}, {5, 0, true}, {4, 0, false}, {2, 1, false}},
    {{3, 2, false}, {5, 3, false}, {0, 3, true}, {2, 2, true}},
    {{3, 1, true}, {1, 0, true}, {0, 0, false}, {4, 1, false}},
};

constexpr int StepI[4] = {0, 1, 0, -1};
constexpr int StepJ[4] = {-1, 0, 1, 0};

} // namespace

FaceEdge faceEdgeOf(int face, int edge) { return FaceEdges[face][edge]; }

Neighbour neighbourAcross(const Cell& cell, int edge) {
    const int last = cellsPerSide(cell.level) - 1;
    const int i = cell.i + StepI[edge], j = cell.j + StepJ[edge];
    if (i >= 0 && i <= last && j >= 0 && j <= last) return {{cell.face, cell.level, i, j}, (edge + 2) % 4};

    const FaceEdge& to = FaceEdges[cell.face][edge];
    const int along = edge % 2 == 0 ? cell.i : cell.j;
    const int a = to.reversed ? last - along : along;
    Cell next{to.face, cell.level, 0, 0};
    switch (to.edge) {
        case 0: next.i = a; next.j = 0; break;
        case 1: next.i = last; next.j = a; break;
        case 2: next.i = a; next.j = last; break;
        default: next.i = 0; next.j = a; break;
    }
    return {next, to.edge};
}

EdgeNeighbours neighboursOf(const Cell& cell) {
    EdgeNeighbours result;
    for (int edge = 0; edge < 4; ++edge) result.across[edge] = neighbourAcross(cell, edge);
    return result;
}

// Walks round the corner: cross the edge that leaves the shared corner, find the corner in the new
// cell (our corner k is the neighbour's corner edge + 1), repeat until back at the start.
CornerCells cellsAround(const Cell& cell, int corner) {
    CornerCells result;
    Cell at = cell;
    int k = corner;
    for (int step = 0; step < 4; ++step) {
        const Neighbour next = neighbourAcross(at, k);
        at = next.cell;
        k = (next.edge + 1) % 4;
        if (at == cell || result.count == 3) break;
        result.cells[result.count] = at;
        result.corner[result.count] = k;
        ++result.count;
    }
    return result;
}

bool atCubeVertex(const Cell& cell, int corner) {
    const int last = cellsPerSide(cell.level) - 1;
    const bool highI = corner == 1 || corner == 2, highJ = corner >= 2;
    return (highI ? cell.i == last : cell.i == 0) && (highJ ? cell.j == last : cell.j == 0);
}

} // namespace derivean::sim

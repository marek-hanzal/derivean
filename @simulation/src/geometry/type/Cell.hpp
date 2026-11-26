#pragma once

namespace derivean::sim {

// One cell of a face quadtree: 0 <= i, j < 2^level. i grows along the face's u axis, j along v.
struct Cell {
    int face{};
    int level{};
    int i{};
    int j{};
};

inline bool operator==(const Cell& a, const Cell& b) { return a.face == b.face && a.level == b.level && a.i == b.i && a.j == b.j; }

// A point on a face in gnomonic coordinates: u, v in [-1, 1] on the face's cube plane.
struct FacePoint {
    int face{};
    double u{};
    double v{};
};

// Corners and edges of a cell run counter-clockwise seen from outside the sphere (all six S2 faces are
// right-handed: u × v is the outward normal). Corner 0 is (u low, v low), 1 (high, low), 2 (high,
// high), 3 (low, high); edge k runs from corner k to corner k + 1, so edge 0 faces -v, 1 +u, 2 +v,
// 3 -u.
//
// The neighbour across one edge, at the same level, and the index of the shared edge in the
// neighbour's numbering. Both cells walk the shared edge in opposite directions: our corner k is the
// neighbour's corner edge + 1 and our corner k + 1 its corner edge.
struct Neighbour {
    Cell cell;
    int edge{};
};

struct EdgeNeighbours {
    Neighbour across[4];
};

// Where one edge of a face leads: the neighbouring face, the shared edge in its numbering, and whether
// the index along the edge (i on edges 0 and 2, j on 1 and 3) runs the other way there. Lets a solver
// map a whole strip of edge cells at once.
struct FaceEdge {
    int face{};
    int edge{};
    bool reversed{};
};

// The cells other than the asking one that meet at one of its corners: three inside a face and on a
// face edge, two at a cube vertex. Listed clockwise round the corner seen from outside, starting with
// the cell across the edge that leaves the corner; `corner` is the shared corner in each cell's own
// numbering.
struct CornerCells {
    int count{};
    Cell cells[3];
    int corner[3]{};
};

} // namespace derivean::sim

// The sphere grid's geometric tables (S3a), sphere fields and rotations. Overlaps and candidates are
// in GeometrySphereOverlap.cpp.
#include "GeometrySphereFixture.hpp"

#include "geometry/fn/Edge.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/type/SphereField.hpp"

#include <algorithm>
#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::geometry_sphere_test;

namespace {

constexpr double Pi = std::numbers::pi;

bool equal(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

double gap(Rotation a, Rotation b) {
    // q and −q are the same rotation.
    const double plus = std::abs(a.w - b.w) + std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
    const double minus = std::abs(a.w + b.w) + std::abs(a.x + b.x) + std::abs(a.y + b.y) + std::abs(a.z + b.z);
    return std::min(plus, minus);
}

} // namespace

namespace derivean::sim::geometry_sphere_test {

Rotation carryAndSpin(Vec3 from, Vec3 to, double spin) {
    const Vec3 axis = cross(from, to);
    const Rotation carry = length(axis) == 0 ? Rotation{} : rotationAbout(axis, centralAngle(from, to));
    return compose(rotationAbout(to, spin), carry);
}

// Counts (6 n² cells, 12 n² edges, 6 n² + 2 vertices, eight with three cells); every table against
// the cell functions it comes from, bit for bit; the edges' orientation round both their cells;
// reach covering every corner; places; areas tiling the sphere.
void checkSphereGrid() {
    for (int level : {0, 1, 3, 5}) {
        const SphereGrid grid = makeSphereGrid(level);
        const int n = grid.side * grid.side;
        expect(grid.cells == 6 * n && grid.cells == sphereCellCount(level), "grid does not have 6 n² cells", level);
        expect(int(grid.edge.size()) == 12 * n && int(grid.vertex.size()) == 6 * n + 2, "grid does not have 12 n² edges and 6 n² + 2 vertices", level);
        double area = 0;
        int places[3] = {};
        for (int c = 0; c < grid.cells; ++c) {
            const Cell cell = cellAt(grid, c);
            expect(indexOf(grid, cell) == c && isValid(cell), "cell index does not round-trip", c);
            expect(grid.area[size_t(c)] == areaOf(cell) && equal(grid.centre[size_t(c)], centreOf(cell)), "cell area or centre is not S1's", c);
            area += grid.area[size_t(c)];
            ++places[int(placeOf(grid, c))];
            for (int k = 0; k < 4; ++k) {
                const SphereEdge& edge = grid.edge[size_t(grid.edges[size_t(c)][size_t(k)])];
                const Neighbour across = neighbourAcross(cell, k);
                expect(neighbourOf(grid, c, k) == indexOf(grid, across.cell), "grid neighbour is not S1's", c);
                expect(edge.left < edge.right && (edge.left == c || edge.right == c), "edge does not join its cell with the lower index left", c);
                const int from = grid.corners[size_t(c)][size_t(k)], to = grid.corners[size_t(c)][size_t((k + 1) % 4)];
                expect(edge.left == c ? edge.from == from && edge.to == to : edge.from == to && edge.to == from,
                       "edge does not run from → to round left and to → from round right", c);
                if (edge.left == c) {
                    const CellEdge shape = edgeOf(cell, k);
                    expect(edge.length == shape.length && equal(edge.normal, shape.normal), "edge length or normal is not S1's", c);
                }
                const SphereVertex& vertex = grid.vertex[size_t(from)];
                expect(std::count(vertex.cells.begin(), vertex.cells.begin() + vertex.count, c) == 1, "vertex does not list its cell", c);
                expect(vertex.count == (atCubeVertex(cell, k) ? 3 : 4), "vertex does not have three cells at cube vertices and four elsewhere", c);
                const Vec3 corner = cornerOf(cell, k);
                expect(length(vertex.point - corner) < 1e-15, "vertex point is not the cell's corner", length(vertex.point - corner));
                expect(length(corner - grid.centre[size_t(c)]) <= grid.reach[size_t(c)] + 1e-15, "reach does not cover a corner", c);
            }
        }
        int cubeVertices = 0;
        for (const SphereVertex& vertex : grid.vertex) cubeVertices += vertex.count == 3;
        expect(cubeVertices == 8, "the grid does not have eight cube vertices", cubeVertices);
        const double sphere = 4 * Pi * PlanetRadius * PlanetRadius;
        expect(near(area / sphere, 1, 1e-12), "cell areas do not tile the sphere", area / sphere - 1);
        if (level >= 2) {
            const int inner = grid.side - 2;
            expect(places[0] == 6 * inner * inner && places[1] == 24 * inner && places[2] == 24, "places are not counted as the faces give them", places[1]);
        }
        const SphereField<double> field{level, std::vector<double>(size_t(sphereCellCount(level)), 1.0)};
        expect(field.level == grid.level && int(field.value.size()) == grid.cells, "a sphere field does not hold one value per cell", level);
    }
}

// The rotation algebra: identity, order of composition, inverse, matrix (a few ulp: the worst of 1e5
// random pairs is 3e-15); 2π after N steps; tiny angles keeping their relative precision; the Euler
// vector's velocity ω × x.
void checkRotation() {
    Random random{7};
    const Vec3 v{0.48, -0.6, 0.64};
    expect(equal(rotate(Rotation{}, v), v) && gap(rotationOf({}, 1e15), Rotation{}) == 0, "identity is not exact");
    expect(length(rotate(rotationAbout({0, 0, 2}, Pi / 2), {1, 0, 0}) - Vec3{0, 1, 0}) < 1e-15, "a quarter turn about z does not take x to y");
    for (int n = 0; n < 500; ++n) {
        const Rotation a = rotationAbout(random.direction(), 2 * Pi * random.next()), b = rotationAbout(random.direction(), 2 * Pi * random.next());
        const Vec3 x = random.direction();
        expect(gap(compose(inverse(a), a), Rotation{}) < 2e-15 && gap(compose(a, inverse(a)), Rotation{}) < 2e-15, "a rotation and its inverse do not compose to the identity");
        expect(length(rotate(inverse(a), rotate(a, x)) - x) < 1e-14, "the inverse does not undo the rotation");
        expect(length(rotate(compose(b, a), x) - rotate(b, rotate(a, x))) < 1e-14, "composition is not first, then later");
        const RotationMatrix m = matrixOf(a);
        expect(length(rotate(m, x) - rotate(a, x)) < 1e-14, "the matrix does not rotate as the quaternion");
        for (int r = 0; r < 3; ++r)
            for (int s = 0; s < 3; ++s) expect(near(dot(m.row[r], m.row[s]), r == s, 1e-14), "the matrix is not orthonormal");
        expect(near(dot(cross(m.row[0], m.row[1]), m.row[2]), 1, 1e-14), "the matrix is not a proper rotation");
        expect(angleOf(a) >= 0 && angleOf(a) <= Pi, "angle is not in [0, π]", angleOf(a));
    }
    // A full turn in N steps of an Euler vector comes back to the start within 1e-12.
    for (int steps : {1000, 100000}) {
        const Vec3 axis = random.direction(), euler = axis * 3e-16;   // rad/s, about 1.7°/Myr
        const double dt = 2 * Pi / (3e-16 * steps);
        const Rotation step = rotationOf(euler, dt);
        Rotation q;
        for (int s = 0; s < steps; ++s) q = renormalized(compose(step, q));
        expect(gap(q, Rotation{}) < 1e-12, "a full turn in steps does not return to the identity", gap(q, Rotation{}));
        for (int t = 0; t < 10; ++t) {
            const Vec3 x = random.direction();
            expect(length(rotate(q, x) - x) < 1e-12, "a full turn in steps does not bring a point back", length(rotate(q, x) - x));
        }
    }
    // Tiny angles keep full relative precision, and a million steps of 1e-9 rad are one step of 1e-3:
    // the vector part gathers about N · ulp(5e-4) / 2 of rounding, at most 5e-14 (over 100 axes the
    // median is 1e-16, the worst 1.1e-14).
    for (double angle : {1e-9, 1e-12, 1e-15, 1e-20}) {
        const Vec3 axis = random.direction(), euler = axis * 2e-16;
        const Rotation q = rotationOf(euler, angle / 2e-16);
        expect(near(angleOf(q) / angle, 1, 1e-15), "a tiny rotation loses its angle", angleOf(q) / angle - 1);
        expect(near(q.x / (axis.x * angle / 2), 1, 1e-15), "a tiny rotation's vector part is not ω dt / 2", q.x / (axis.x * angle / 2) - 1);
    }
    const Vec3 axis = random.direction();
    const Rotation step = rotationOf(axis * 1e-15, 1e6);   // 1e-9 rad
    Rotation q;
    for (int s = 0; s < 1000000; ++s) q = renormalized(compose(step, q));
    const Rotation once = rotationAbout(axis, 1e-3);
    expect(gap(q, once) < 5e-14, "a million steps of 1e-9 rad are not one step of 1e-3", gap(q, once));
    // The velocity of an Euler vector is ω × x: over a short time the move is (ω × x) t to second order
    // in the angle.
    const Vec3 euler = random.direction() * 2e-15, x = random.direction();
    const double t = 1e-6 / 2e-15;
    const Vec3 moved = rotate(rotationOf(euler, t), x) - x, expected = cross(euler, x) * t;
    expect(length(moved - expected) < 1e-6 * length(expected), "an Euler vector does not move a point by ω × x", length(moved - expected) / length(expected));
}

} // namespace derivean::sim::geometry_sphere_test

void testGeometrySphere() {
    using namespace derivean::sim::geometry_sphere_test;
    checkSphereGrid();
    checkRotation();
    checkOverlap();
    checkCandidates();
}

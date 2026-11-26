#include "geometry/fn/Overlap.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SpherePolygon.hpp"

#include <array>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// Polygons within this chord of their centre (about 20°) project with every corner of either well in
// front of the tangent plane's hemisphere.
constexpr double LargestReach = 0.35;

// Apart is decided by chords on the unit sphere with this margin over rounding.
constexpr double ApartMargin = 1e-12;

// The tangent plane at a unit direction: an orthonormal pair with first × second = centre.
struct Plane {
    Vec3 centre, first, second;
};

Plane planeAt(Vec3 centre) {
    const Vec3 first = normalized(cross(std::abs(centre.z) < 0.9 ? Vec3{0, 0, 1} : Vec3{1, 0, 0}, centre));
    return {centre, first, cross(centre, first)};
}

// Gnomonic plane coordinates: the point p / (p · centre) − centre in (first, second).
struct Flat {
    int count{};
    std::array<double, 16> x{}, y{};
};

Flat project(const Plane& plane, const SpherePolygon& polygon) {
    assert(polygon.count >= 3 && polygon.count <= 8);
    Flat flat;
    flat.count = polygon.count;
    for (int k = 0; k < polygon.count; ++k) {
        const Vec3 p = polygon.corner[size_t(k)];
        const double height = dot(p, plane.centre);
        assert(height > 0.3);
        flat.x[size_t(k)] = dot(p, plane.first) / height;
        flat.y[size_t(k)] = dot(p, plane.second) / height;
    }
    return flat;
}

// Which side of the line through a and b the point is on (positive: left of a → b). The two products
// are separate statements so that clang (whose default contracts only within an expression) leaves an
// endpoint of the line exactly on it. A compiler that fuses across statements only moves such a point
// by rounding: it comes back as a cut point on the line, and the area is unchanged.
double sideOf(double ax, double ay, double bx, double by, double px, double py) {
    const double along = (bx - ax) * (py - ay);
    const double across = (by - ay) * (px - ax);
    return along - across;
}

// Sutherland & Hodgman (1974): keeps the part of `subject` left of the directed line a → b. Each clip
// adds at most one corner, so eight corners clipped by eight edges need at most sixteen.
void clip(Flat& subject, double ax, double ay, double bx, double by) {
    Flat kept;
    const int n = subject.count;
    if (n == 0) return;
    double previousSide = sideOf(ax, ay, bx, by, subject.x[size_t(n - 1)], subject.y[size_t(n - 1)]);
    for (int k = 0; k < n; ++k) {
        const int before = (k + n - 1) % n;
        const double side = sideOf(ax, ay, bx, by, subject.x[size_t(k)], subject.y[size_t(k)]);
        assert(kept.count < 16);
        if ((side >= 0) != (previousSide >= 0)) {
            const double t = previousSide / (previousSide - side);
            const size_t at = size_t(kept.count++);
            kept.x[at] = subject.x[size_t(before)] + t * (subject.x[size_t(k)] - subject.x[size_t(before)]);
            kept.y[at] = subject.y[size_t(before)] + t * (subject.y[size_t(k)] - subject.y[size_t(before)]);
        }
        if (side >= 0) {
            assert(kept.count < 16);
            const size_t at = size_t(kept.count++);
            kept.x[at] = subject.x[size_t(k)];
            kept.y[at] = subject.y[size_t(k)];
        }
        previousSide = side;
    }
    subject = kept;
}

// Σ of the fan's excesses, tan(E/2) = |P1 · (P2 × P3)| / (|P1||P2||P3| + (P1·P2)|P3| + (P2·P3)|P1| +
// (P3·P1)|P2|) for the unnormalized corners P = (x, y, 1) in (first, second, centre). P1 · (P2 × P3)
// is the plane cross product of the corner differences.
double excessOf(const Flat& flat) {
    if (flat.count < 3) return 0;
    std::array<double, 16> norm{};
    for (int k = 0; k < flat.count; ++k) norm[size_t(k)] = std::sqrt(1 + flat.x[size_t(k)] * flat.x[size_t(k)] + flat.y[size_t(k)] * flat.y[size_t(k)]);
    const double x0 = flat.x[0], y0 = flat.y[0], n0 = norm[0];
    double excess = 0;
    for (int k = 1; k + 1 < flat.count; ++k) {
        const size_t b = size_t(k), c = size_t(k + 1);
        const double triple = (flat.x[b] - x0) * (flat.y[c] - y0) - (flat.y[b] - y0) * (flat.x[c] - x0);
        const double ab = 1 + x0 * flat.x[b] + y0 * flat.y[b];
        const double bc = 1 + flat.x[b] * flat.x[c] + flat.y[b] * flat.y[c];
        const double ca = 1 + flat.x[c] * x0 + flat.y[c] * y0;
        const double denominator = n0 * norm[b] * norm[c] + ab * norm[c] + bc * n0 + ca * norm[b];
        excess += 2 * std::atan2(std::abs(triple), denominator);
    }
    return excess;
}

} // namespace

double areaOf(const SpherePolygon& polygon) {
    const PolygonCap cap = capOf(polygon);
    assert(cap.reach < LargestReach);
    return excessOf(project(planeAt(cap.centre), polygon)) * PlanetRadius * PlanetRadius;
}

double overlapArea(const SpherePolygon& a, const SpherePolygon& b) {
    const PolygonCap capA = capOf(a), capB = capOf(b);
    assert(capA.reach < LargestReach && capB.reach < LargestReach);
    if (length(capA.centre - capB.centre) > capA.reach + capB.reach + ApartMargin) return 0;
    const Plane plane = planeAt(capA.centre);
    Flat piece = project(plane, a);
    const Flat edge = project(plane, b);
    for (int k = 0; k < edge.count && piece.count > 0; ++k) {
        const size_t next = size_t((k + 1) % edge.count);
        clip(piece, edge.x[size_t(k)], edge.y[size_t(k)], edge.x[next], edge.y[next]);
    }
    return excessOf(piece) * PlanetRadius * PlanetRadius;
}

} // namespace derivean::sim

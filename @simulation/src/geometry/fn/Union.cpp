#include "geometry/fn/Union.hpp"

#include "geometry/constant/Sphere.hpp"

#include <clipper2/clipper.h>

#include <cassert>
#include <cmath>
#include <utility>
#include <cstdio>
#include <cstdlib>

namespace derivean::sim {
namespace {

constexpr double Scale = 1e12;   // integer units per plane unit

// Signed spherical excess of the fan from the tangent point (0, 0, 1) over a closed plane contour:
// tan(E/2) = P0 · (Pa × Pb) / (|Pa||Pb| + P0·Pa |Pb| + Pa·Pb + P0·Pb |Pa|) with P = (x, y, 1) and |P0| = 1,
// where P0 · (Pa × Pb) is the plane cross product xa yb − ya xb. `corner(k)` gives corner k's (x, y);
// each corner's |P| is taken once and carried to the next triangle.
template <class Corner>
double fanExcess(size_t count, Corner corner) {
    if (count < 3) return 0;   // no area, and corner(0) may not exist
    double excess = 0;
    const auto [x0, y0] = corner(0);
    const double n0 = std::sqrt(1 + x0 * x0 + y0 * y0);
    double xa = x0, ya = y0, na = n0;
    for (size_t k = 0; k < count; ++k) {
        const bool last = k + 1 == count;
        const auto [xb, yb] = last ? std::pair{x0, y0} : corner(k + 1);
        const double nb = last ? n0 : std::sqrt(1 + xb * xb + yb * yb);
        const double triple = xa * yb - ya * xb;
        const double denominator = na * nb + nb + (1 + xa * xb + ya * yb) + na;
        excess += 2 * std::atan2(triple, denominator);
        xa = xb, ya = yb, na = nb;
    }
    return excess;
}

double contourArea(const Clipper2Lib::Path64& path) {
    return fanExcess(path.size(), [&](size_t k) { return std::pair{double(path[k].x) / Scale, double(path[k].y) / Scale}; }) * PlanetRadius * PlanetRadius;
}

double side(double ax, double ay, double bx, double by, double px, double py) {
    const double along = (bx - ax) * (py - ay);
    const double across = (by - ay) * (px - ax);
    return along - across;
}

// Walks the tree: outer contours positive, holes negative, after the projection back onto the sphere.
// A degenerate sliver of next to no area has the sign of the rounding of its fan (about 1e-16 of a
// cell's area per triangle, 1e-6 m² at level 7; one seen at -2e-8 m²), so the sign is checked only
// above 1e-14 R² per vertex (0.4 m²), far below anything a threshold of half a cell can feel.
void addTree(const Clipper2Lib::PolyPath64& node, UnionArea& result) {
    for (const auto& child : node) {
        const double area = contourArea(child->Polygon());
        const bool hole = child->IsHole();
        const double noise = 1e-14 * PlanetRadius * PlanetRadius * double(child->Polygon().size());
        if (std::abs(area) > noise && ((hole && area > 0) || (!hole && area < 0))) {
            std::fprintf(stderr, "geometry: a contour of a union has the wrong sign (%g m², hole %d)\n", area, int(hole));
            std::abort();
        }
        result.area += area;
        ++(hole ? result.holes : result.outers);
        addTree(*child, result);
    }
}

Clipper2Lib::Path64 pathOf(const PlanePolygon& polygon) {
    Clipper2Lib::Path64 path;
    path.reserve(size_t(polygon.count));
    for (int k = 0; k < polygon.count; ++k)
        path.emplace_back(int64_t(std::llround(polygon.x[size_t(k)] * Scale)), int64_t(std::llround(polygon.y[size_t(k)] * Scale)));
    return path;
}

} // namespace

TangentPlane tangentPlaneAt(Vec3 centre) {
    const Vec3 first = normalized(cross(std::abs(centre.z) < 0.9 ? Vec3{0, 0, 1} : Vec3{1, 0, 0}, centre));
    return {centre, first, cross(centre, first)};
}

PlanePolygon projected(const TangentPlane& plane, const SpherePolygon& polygon) {
    PlanePolygon flat;
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

PlanePolygon clipped(PlanePolygon subject, const PlanePolygon& clip) {
    for (int e = 0; e < clip.count && subject.count > 0; ++e) {
        const size_t a = size_t(e), b = size_t((e + 1) % clip.count);
        PlanePolygon kept;
        const int n = subject.count;
        double previous = side(clip.x[a], clip.y[a], clip.x[b], clip.y[b], subject.x[size_t(n - 1)], subject.y[size_t(n - 1)]);
        for (size_t at = 0, before = size_t(n - 1); at < size_t(n); before = at++) {
            const double now = side(clip.x[a], clip.y[a], clip.x[b], clip.y[b], subject.x[at], subject.y[at]);
            if ((now >= 0) != (previous >= 0)) {
                const double t = previous / (previous - now);
                assert(kept.count < 16);
                kept.x[size_t(kept.count)] = subject.x[before] + t * (subject.x[at] - subject.x[before]);
                kept.y[size_t(kept.count++)] = subject.y[before] + t * (subject.y[at] - subject.y[before]);
            }
            if (now >= 0) {
                assert(kept.count < 16);
                kept.x[size_t(kept.count)] = subject.x[at];
                kept.y[size_t(kept.count++)] = subject.y[at];
            }
            previous = now;
        }
        subject = kept;
    }
    if (subject.count < 3) subject.count = 0;
    return subject;
}

double sphericalArea(const PlanePolygon& polygon) {
    if (polygon.count < 3) return 0;
    return fanExcess(size_t(polygon.count), [&](size_t k) { return std::pair{polygon.x[k], polygon.y[k]}; }) * PlanetRadius * PlanetRadius;
}

UnionArea unionArea(const std::vector<PlanePolygon>& polygons) {
    UnionArea result;
    Clipper2Lib::Paths64 subjects;
    // A clip along a shared edge leaves slivers of no area (a polygon folded onto one segment). Clipper2
    // 1.5.4 then nests a whole neighbouring part as a hole of the union (seen on the level-7 history), so
    // duplicate and collinear corners are trimmed first and what has no area left is dropped.
    for (const PlanePolygon& polygon : polygons) {
        if (polygon.count < 3) continue;
        Clipper2Lib::Path64 path = Clipper2Lib::TrimCollinear(pathOf(polygon), false);
        if (path.size() >= 3) subjects.push_back(std::move(path));
    }
    if (subjects.empty()) return result;
    Clipper2Lib::Clipper64 clipper;
    clipper.AddSubject(subjects);
    Clipper2Lib::PolyTree64 tree;
    clipper.Execute(Clipper2Lib::ClipType::Union, Clipper2Lib::FillRule::NonZero, tree);
    addTree(tree, result);
    return result;
}

double coveredBy(const PlanePolygon& a, const std::vector<PlanePolygon>& others) {
    if (others.empty() || a.count < 3) return 0;
    std::vector<PlanePolygon> all(others);
    const double alone = unionArea(others).area;
    all.push_back(a);
    // The same integer corners in all three areas, so the difference cancels their rounding.
    std::vector<PlanePolygon> one{a};
    return unionArea(one).area + alone - unionArea(all).area;
}

} // namespace derivean::sim

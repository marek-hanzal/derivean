// Frames, vector transforms, parallel transport, distances and the tangent frame's error budget.
#include "GeometryFixture.hpp"

#include "geometry/fn/Frame.hpp"
#include "geometry/fn/LocalFrame.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Neighbour.hpp"
#include "geometry/fn/Projection.hpp"

#include <numbers>
#include <random>

namespace derivean::sim::geometry_test {

namespace {

constexpr double Pi = std::numbers::pi;

// Where the frames point: geographic east and north at reference places and on the pole, and the face
// grid's first axis at each face centre (east on faces 0, 1, 5, north on 2, south on 3 and 4).
void checkOrientation() {
    const SurfaceFrame equator = geographicFrame({1, 0, 0});
    expect(same(equator.first, {0, 1, 0}, 1e-15) && same(equator.second, {0, 0, 1}, 1e-15), "east and north at 0° 0° are wrong");
    const SurfaceFrame east90 = geographicFrame({0, 1, 0});
    expect(same(east90.first, {-1, 0, 0}, 1e-15) && same(east90.second, {0, 0, 1}, 1e-15), "east and north at 0° 90° E are wrong");
    const SurfaceFrame pole = geographicFrame({0, 0, 1});
    expect(same(pole.first, {0, 1, 0}, 1e-15) && same(pole.second, {-1, 0, 0}, 1e-15), "the north pole frame is not the longitude-0 limit");
    const int firstIs[FaceCount] = {0, 0, 1, 2, 2, 0};   // 0 east, 1 north, 2 south
    for (int face = 0; face < FaceCount; ++face) {
        const SurfaceFrame grid = cellFrame({face, 0, 0, 0}), geo = geographicFrame(grid.up);
        const Vec3 expected = firstIs[face] == 0 ? geo.first : firstIs[face] == 1 ? geo.second : -geo.second;
        expect(same(grid.first, expected, 1e-14), "face grid frame points the wrong way", face);
        expect(same(cross(grid.first, grid.second), grid.up, 1e-15), "face grid frame is not right-handed", face);
    }
}

// The same vector at the same point given in two faces' frames on their shared edge and in the
// geographic frame; then back and forth across edges and corners.
void checkTransforms() {
    for (double v : {-0.9, 0.0, 0.42}) {
        const SurfaceFrame a = faceFrame({0, 1, v}), b = faceFrame({1, -1, v});
        expect(same(a.up, b.up, 1e-15), "face frames on a shared edge disagree on up");
        const TangentVector wind{3.0, -4.5};
        expect(same(vectorOf(reframe(wind, a, b), b), vectorOf(wind, a), 1e-14), "vector changed between face frames");
        const TangentVector back = reframe(reframe(wind, a, b), b, a);
        expect(near(back.first, wind.first, 1e-14) && near(back.second, wind.second, 1e-14), "reframing is not reversible");
    }
    for (int level : {0, 3, RegionLevel, SiteLevel, GroundCellLevel})
        for (const Cell& cell : samplesAt(level)) {
            const TangentVector wind{12.0, -7.0};
            for (int edge = 0; edge < 4; ++edge) {   // edge k and corner k
                const Cell next = neighbourAcross(cell, edge).cell;
                const TangentVector back = carry(carry(wind, cell, next), next, cell);
                expect(near(back.first, wind.first, 1e-12) && near(back.second, wind.second, 1e-12), "carry across an edge and back is not identity");
                const Vec3 direct = transport(vectorOf(wind, cellFrame(cell)), centreOf(cell), centreOf(next));
                expect(same(vectorOf(carry(wind, cell, next), cellFrame(next)), direct, 1e-12), "carry is not the transported vector");
                const CornerCells around = cellsAround(cell, edge);
                for (int k = 0; k < around.count; ++k) {
                    const TangentVector round = carry(carry(wind, cell, around.cells[k]), around.cells[k], cell);
                    expect(near(round.first, wind.first, 1e-12) && near(round.second, wind.second, 1e-12), "carry across a corner and back is not identity");
                }
            }
        }
}

// A fixed sequence of unit directions, the same on every platform (the standard fixes mt19937_64's
// output; the conversion to [0, 1) is done here, not by a distribution).
struct Directions {
    std::mt19937_64 engine;
    double unit() { return double(engine() >> 11) * 0x1p-53; }
    Vec3 next() { return normalized(Vec3{2 * unit() - 1, 2 * unit() - 1, 2 * unit() - 1}); }
};

// Transport stays finite and reversible up to and including antipodes: Codex's two cases (1e-9 short of
// the antipode, two cells at the centres of opposite faces), the antipode itself, pairs off the axes at
// π − ε down to ε = 1e-15, and cells carried to the cells round their antipode and back.
void checkAntipodes() {
    const Vec3 v{0, 1, 0};
    for (Vec3 to : {Vec3{-1, 1e-9, 0}, Vec3{-1, 0, 0}, Vec3{-1, 2e-12, 0}, Vec3{-1, 5e-13, 3e-13}, Vec3{-1, 1e-3, 0}, Vec3{-0.3, 1, 0}}) {
        const Vec3 there = transport(v, {1, 0, 0}, to), back = transport(there, to, {1, 0, 0});
        expect(std::isfinite(there.x) && std::isfinite(there.y) && std::isfinite(there.z), "transport near the antipode is not finite");
        expect(near(length(there), 1, 1e-14) && std::abs(dot(there, normalized(to))) < 1e-9, "transport near the antipode is not a tangent rotation");
        expect(same(back, v, 1e-14), "transport near the antipode is not reversible");
    }
    const Vec3 reached = transport(Vec3{1, 0, 0}, Vec3{1, 0, 0}, Vec3{-1, 1e-9, 0});
    expect(same(reached, Vec3{-1, 1e-9, 0}, 1e-12), "transport 1e-9 short of the antipode does not reach the target");
    const Cell a{0, 30, 536870912, 536870912}, b{3, 30, 536870913, 536870912};
    const FrameRotation turn = carryRotation(a, b);
    expect(std::isfinite(turn.cosine) && std::isfinite(turn.sine) && near(std::hypot(turn.cosine, turn.sine), 1, 1e-14), "carry between opposite faces is not finite");
    const TangentVector wind{12.0, -7.0}, round = carry(carry(wind, a, b), b, a);
    expect(near(round.first, wind.first, 1e-12) && near(round.second, wind.second, 1e-12), "carry between opposite faces is not reversible");
    Directions random;
    for (int sample = 0; sample < 2000; ++sample) {
        const Vec3 from = random.next(), side = normalized(cross(from, random.next()));
        const Vec3 tangent = normalized(cross(side, from) + side * (2 * random.unit() - 1));
        for (double epsilon : {1e-3, 1e-9, 1e-11, 1e-12, 1e-13, 1e-15, 0.0}) {
            const Vec3 to = normalized(from * -std::cos(epsilon) + side * std::sin(epsilon));
            const Vec3 there = transport(tangent, from, to), back = transport(there, to, from);
            expect(std::isfinite(there.x) && std::isfinite(there.y) && std::isfinite(there.z), "transport of a random pair near the antipode is not finite");
            expect(near(length(there), 1, 1e-14) && std::abs(dot(there, to)) < 1e-14, "transport of a random pair near the antipode is not a tangent rotation",
                   std::abs(dot(there, to)));
            expect(same(back, tangent, 1e-13), "transport of a random pair near the antipode is not reversible", length(back - tangent));
            expect(same(transport(from, from, to), to, 1e-14), "transport of a random pair near the antipode does not reach the target");
        }
        // Beyond a right angle the rotation is about from × to: the same as two steps through the arc's
        // midpoint, each within a right angle. Rounding `to` by 1e-16 tilts the great circle through
        // nearly antipodal points by 1e-16 / sin θ, which the midpoint pins, so the bound grows with it.
        for (double angle : {1.6, 2.5, 3.0, 3.14}) {
            const Vec3 to = normalized(from * std::cos(angle) + side * std::sin(angle)), middle = normalized(from * std::cos(angle / 2) + side * std::sin(angle / 2));
            expect(same(transport(tangent, from, to), transport(transport(tangent, from, middle), middle, to), 4e-15 / std::sin(angle)), "obtuse transport is not about from × to",
                   length(transport(tangent, from, to) - transport(transport(tangent, from, middle), middle, to)));
        }
    }
    for (double tiny : {1e-160, 3e-162, 1e-300}) {   // a × (a + b) far below the square root of the smallest double
        const Vec3 there = transport(Vec3{0, 0, 1}, {1, 0, 0}, {-1, tiny, 0}), back = transport(there, {-1, tiny, 0}, {1, 0, 0});
        expect(near(length(there), 1, 1e-15) && same(back, Vec3{0, 0, 1}, 1e-15), "transport a denormal angle short of the antipode is not a rotation");
    }
    for (int level : {0, 3, RegionLevel, SiteLevel, GroundCellLevel, MaxLevel})
        for (const Cell& cell : samplesAt(level)) {
            const Cell opposite = cellOf(-centreOf(cell), level);
            for (int edge = 0; edge < 4; ++edge)
                for (const Cell& far : {opposite, neighbourAcross(opposite, edge).cell}) {
                    const TangentVector back = carry(carry(wind, cell, far), far, cell);
                    expect(near(back.first, wind.first, 1e-12) && near(back.second, wind.second, 1e-12), "carry to the antipodal cells and back is not identity");
                }
        }
}

// Parallel transport round a closed geodesic loop turns the vector by the loop's solid angle, in the
// sense of travel: the octant (π/2), the three cells round a cube vertex, four cells round an inner
// corner at Site level.
double holonomy(const std::vector<Vec3>& loop) {
    const SurfaceFrame frame = geographicFrame(loop[0]);
    Vec3 vector = frame.first;
    for (size_t k = 0; k < loop.size(); ++k) vector = transport(vector, loop[k], loop[(k + 1) % loop.size()]);
    return std::atan2(dot(cross(frame.first, vector), frame.up), dot(frame.first, vector));
}

double excessOf(const std::vector<Vec3>& loop) {
    double sum = 0;
    for (size_t k = 1; k + 1 < loop.size(); ++k) sum += sphericalExcess(loop[0], loop[k], loop[k + 1]);
    return sum;
}

void checkHolonomy() {
    const std::vector<Vec3> octant{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    expect(near(holonomy(octant), Pi / 2, 1e-14), "octant loop does not turn by π/2", holonomy(octant));
    expect(same(transport(Vec3{0, 3, 4}, Vec3{0, 3, 4}, Vec3{1, 0, 0}), Vec3{5, 0, 0}, 1e-14), "transport is not a rotation onto the target");
    checkAntipodes();
    for (const auto& [cell, corner] : {std::pair{Cell{0, 3, 7, 7}, 2}, std::pair{Cell{2, SiteLevel, 0, 0}, 0}, std::pair{Cell{4, SiteLevel, 300, 4095}, 3},
                                       std::pair{Cell{1, SiteLevel, 1234, 777}, 1}}) {
        const CornerCells around = cellsAround(cell, corner);
        std::vector<Vec3> loop{centreOf(cell)};
        for (int k = around.count - 1; k >= 0; --k) loop.push_back(centreOf(around.cells[k]));   // counter-clockwise
        expect(excessOf(loop) > 0 && near(holonomy(loop) / excessOf(loop), 1, 1e-6), "loop does not turn by its solid angle", holonomy(loop) / excessOf(loop) - 1);
    }
}

// Great-circle distances against haversine (Sinnott 1984, an independent formula) and exact arcs.
void checkDistances() {
    auto haversine = [](LatLon a, LatLon b) {
        const double h = std::pow(std::sin((b.latitude - a.latitude) / 2), 2) +
                         std::cos(a.latitude) * std::cos(b.latitude) * std::pow(std::sin((b.longitude - a.longitude) / 2), 2);
        return 2 * PlanetRadius * std::asin(std::sqrt(h));
    };
    const double radian = Pi / 180;
    const LatLon places[][2] = {{{51.5074 * radian, -0.1278 * radian}, {40.7128 * radian, -74.0060 * radian}},
                                {{-33.8688 * radian, 151.2093 * radian}, {-33.4489 * radian, -70.6693 * radian}},
                                {{50.0755 * radian, 14.4378 * radian}, {50.0755 * radian + 1e-7, 14.4378 * radian}}};
    for (const auto& pair : places)
        expect(near(distanceBetween(directionFromLatLon(pair[0]), directionFromLatLon(pair[1])) / haversine(pair[0], pair[1]), 1, 1e-9),
               "distance disagrees with haversine");
    expect(near(distanceBetween({1, 0, 0}, {0, 0, 1}), Pi / 2 * PlanetRadius, 1e-6), "quarter circle is not πR/2");
    expect(near(distanceBetween({1, 0, 0}, {-1, 1e-12, 0}), (Pi - 1e-12) * PlanetRadius, 1e-4), "near-antipode distance is off");
    const LatLon back = latLonOf(directionFromLatLon(places[2][0]));
    expect(near(back.latitude, places[2][0].latitude, 1e-15) && near(back.longitude, places[2][0].longitude, 1e-15), "latitude and longitude do not return");
}

// The tangent frame's error budget: 7.85 cm and 0.009° at 1 km, 785 m at 100 km, and the same numbers
// from placing a planar point through the frame and measuring it on the sphere.
void checkTangentBudget() {
    const TangentError km = tangentErrorAt(1000, PlanetRadius);
    expect(near(km.height, 0.0785, 5e-5), "height error at 1 km is not 7.85 cm", km.height);
    expect(near(km.tilt * 180 / Pi, 0.009, 5e-5), "tilt at 1 km is not 0.009°", km.tilt * 180 / Pi);
    expect(near(tangentErrorAt(100e3, PlanetRadius).height, 785, 1), "height error at 100 km is not 785 m");
    expect(near(tangentErrorAt(100e3, PlanetRadius).distance, 8.2, 0.05), "distance error at 100 km is not 8.2 m");
    expect(near(tangentReach(km.height, PlanetRadius), 1000, 1e-6), "reach does not invert the budget");
    for (LatLon place : {LatLon{0.87, 0.25}, LatLon{-0.3, 2.9}, LatLon{Pi / 2, 0}}) {
        const double height = 350;   // the anchor's radial height
        const LocalFrame frame = anchorAt(ecefOf(directionFromLatLon(place), height));
        for (Vec3 local : {Vec3{1000, 0, 0}, Vec3{-600, 800, 0}, Vec3{30e3, -40e3, 0}}) {
            const double d = std::hypot(local.x, local.y);
            const TangentError budget = tangentErrorAt(d, PlanetRadius + height);
            const Vec3 ecef = ecefOfLocal(frame, local);
            expect(near(heightOf(ecef) - height, budget.height, 1e-7), "budget height disagrees with the frame", heightOf(ecef) - height - budget.height);
            expect(near(centralAngle(frame.axes.up, ecef), budget.tilt, 1e-13), "budget tilt disagrees with the frame");
            const double arc = centralAngle(frame.origin, ecef) * (PlanetRadius + height);
            expect(near(d - arc, budget.distance, 1e-6), "budget distance disagrees with the frame", d - arc - budget.distance);
            expect(same(localOf(frame, ecef), local, 1e-8), "local -> ECEF -> local does not return");
        }
    }
}

} // namespace

void checkFrames() {
    checkOrientation();
    checkTransforms();
    checkHolonomy();
    checkDistances();
    checkTangentBudget();
}

} // namespace derivean::sim::geometry_test

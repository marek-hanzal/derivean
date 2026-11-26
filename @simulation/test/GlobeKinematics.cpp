// Plate kinematics on the sphere (S3-design.md §3, §11 test 6): v = R ω × x, the sign of convergence
// on two plates closing and parting, no net rotation over the applied vectors after damping (Codex V5)
// and at least one substep (Codex V4).
#include "GlobeFixture.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "tectonics/fn/Euler.hpp"

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace derivean::sim::globe_test {

void checkKinematics() {
    Random random{42};
    // The surface velocity of an Euler vector is the derivative of its rotation.
    for (int trial = 0; trial < 200; ++trial) {
        const Vec3 euler = random.direction() * (1e-8 * (0.1 + random.next()));
        const Vec3 x = random.direction();
        const double dt = 1e3;
        const Vec3 measured = (rotate(rotationOf(euler, dt), x) - rotate(rotationOf(euler, -dt), x)) * (PlanetRadius / (2 * dt));
        const Vec3 expected = velocityAt(euler, x);
        expect(length(measured - expected) <= 1e-6 * length(expected) + 1e-15, "v = R ω × x", length(measured - expected));
    }

    // Two plates on either side of the meridian of longitude 0 at the equator, p to the west of q. The
    // normal from p towards q points east. Turning p east and q west about the pole closes the contact.
    const Vec3 x{1, 0, 0}, east{0, 1, 0}, pole{0, 0, 1};
    const double rate = 0.05 / PlanetRadius;   // 50 mm/yr at the equator
    const Closing closing = closingAt(pole * rate, pole * -rate, x, east);
    expect(closing.convergence > 0.099 && closing.convergence < 0.101, "plates turning towards each other close", closing.convergence);
    expect(std::abs(closing.shear) < 1e-12, "head-on plates have no shear", closing.shear);
    const Closing parting = closingAt(pole * -rate, pole * rate, x, east);
    expect(parting.convergence < -0.099, "plates turning apart open", parting.convergence);
    // Along the contact: a pole on the y axis moves the point along -z, parallel to the contact.
    const Closing sliding = closingAt(Vec3{0, 1, 0} * rate, Vec3{}, x, east);
    expect(std::abs(sliding.convergence) < 1e-12 && std::abs(std::abs(sliding.shear) - 0.05) < 1e-9, "transform motion is shear only", sliding.shear);

    // NNR after damping: one plate fully jammed, the epoch's ramp under way.
    Globe globe = makeGlobe(7, 5, PlateSpeeds::Earth, TectonicsStart);
    enterGlobeEpoch(globe);
    expect(globe.epoch.plates.size() >= 9 && globe.epoch.plates.size() <= 25, "9 to 25 plates", double(globe.epoch.plates.size()));
    for (const double into : {3.0, 6.0, 40.0}) {
        globe.time = globe.epoch.start + into * Myr;
        globe.motion[0].resistance = 1e18;
        globe.motion[1].resistance = 3e15;
        applyKinematics(globe, PlanetStep);
        const NetRotation net = netRotationOf(globe);
        expect(net.scale > 0, "plates move", net.scale);
        expect(length(net.net) <= 1e-12 * net.scale, "no net rotation after damping", length(net.net) / net.scale);
        expect(length(globe.applied[0]) < length(globe.epoch.plates[0].euler), "a jammed plate turns slower", length(globe.applied[0]));
    }

    // RMS speed over the crust from the applied vectors in the middle of an epoch: Earth's speeds land
    // near Earth's 43.9 mm/yr (the seed-level check against Earth is terra globe's).
    globe.time = 0.5 * (globe.epoch.start + globe.epoch.end);
    for (GlobeMotion& motion : globe.motion) motion.resistance = 0;
    applyKinematics(globe, PlanetStep);
    double square = 0, area = 0;
    for (const CrustPiece& piece : globe.pieces) {
        const Vec3 v = velocityAt(globe.applied[piece.plate], centreOf(*globe.grid, piece));
        square += piece.area * dot(v, v);
        area += piece.area;
    }
    const double rms = std::sqrt(square / area);
    expect(rms > 0.025 && rms < 0.075, "Earth's speeds give tens of mm/yr", rms);

    // Every plate with the same vector: nothing moves relative to anything, yet one substep.
    std::vector<Vec3> same(globe.epoch.plates.size(), Vec3{0, 0, 1e-9});
    std::vector<Inertia> inertia(same.size());
    for (const CrustPiece& piece : globe.pieces) addInertia(inertia[piece.plate], centreOf(*globe.grid, piece), piece.area);
    const std::vector<Vec3> applied = withoutNetRotation(same, inertia);
    for (const Vec3& omega : applied) expect(length(omega) < 1e-21, "a common rotation is the net rotation", length(omega));
    expect(substepCount(0.0, PlanetStep, 55e3) == 1, "at least one substep", substepCount(0.0, PlanetStep, 55e3));
    expect(substepCount(0.16, PlanetStep, 55e3) == 6, "80 mm/yr against each other takes six substeps at 55 km", substepCount(0.16, PlanetStep, 55e3));
}

} // namespace derivean::sim::globe_test

void testGlobeKinematics() { checkKinematics(); }

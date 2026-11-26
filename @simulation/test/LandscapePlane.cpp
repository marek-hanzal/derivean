#include "LandscapeFixture.hpp"

#include "measure/constant/Angle.hpp"
#include "measure/fn/Plane.hpp"
#include "measure/fn/Regularity.hpp"

using namespace derivean::sim;

// A featureless slope must stay one: the solver may not draw a pattern of its own on it, whatever
// its heading against the grid, while a crease in the same slope must gather the water.
void testLandscapePlane() {
    for (const double degrees : {0.0, 22.5, 45.0}) {
        PlaneSpec spec;
        spec.level = Level::Site;
        spec.azimuth = degrees / DegreesPerRadian;
        spec.steps = 30;
        const PlaneReport site = measurePlane(spec, replayPlane(spec));
        expectLandscape(site.gully.p95 <= NotchDepth * specOf(Level::Site).cell, "a site plane was cut into gullies", site.gully.p95);
        expectLandscape(site.gridShare == 0, "a site plane carries a pattern at the grid scale", site.gridShare);
        expectLandscape(std::abs(site.lowered) < 0.5, "a site plane left the surface it is held to", site.lowered);
    }

    // The local level does cut valleys into a plane; what it may not do is run them along the grid.
    PlaneSpec local;
    local.level = Level::Local;
    local.azimuth = 22.5 / DegreesPerRadian;
    local.steps = 20;
    const PlaneReport plane = measurePlane(local, replayPlane(local));
    expectLandscape(plane.paths > 200 && plane.headingError < 15, "water on a plane at 22.5 degrees leaves the fall line for a grid direction", plane.headingError);
    expectLandscape(plane.gridShare <= 0.05, "a local plane carries a pattern at the grid scale", plane.gridShare);
    expectLandscape(std::abs(plane.lowered) < 2, "a local plane left the surface it is held to", plane.lowered);

    local.swale = 0.5 * local.slope * specOf(Level::Local).cell * DomainCells / 2;
    local.swalePower = 1;
    const PlaneReport crease = measurePlane(local, replayPlane(local));
    expectLandscape(crease.gathered > plane.gathered + 0.2 && crease.axisShare > 3 * 0.07, "a crease does not gather the water of its flanks", crease.gathered);
}

// Boundary events on two hemispheres turning against each other (S3-design.md §11 test 7): new floor
// at the ridge balances what the trench consumes, nothing is born where the contact closes, the
// crust that goes into the interior is the consumed crust less the accreted wedge, a collision stops
// at StackLimit and backs up without losing volume, and the footprints keep covering the sphere.
#include "GlobeHemispheres.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"

#include <algorithm>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

constexpr double Step = 1 * Myr;

// How deep a direction lies inside the lune where both original hemispheres, turned rigidly by their
// plates' vectors over `time`, still cover the sphere: the trench's lune, where the contact closes.
double closingDepth(const Globe& globe, Vec3 x, double time) {
    const double north = rotate(inverse(rotationOf(globe.epoch.plates[0].euler, time)), x).z;
    const double south = -rotate(inverse(rotationOf(globe.epoch.plates[1].euler, time)), x).z;
    return std::min(north, south);
}

void checkRidgeAndTrench() {
    Hemispheres h = hemispheres({1, 0, 0}, 0.05, 300.0f, 300.0f);
    Globe& globe = h.globe;
    long closingBirths = 0;
    for (int i = 0; i < 200; ++i) {
        const long births = globe.stats.births;
        advance(h, Step);
        if (globe.stats.births == births) continue;
        // Within one cell (2.8° at level 5) of the lune's edge the discrete boundary may lead or lag.
        for (const CrustPiece& piece : globe.pieces)
            if (piece.key.step == h.step - 1 && closingDepth(globe, globe.grid->centre[size_t(piece.birth)], (i + 1) * Step) > 0.05) ++closingBirths;
    }
    const GlobeLedger& ledger = globe.ledger;
    expect(globe.stats.births > 0 && globe.stats.consumptions > 0, "two hemispheres neither bear nor consume");
    expect(std::abs(ledger.born / ledger.consumed - 1) < 0.02, "the ridge does not balance the trench to 2 %", ledger.born / ledger.consumed - 1);
    expect(closingBirths == 0, "a piece is born where the contact closes", double(closingBirths));
    expect(std::abs(felsicDrift(h)) < 1e-12, "felsic crust is not conserved with the interior", felsicDrift(h));
    expect(std::abs(basaltDrift(h)) < 1e-12, "basalt is not conserved with the interior", basaltDrift(h));
    expect(ledger.accreted > 0, "the trench scrapes no wedge");
    expect(std::abs(ledger.interior.felsic - ledger.subducted) <= 1e-12 * h.start.felsic, "the interior's crust is not what subducted", ledger.interior.felsic - ledger.subducted);
    expect(std::abs(coverage(h) - 1) < 0.02, "the footprints do not cover the sphere to 2 %", coverage(h) - 1);
    for (const CrustPiece& piece : globe.pieces) expect(piece.stock.felsic >= 0 && piece.stock.basalt >= 0, "a piece holds a negative stock");
    // Exact weights overcount where footprints overlap, so A − Σ w bounds a cell's hole from below (the
    // cloud-in-cell weights of the step do not reach across a face edge at a cube vertex).
    Coupling exact;
    couple(*globe.grid, globe.pieces, {}, true, exact);
    double worst = 0;
    for (int cell = 0; cell < globe.grid->cells; ++cell) worst = std::max(worst, 1 - exact.covered[size_t(cell)] / globe.grid->area[size_t(cell)]);
    expect(worst < 0.5, "a cell is left uncovered by half or more", worst);
}

// Two continents: nothing sinks, the arriving plate is consumed into the other, stacking stops at StackLimit and the rest backs up or brakes the plate.
void checkCollision() {
    Hemispheres h = hemispheres({1, 0, 0}, 0.05, 30000.0f, 200.0f);
    Globe& globe = h.globe;
    for (int i = 0; i < 60; ++i) advance(h, Step);
    float thickest = 0;
    for (const CrustPiece& piece : globe.pieces) thickest = std::max(thickest, piece.column.crust);
    expect(globe.ledger.stacked > 0, "two continents do not stack");
    // Only what rounding of the shares leaves on a consumed piece goes to the interior.
    expect(globe.ledger.subducted <= 1e-12 * h.start.felsic, "a continent sinks", globe.ledger.subducted);
    expect(thickest <= StackLimit * (1 + 1e-6f), "a collision stacks beyond StackLimit", thickest);
    expect(std::abs(felsicDrift(h)) < 1e-12, "a collision does not conserve crust", felsicDrift(h));
    expect(globe.motion[0].resistance + globe.motion[1].resistance > 0, "the arriving plate is not braked");
}

} // namespace

void testGlobeBoundary() {
    checkRidgeAndTrench();
    checkCollision();
}

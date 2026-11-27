// Boundary events on two hemispheres turning against each other (S3-design.md §11 test 7): new floor
// at the ridge balances what the trench consumes and the integrated spreading, is born on the axis and
// ages away from it on both sides, nothing is born where the contact closes, the
// crust that goes into the interior is the consumed crust less the accreted wedge, a collision stops
// at StackLimit and backs up without losing volume, the footprints keep covering the sphere, three
// plates meeting at a trench and at a ridge triple junction conserve their crust, the weld moves new
// crust by the union of its plate's cover, not the sum, and two oceans meet with their plates' polarity.
#include "GlobeHemispheres.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Band.hpp"
#include "planet/fn/Lattice.hpp"

#include <algorithm>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

constexpr double Step = 1 * Myr;

// The two hemispheres as their plates' applied vectors turned them, and the frame of the ridge's axis:
// no-net-rotation adds a common turn as the plates' areas change (Rotate.cpp), so the axis, the midline
// between the plates' trailing edges, moves by the mean of the two vectors (a ridge migrates over the
// mantle when its plates move unequally over it, as Earth's do).
struct Frames {
    Rotation plate[2], axis;
};
void carry(Frames& frames, const Globe& globe, double dt) {
    for (int p = 0; p < 2; ++p) frames.plate[p] = compose(rotationOf(globe.applied[size_t(p)], dt), frames.plate[p]);
    frames.axis = compose(rotationOf((globe.applied[0] + globe.applied[1]) * 0.5, dt), frames.axis);
}

// How deep a direction lies inside the lune where both original hemispheres, turned rigidly by their
// plates, still cover the sphere: the trench's lune, where the contact closes.
double closingDepth(const Frames& frames, Vec3 x) {
    return std::min(rotate(inverse(frames.plate[0]), x).z, -rotate(inverse(frames.plate[1]), x).z);
}

GlobeCover outsideBand(const Globe& globe) {
    Band band;
    markBand(globe, 1, band);
    return measureCover(globe, band.decide);
}

void checkRidgeAndTrench() {
    Hemispheres h = hemispheres({1, 0, 0}, 0.05, 300.0f, 300.0f);
    Globe& globe = h.globe;
    Frames frames;
    long closingBirths = 0, births = 0;
    double offAxis = 0, farthest = 0, apart = 0, widest = 0, bornBy[2] = {};
    const double cell = std::sqrt(4 * 3.141592653589793 / double(globe.grid->area.size())) * PlanetRadius;   // the mean cell
    for (int i = 0; i < 200; ++i) {
        const long before = globe.stats.births;
        advance(h, Step);
        carry(frames, globe, Step);
        if (globe.stats.births == before) continue;
        for (const CrustPiece& piece : globe.pieces) {
            if (piece.key.step != h.step - 1) continue;
            // Born at the boundary: next to the other plate's nearest piece, about a cell away.
            const Vec3 c = centreOf(*globe.grid, piece);
            double nearest = 4;
            for (const CrustPiece& other : globe.pieces)
                if (other.plate != piece.plate) nearest = std::min(nearest, length(centreOf(*globe.grid, other) - c));
            apart += nearest * PlanetRadius, widest = std::max(widest, nearest * PlanetRadius);
            bornBy[piece.plate] += piece.area;
            // Within one cell (2.8° at level 5) of the lune's edge the discrete boundary may lead or lag.
            if (closingDepth(frames, globe.grid->centre[size_t(piece.key.cell)]) > 0.05) ++closingBirths;
            // The axis is where the youngest floor forms (Codex DREV-CLAUDE-005-02): by symmetry the plane
            // z = 0 of the axis frame. A piece moves 50 km in the step it was born in.
            const double d = std::abs(std::asin(rotate(inverse(frames.axis), c).z)) * PlanetRadius;
            offAxis += d, farthest = std::max(farthest, d), ++births;
        }
    }
    const GlobeLedger& ledger = globe.ledger;
    expect(globe.stats.births > 0 && globe.stats.consumptions > 0, "two hemispheres neither bear nor consume");
    expect(std::abs(ledger.born / ledger.consumed - 1) < 0.02, "the ridge does not balance the trench to 2 %", ledger.born / ledger.consumed - 1);
    expect(closingBirths == 0, "a piece is born where the contact closes", double(closingBirths));
    expect(apart / double(births) < 1.25 * cell && widest < 2 * cell, "new floor is born away from the other plate", std::max(apart / double(births), widest / 2) / cell);
    // Each plate takes half the new floor to 3 %; what the shares leave over moves the axis against the
    // frame, by a cell over 200 Myr at 2 %.
    expect(std::abs(bornBy[0] / (bornBy[0] + bornBy[1]) - 0.5) < 0.03, "the ridge does not spread symmetrically", bornBy[0] / (bornBy[0] + bornBy[1]));
    // Measured 0.65 and 2.47 cells; the drift of the axis is a walk over 200 Myr, so the gate leaves room.
    expect(offAxis / double(births) < cell && farthest < 3 * cell, "new floor is born away from the ridge's axis", std::max(offAxis / double(births), farthest / 3) / cell);
    // The ridge opens at 2 ω R |sin φ| along the half circle φ ∈ (0, π) of the axis: 4 ω R² = 4 v R of new
    // floor a year, v = 50 mm/yr.
    const double spread = 4 * 0.05 * PlanetRadius * 200 * Step;
    expect(std::abs(ledger.born / spread - 1) < 0.03, "the floor born differs from the integrated spreading by 3 %", ledger.born / spread - 1);
    // Ages grow away from the axis on both sides as the rotation carried them: each plate turns at ω
    // about the pole x against the axis frame, so age grows by 1 / ω per radian from the axis (θ from
    // +y, where the ridge runs, about x). The slope of age over θ is fitted per side, so where the axis
    // stands within its cell does not count; pieces near the pole, where the arc is short, are left out.
    double n[2] = {}, st[2] = {}, sa[2] = {}, stt[2] = {}, sta[2] = {};
    for (const CrustPiece& piece : globe.pieces) {
        const Vec3 c = rotate(inverse(frames.axis), centreOf(*globe.grid, piece));
        if (piece.key.step < 0 || c.y * c.y + c.z * c.z < 0.09) continue;
        const double theta = std::atan2(std::abs(c.z), c.y), age = globe.time - piece.bornAt;
        if (theta < 0.1) continue;
        const int side = c.z > 0;
        n[side] += 1, st[side] += theta, sa[side] += age, stt[side] += theta * theta, sta[side] += theta * age;
    }
    for (int side = 0; side < 2; ++side) {
        const double slope = (n[side] * sta[side] - st[side] * sa[side]) / (n[side] * stt[side] - st[side] * st[side]);
        expect(n[side] > 100 && std::abs(slope / (PlanetRadius / 0.05) - 1) < 0.05, "ages do not grow away from the ridge as the plates carried them", slope / (PlanetRadius / 0.05));
    }
    expect(std::abs(felsicDrift(h)) < 1e-12, "felsic crust is not conserved with the interior", felsicDrift(h));
    expect(std::abs(basaltDrift(h)) < 1e-12, "basalt is not conserved with the interior", basaltDrift(h));
    expect(ledger.accreted > 0, "the trench scrapes no wedge");
    // Only oceans meet here, so all that the trench consumed went down but the wedge.
    const double sunk = ledger.consumedFelsic - ledger.accreted;
    expect(std::abs(ledger.interior.felsic - sunk) <= 1e-12 * h.start.felsic, "the interior's crust is not the consumed less the wedge", ledger.interior.felsic - sunk);
    expect(std::abs(coverage(h) - 1) < 0.02, "the footprints do not cover the sphere to 2 %", coverage(h) - 1);
    for (const CrustPiece& piece : globe.pieces) expect(piece.stock.felsic >= 0 && piece.stock.basalt >= 0, "a piece holds a negative stock");
    // §4's local measures from the union in every cell: outside the band new floor tiles its plate's
    // lattice; within it a gap stands open until it bears and a trench until it consumes.
    const GlobeCover cover = measureCover(globe), outside = outsideBand(globe);
    expect(outside.worstHole < 0.05 && outside.worstOverlap < 0.05, "a cell outside the band is left open or covered twice", std::max(outside.worstHole, outside.worstOverlap));
    expect(cover.worstHole < 0.5 && cover.hole < 0.01 && cover.overlap < 0.01, "the band leaves holes or overlaps", std::max(cover.worstHole, 100 * std::max(cover.hole, cover.overlap)));
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

// Three oceanic plates, the sectors of 120° of longitude about z, each turning its middle meridian
// towards the north pole (Codex DREV-CLAUDE-005-02): at the north pole every pair closes and the
// trenches meet in a triple junction, at the south pole every pair opens. The plan, the births and the
// coupling go through consumeInBand where three plates share cells.
void checkTripleJunction() {
    Hemispheres h = hemispheres({1, 0, 0}, 0.05, 300.0f, 300.0f);
    Globe& globe = h.globe;
    globe.epoch.plates.resize(3);
    globe.motion.assign(3, GlobeMotion{});
    globe.applied.assign(3, Vec3{});
    for (int p = 0; p < 3; ++p) {
        const double middle = 2.0943951023931953 * p;
        globe.epoch.plates[size_t(p)].euler = Vec3{std::sin(middle), -std::cos(middle), 0} * (0.05 / PlanetRadius);
    }
    for (CrustPiece& piece : globe.pieces) {
        const Vec3 c = globe.grid->centre[size_t(piece.birth)];
        const double turn = std::atan2(c.y, c.x) + 1.0471975511965976;
        piece.plate = uint16_t(int(std::floor((turn < 0 ? turn + 6.283185307179586 : turn) / 2.0943951023931953)) % 3);
    }
    couple(*globe.grid, globe.pieces, {}, true, globe.coupling);
    for (int i = 0; i < 60; ++i) advance(h, Step);
    expect(globe.stats.consumptions > 0 && globe.stats.births > 0, "a triple junction neither consumes nor bears");
    expect(globe.stats.contested > 0, "no piece is covered by two overriding plates at the triple junction");
    expect(std::abs(globe.ledger.born / globe.ledger.consumed - 1) < 0.05, "the triple junctions do not balance to 5 %", globe.ledger.born / globe.ledger.consumed - 1);
    expect(std::abs(felsicDrift(h)) < 1e-12 && std::abs(basaltDrift(h)) < 1e-12, "a triple junction does not conserve crust", felsicDrift(h));
    expect(std::abs(coverage(h) - 1) < 0.02, "a triple junction does not cover the sphere to 2 %", coverage(h) - 1);
    const GlobeCover outside = outsideBand(globe);
    expect(outside.worstHole < 0.05 && outside.worstOverlap < 0.05, "a cell away from the triple junction is left open or covered twice", std::max(outside.worstHole, outside.worstOverlap));
}

// The weld (Codex DREV-CLAUDE-005-02): a piece of plate 0 at the boundary moved 0.3 of a cell into the
// cell of plate 1 beside it, and a second piece on top of it. The cell is covered 0.3 by the union and
// 0.6 by the sum; the weld turns new crust there by the union's length, not the sum's.
void checkWeld() {
    Hemispheres h = hemispheres({1, 0, 0}, 0.05, 300.0f, 300.0f);
    Globe& globe = h.globe;
    const SphereGrid& grid = *globe.grid;
    int north = -1, south = -1;
    for (size_t k = 0; k < globe.pieces.size() && north < 0; ++k) {
        const Vec3 c = grid.centre[size_t(globe.pieces[k].birth)];
        if (globe.pieces[k].plate != 0 || std::abs(c.x) > 0.3) continue;
        for (int e = 0; e < 4; ++e) {
            const int other = neighbourOf(grid, globe.pieces[k].birth, e);
            if (grid.centre[size_t(other)].z < 0 && grid.centre[size_t(other)].z < c.z - 0.5 * std::sqrt(grid.area[size_t(other)]) / PlanetRadius) north = int(k), south = other;
        }
    }
    expect(north >= 0, "no boundary piece for the weld");
    const Vec3 from = grid.centre[size_t(globe.pieces[size_t(north)].birth)], x = grid.centre[size_t(south)];
    const Vec3 normal = towards(x, x + (x - from));
    const double side = std::sqrt(grid.area[size_t(south)]) / PlanetRadius;
    CrustPiece& moved = globe.pieces[size_t(north)];
    moved.rotation = rotationAbout(cross(from, normal), 0.3 * side);
    CrustPiece twin = moved;
    twin.key = {0, moved.key.cell, 1};
    globe.pieces.push_back(twin);
    sortPieces(globe.pieces);
    couple(grid, globe.pieces, {}, true, globe.coupling);
    Band band;
    markBand(globe, 1, band);
    coupleBand(globe, band);
    const Over behind = overOf(globe, band, south, 0, x, normal);
    expect(behind.pieces.size() == 2, "the weld does not see both pieces behind it", double(behind.pieces.size()));
    long unions = 0;
    const double united = unionInCell(globe, south, behind.pieces, behind.sum, unions);
    expect(std::abs(united / grid.area[size_t(south)] - 0.3) < 0.05 && behind.sum > 1.9 * united, "the moved pieces do not cover 0.3 of the cell twice", united / grid.area[size_t(south)]);
    const double turn = weldTurn(globe, band, south, 0, x, normal);
    expect(std::abs(turn - united / std::sqrt(grid.area[size_t(south)]) / PlanetRadius) < 1e-12 * side, "the weld does not turn by the union", turn / side);
}

// The plate whose ocean floor was older as the epoch began sinks along the whole trench (sinksUnder),
// though every piece of the other plate is older; either plate, so neither the index nor the pieces decide.
void checkTrenchPolarity() {
    for (int older = 0; older < 2; ++older) {
        Hemispheres h = hemispheres({1, 0, 0}, 0.05, 300.0f, 300.0f);
        Globe& globe = h.globe;
        globe.motion[size_t(older)].oceanAge = 80 * Myr, globe.motion[size_t(1 - older)].oceanAge = 20 * Myr;
        long left[2] = {}, kept[2] = {};
        for (CrustPiece& piece : globe.pieces) piece.column.oceanAge = float((piece.plate == older ? 50 : 120) * Myr), ++left[piece.plate];
        for (int i = 0; i < 20; ++i) advance(h, Step);
        for (const CrustPiece& piece : globe.pieces) kept[piece.plate] += piece.key.step < 0;
        expect(kept[older] < left[older] && kept[1 - older] == left[1 - older], "a trench between two oceans follows the pieces' ages, not the plates'", double(older));
    }
}

} // namespace

void testGlobeBoundary() {
    checkRidgeAndTrench();
    checkCollision();
    checkTripleJunction();
    checkWeld();
    checkTrenchPolarity();
}

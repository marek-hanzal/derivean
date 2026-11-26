// Budgets and records through the coupling and the boundary (S3-design.md §11 tests 4, 5 and 14):
// 1 000 writes through the weights between turns book exactly what they change, pieces that took no
// part in a transfer keep their record bit for bit, a new epoch changes only the plate, and the
// boundary's areas converge with the step.
#include "GlobeHemispheres.hpp"

#include "geometry/fn/Rotation.hpp"
#include "planet/fn/Rotate.hpp"


using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

// Test 4: gains and losses of random cells (some larger than the pieces hold) booked into the pieces
// through the weights, with the crust turned between writes; a slip of coverage is left in.
void checkWrites() {
    Globe globe = makeGlobe(5, 5, PlateSpeeds::Today, TectonicsStart);
    Random random{99};
    onePlate(globe, random.direction() * (0.03 / PlanetRadius));
    couple(*globe.grid, globe.pieces, {}, false, globe.coupling);
    Sum booked;
    const double before = totalStock(globe.pieces).felsic;
    for (int i = 0; i < 1000; ++i) {
        const int cell = int(random.next() * globe.grid->cells);
        const double area = globe.grid->area[size_t(cell)];
        const double volume = (random.next() < 0.5 ? -1 : 1) * random.next() * 40000.0 * area;
        booked.add(bookCell(globe.coupling, globe.pieces, cell, volume));
        if (i % 50 == 49) {
            rotatePieces(globe.pieces, globe.applied, 1 * Myr);
            couple(*globe.grid, globe.pieces, {}, i % 100 == 99, globe.coupling);
        }
    }
    const double change = totalStock(globe.pieces).felsic - before;
    expect(std::abs(change - booked.value()) <= 1e-12 * before, "writes through the weights do not book what they change", change - booked.value());
    for (const CrustPiece& piece : globe.pieces) expect(piece.stock.felsic >= 0, "a write leaves a negative stock", piece.stock.felsic);
}

// Test 5: after a step only donors and receivers change their record; a new epoch changes only `plate`.
void checkRecords() {
    Hemispheres h = hemispheres({1, 0, 0}, 0.05, 300.0f, 300.0f);
    Globe& globe = h.globe;
    for (int i = 0; i < 3; ++i) advance(h, 1 * Myr);
    std::vector<CrustPiece> before = globe.pieces;
    advance(h, 1 * Myr);
    size_t far = 0;
    for (const CrustPiece& was : before) {
        if (std::abs(centreOf(*globe.grid, was).z) < 0.5) continue;
        const auto now = std::find_if(globe.pieces.begin(), globe.pieces.end(), [&](const CrustPiece& p) { return p.key == was.key; });
        expect(now != globe.pieces.end(), "a piece far from the boundary is gone");
        expect(sameBits(now->record, was.record) && sameBits(now->stock, was.stock), "a piece far from the boundary changed its record or stock");
        ++far;
    }
    expect(far > 1000, "too few pieces far from the boundary", double(far));
    before = globe.pieces;
    globe.seed = 42;
    enterGlobeEpoch(globe);
    for (size_t k = 0; k < before.size(); ++k) {
        CrustPiece was = before[k];
        was.plate = globe.pieces[k].plate;
        expect(sameBits(was.rotation, globe.pieces[k].rotation) && sameBits(was.record, globe.pieces[k].record) && sameBits(was.stock, globe.pieces[k].stock) &&
                   sameBits(was.column, globe.pieces[k].column) && was.birth == globe.pieces[k].birth && was.key == globe.pieces[k].key,
               "a new epoch changes more than the plate");
    }
}

// Test 14: an oblique boundary, a transform and a boundary along face diagonals through cube vertices,
// each over the same 192 Myr in steps of dt, dt/2 and dt/4. The consumed and born areas agree to 2 %.
// The substeps adapt to the motion (Codex V4), so dt and dt/2 can run the same substep and the
// difference of dt against dt/2 is not asked to exceed that of dt/2 against dt/4; a row of level-5
// cells along the trench is itself about 2 % of what it consumes in that time.
struct Run {
    double born{}, consumed{}, drift{}, cover{};
};

Run run(Vec3 pole, Vec3 split, double dt) {
    Hemispheres h = hemispheres(pole, 0.05, 300.0f, 300.0f, normalized(split));
    for (double t = 0; t < 192 * Myr - 1; t += dt) advance(h, dt);
    return {h.globe.ledger.born, h.globe.ledger.consumed, felsicDrift(h), coverage(h)};
}

void checkSubsteps() {
    const double cell = 4 * 3.141592653589793 * PlanetRadius * PlanetRadius / (6 * 32 * 32);
    struct Boundary {
        const char* name;
        Vec3 pole, split;
    };
    for (const Boundary& b : {Boundary{"oblique", {1, 0, 0.35}, {0, 0, 1}}, Boundary{"vertex", {0.3, 0.3, 1}, {1, -1, 0}}, Boundary{"transform", {0, 0, 1}, {0, 0, 1}}}) {
        const Run full = run(b.pole, b.split, 4 * Myr), half = run(b.pole, b.split, 2 * Myr), quarter = run(b.pole, b.split, 1 * Myr);
        for (const Run& r : {full, half, quarter}) expect(std::abs(r.drift) < 1e-12, "a step size loses crust", r.drift);
        if (b.name[0] == 't') {
            for (const Run& r : {full, half, quarter}) expect(r.born + r.consumed < 20 * cell, "a transform churns crust", (r.born + r.consumed) / cell);
            continue;
        }
        const auto gap = [](double a, double c) { return std::abs(a / c - 1); };
        for (const Run* r : {&full, &half})
            expect(gap(r->born, quarter.born) <= 0.02 && gap(r->consumed, quarter.consumed) <= 0.02, "a step differs from dt/4 by more than 2 %", gap(r->consumed, quarter.consumed));
    }
}

} // namespace

void testGlobeBalance() {
    checkWrites();
    checkRecords();
    checkSubsteps();
}

// The plates of the real generator across a reorganisation (Codex DREV-CLAUDE-005-04; lead, 27 Nov):
// level 5, seed 42, the kinematics alone from TectonicsStart through the first new epoch (step 103) and
// 27 steps on.
// - In the first epoch the band leaves no cell open by half or more at the end of any step and nothing
//   outside it is open or doubled by more than 0.05 (one lattice per plate).
// - Dealing the pieces to the new plates moves the old boundaries' holes and overlaps inside one plate
//   without moving a footprint; the seam closing then leaves no piece half covered by its own plate and
//   no slot inside a plate half open (GlobeGaps.hpp).
// - The steps after keep both gates in the plate's frame and no cell of a boundary proper (two plates
//   within its ring) open by half; the stock of felsic crust and basalt with the interior stays exact.
// - A seam whose crust finds no room within StackLimit nearby spreads it through its plate and
//   delaminates the rest, and closes with the stock and StackLimit kept.
// - After every step each plate is one region with a cell (planet/fn/Cohesion, stats.wholeUnsettled; no
//   region of MicroplateCells and MicroplateArea apart, GlobeCohesion.hpp), fragments do join the plate
//   round them, and under 1 % of the cells lie outside their plate's largest region at the end of the
//   first epoch and 27 steps after the reorganisation; the closing leaves every surviving piece its
//   birth, area, rotation and age.
// - The plates' mean ocean ages that set the trenches' polarity for the epoch (sinksUnder) are those of
//   the floor dealt to them.
// A rigid plate of two lattices carries a seam's gap of under half a slot diagonally over the mantle
// cells and opens them by up to √2 of it, so per mantle cell inside a plate the cover is reported
// (docs/handoff/planet-globe.md), not gated.
#include "GlobeCohesion.hpp"
#include "GlobeFixture.hpp"
#include "GlobeGaps.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/GlobeCover.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "planet/fn/Seam.hpp"

#include <algorithm>
#include <cmath>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

struct Covers {
    GlobeCover band, outside;
};

Covers coversOf(const Globe& globe) {
    Band band;
    markBand(globe, 1, band);
    std::vector<uint8_t> outside(band.decide.size());
    for (size_t c = 0; c < outside.size(); ++c) outside[c] = !band.decide[c];
    return {measureCover(globe, outside), measureCover(globe, band.decide)};
}

// The cells of the boundaries proper: with pieces of two plates within their ring.
GlobeCover coreCover(const Globe& globe) {
    const Coupling& c = globe.coupling;
    std::vector<uint8_t> skip(size_t(globe.grid->cells), 1);
    for (int cell = 0; cell < globe.grid->cells; ++cell) {
        int plate = -1;
        const auto look = [&](int at) {
            for (int32_t s = c.cellStart[size_t(at)]; s < c.cellStart[size_t(at) + 1]; ++s) {
                const int p = globe.pieces[size_t(c.byCell[size_t(s)].piece)].plate;
                if (plate < 0) plate = p;
                else if (p != plate) skip[size_t(cell)] = 0;
            }
        };
        look(cell);
        for (const int32_t other : (*globe.rings)[size_t(cell)])
            if (other >= 0) look(other);
    }
    return measureCover(globe, skip);
}

double drift(const Globe& globe, const CrustStock& start) {
    const CrustStock now = totalStock(globe.pieces);
    return std::max(std::abs(now.felsic + globe.ledger.interior.felsic - start.felsic) / start.felsic,
                    std::abs(now.basalt + globe.ledger.interior.basalt - start.basalt) / start.basalt);
}

void step(Globe& globe, int index) {
    advancePlates(globe, PlanetStep, index);
    keepPlatesWhole(globe, index);
    const Cohesion cohesion = measureCohesion(globe);
    expect(cohesion.fragment < MicroplateArea && globe.stats.wholeUnsettled == 0, "a plate is not one region with a cell after a step", cohesion.fragment);
    const PlateRegions regions = plateRegionsOf(globe);
    for (size_t p = 0; p < globe.epoch.plates.size(); ++p)
        expect(p < regions.largest.size() && regions.largest[p] >= 0, "a plate holds no cell after a step", double(p));
    for (CrustPiece& piece : globe.pieces) piece.column.oceanAge += float(PlanetStep);
    globe.time += PlanetStep;
}

void expectCohesive(const Globe& globe, const char* what) {
    const Cohesion cohesion = measureCohesion(globe);
    expect(100 * cohesion.stray < cohesion.cells, what, double(cohesion.stray) / double(cohesion.cells));
}

// Codex DREV-CLAUDE-005-06: a piece that survives a closing keeps what it was born with.
void expectSameSurvivors(const std::vector<CrustPiece>& before, const std::vector<CrustPiece>& after, const char* what) {
    expect(std::is_sorted(before.begin(), before.end(), [](const CrustPiece& a, const CrustPiece& b) { return a.key < b.key; }), what);
    for (const CrustPiece& piece : after) {
        const auto was = std::lower_bound(before.begin(), before.end(), piece, [](const CrustPiece& a, const CrustPiece& b) { return a.key < b.key; });
        if (was == before.end() || !(was->key == piece.key)) continue;
        expect(was->birth == piece.birth && sameBits(was->area, piece.area) && sameBits(was->rotation, piece.rotation) && sameBits(was->column.oceanAge, piece.column.oceanAge),
               what, double(piece.key.cell));
    }
}

// The plates' mean ocean ages as enterGlobeEpoch sets them, after the pieces are dealt to the new plates
// (test 7 checks that a trench follows them).
void checkOceanAges(const Globe& globe) {
    std::vector<double> age(globe.motion.size(), 0), area(globe.motion.size(), 0);
    for (const CrustPiece& piece : globe.pieces)
        if (!continentalPiece(piece)) age[piece.plate] += double(piece.column.oceanAge) * piece.area, area[piece.plate] += piece.area;
    for (size_t p = 0; p < age.size(); ++p) {
        const double mean = area[p] > 0 ? age[p] / area[p] : 0;
        expect(std::abs(globe.motion[p].oceanAge - mean) <= 1e-9 * std::max(1.0, mean), "a plate's mean ocean age is not its floor's", double(p));
    }
}

// A seam with no room (Codex DREV-CLAUDE-005-05): every piece of a level-5 sphere holds StackLimit of
// crust, an old boundary runs along the equator and a second continental piece lies 0.1 of a cell off
// one of its pieces. Once both plates are one, no piece anywhere has room for the doubled piece's crust
// within StackLimit: the plan refuses it, the wide reach finds no room either, and the crust delaminates
// into the interior, so the seam closes with the stock and StackLimit kept. With room 700–950 km away
// (Room::Within), beyond the ordinary reach and within the wide one, the crust goes there and nothing
// founders; with room only 1 050–1 300 km away (Room::Beyond, Codex DREV-CLAUDE-005-08) nothing goes there
// and the crust founders.
enum class Room { None, Within, Beyond };
void checkCrowdedSeam(Room room) {
    const bool far = room != Room::None;
    const double nearest = room == Room::Beyond ? 1.05e6 : 7e5, farthest = room == Room::Beyond ? 1.3e6 : 9.5e5;
    Globe globe = makeGlobe(42, 5, PlateSpeeds::Today, TectonicsStart);
    const SphereGrid& grid = *globe.grid;
    int k = -1;
    for (int c = 0; c < grid.cells; ++c) {
        CrustPiece& piece = globe.pieces[size_t(c)];
        piece.plate = grid.centre[size_t(c)].z < 0 ? 1 : 0;
        piece.stock.felsic = double(StackLimit) * piece.area;
        refreshColumn(piece);
        if (k < 0 && piece.plate == 0 && grid.centre[size_t(c)].z < 0.04 && grid.centre[size_t(c)].x > 0.8) k = c;
    }
    expect(k >= 0, "the crowded seam finds no piece on the equator");
    std::vector<int> roomy;
    std::vector<double> roomyStock;
    for (int c = 0; far && c < grid.cells; ++c) {
        const double d = length(grid.centre[size_t(c)] - grid.centre[size_t(k)]) * PlanetRadius;
        if (d < nearest || d > farthest || globe.pieces[size_t(c)].plate != 0) continue;
        globe.pieces[size_t(c)].stock.felsic *= 0.5, refreshColumn(globe.pieces[size_t(c)]);
        roomy.push_back(c), roomyStock.push_back(globe.pieces[size_t(c)].stock.felsic);
    }
    CrustPiece other = globe.pieces[size_t(k)];
    other.rotation = rotationAbout(normalized(cross(grid.centre[size_t(k)], {0, 0, -1})), 0.1 * std::sqrt(other.area) / PlanetRadius);
    other.plate = 1, other.key = {0, other.birth, 0};
    globe.pieces.push_back(other);
    globe.motion.assign(2, GlobeMotion{});
    couple(grid, globe.pieces, {}, true, globe.coupling);
    Band old;
    markBand(globe, 1, old);
    for (CrustPiece& piece : globe.pieces) piece.plate = 0;   // the old boundary is now a seam inside one plate
    expect(measureGaps(globe).ownHalf > 0, "the crowded seam is not half covered by its own plate");
    const CrustStock start = totalStock(globe.pieces);
    std::vector<CrustPiece> before = globe.pieces;
    std::sort(before.begin(), before.end(), [](const CrustPiece& a, const CrustPiece& b) { return a.key < b.key; });
    const bool settled = closeSeams(globe, old, 1, 0);
    expectSameSurvivors(before, globe.pieces, "the crowded seam changes a surviving piece's identity");
    const Gaps after = measureGaps(globe);
    const CrustStock now = totalStock(globe.pieces);
    expect(globe.stats.seamsRefused > 0, "the crowded seam finds room nearby", double(globe.stats.seamsRefused));
    expect(settled && after.ownHalf == 0 && globe.stats.unsettled == 0, "a seam with no room does not close", double(after.ownHalf));
    if (room != Room::Within) expect(globe.ledger.delaminated > 0 && globe.ledger.delaminated <= globe.ledger.interior.felsic, "the crowded seam delaminates nothing", globe.ledger.delaminated);
    // What the roomy pieces gained, against the stock each held (a difference of stocks, so no product
    // is contracted into it).
    double gained = 0;
    long touched = 0;
    for (const CrustPiece& piece : globe.pieces) {
        const auto at = std::find(roomy.begin(), roomy.end(), piece.key.cell);
        if (at == roomy.end() || piece.key.step >= 0) continue;
        const double held = roomyStock[size_t(at - roomy.begin())];
        gained += piece.stock.felsic - held, touched += piece.stock.felsic != held;
    }
    if (room == Room::Within) expect(globe.ledger.delaminated == 0 && gained > 0, "the crowded seam delaminates crust its plate has room for", globe.ledger.delaminated);
    if (room == Room::Beyond) expect(touched == 0, "the crowded seam sends crust beyond the wide reach", gained);
    expect(now.felsic + globe.ledger.interior.felsic == start.felsic && now.basalt + globe.ledger.interior.basalt == start.basalt, "the crowded seam loses crust");
    for (const CrustPiece& piece : globe.pieces) expect(piece.stock.felsic <= double(StackLimit) * piece.area * (1 + 1e-12), "the crowded seam stacks over StackLimit");
}

} // namespace

void testGlobeEpochs() {
    checkCrowdedSeam(Room::None);
    checkCrowdedSeam(Room::Within);
    checkCrowdedSeam(Room::Beyond);
    Globe globe = makeGlobe(42, 5, PlateSpeeds::Today, TectonicsStart);
    enterGlobeEpoch(globe);
    couple(*globe.grid, globe.pieces, {}, false, globe.coupling);
    const CrustStock start = totalStock(globe.pieces);
    int index = 0;
    for (; globe.time < globe.epoch.end; ++index) {
        step(globe, index);
        if (index % 5 != 4) continue;
        const Covers c = coversOf(globe);
        expect(c.band.worstHole < 0.5, "the first epoch's band leaves a cell open by half or more", c.band.worstHole);
        expect(c.outside.worstHole < 0.05 && c.outside.worstOverlap < 0.05, "a plate of one lattice is open or doubled outside the band", std::max(c.outside.worstHole, c.outside.worstOverlap));
    }
    expect(index > 60 && index < 160, "the first epoch is not where the fixture expects it", index);
    expectCohesive(globe, "a plate breaks into regions by the first epoch's end");

    Band old;
    markBand(globe, 1, old);
    const std::vector<CrustPiece> before = globe.pieces;
    enterGlobeEpoch(globe);
    checkOceanAges(globe);
    const Covers dealt = coversOf(globe);
    expect(dealt.outside.worstHole > 0.3 && dealt.outside.worstOverlap > 0.3, "dealing the pieces anew no longer moves a boundary's defects inside a plate", dealt.outside.worstHole);
    closeSeams(globe, old, index, 0);
    expectSameSurvivors(before, globe.pieces, "the closing changes a surviving piece's identity");
    const Gaps closed = measureGaps(globe);
    expect(closed.ownCover < 0.5, "a piece stays half covered by its own plate after the seams close", closed.ownCover);
    expect(closed.slotOpen < 0.5, "a slot inside a plate stays half open after the seams close", closed.slotOpen);
    expect(globe.stats.sutured > 0 && globe.stats.fossil > 0 && globe.stats.unsettled == 0, "the seams closed nothing or did not settle", double(globe.stats.sutured));
    const long sutured = globe.stats.sutured;
    expect(drift(globe, start) < 1e-12, "the reorganisation loses crust", drift(globe, start));

    for (const int end = index + 27; index < end; ++index) {
        step(globe, index);
        expect(globe.stats.unsettledSubsteps == 0, "a substep used all its rounds of births and sutures", double(globe.stats.unsettledSubsteps));
        if (index % 3 != 2) continue;
        const GlobeCover core = coreCover(globe);
        expect(core.worstHole < 0.5, "a boundary leaves a cell open by half or more after the reorganisation", core.worstHole);
        const Gaps gaps = measureGaps(globe);
        expect(gaps.ownCover < 0.5 && gaps.slotOpen < 0.5, "a plate is half open or half doubled in its own frame after the reorganisation", std::max(gaps.ownCover, gaps.slotOpen));
    }
    expect(globe.stats.sutured > sutured, "no seam closes in the band after the reorganisation", double(globe.stats.sutured - sutured));
    expectCohesive(globe, "a plate breaks into regions in the steps after the reorganisation");
    expect(globe.stats.fragmentsJoined > 0, "no fragment of a plate joins the plate round it", double(globe.stats.fragmentsJoined));
    expect(drift(globe, start) < 1e-12, "the steps after the reorganisation lose crust", drift(globe, start));
}

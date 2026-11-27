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
// A rigid plate of two lattices carries a seam's gap of under half a slot diagonally over the mantle
// cells and opens them by up to √2 of it, so per mantle cell inside a plate the cover is reported
// (docs/handoff/planet-globe.md), not gated.
#include "GlobeFixture.hpp"
#include "GlobeGaps.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/GlobeCover.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "planet/fn/Seam.hpp"

#include <algorithm>

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
    for (CrustPiece& piece : globe.pieces) piece.column.oceanAge += float(PlanetStep);
    globe.time += PlanetStep;
}

} // namespace

void testGlobeEpochs() {
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

    Band old;
    markBand(globe, 1, old);
    enterGlobeEpoch(globe);
    const Covers dealt = coversOf(globe);
    expect(dealt.outside.worstHole > 0.3 && dealt.outside.worstOverlap > 0.3, "dealing the pieces anew no longer moves a boundary's defects inside a plate", dealt.outside.worstHole);
    closeSeams(globe, old, index);
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
    expect(drift(globe, start) < 1e-12, "the steps after the reorganisation lose crust", drift(globe, start));
}

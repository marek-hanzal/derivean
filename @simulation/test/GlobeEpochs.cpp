// The plates of the real generator across a reorganisation (Codex DREV-CLAUDE-005-04): level 5, seed
// 42, the kinematics alone from TectonicsStart through the first new epoch (step 103) and 27 steps on.
// - In the first epoch the band leaves no cell open by half or more at the end of any step and nothing
//   outside it is open or doubled by more than 0.05 (one lattice per plate).
// - Dealing the pieces to the new plates moves the old boundaries' holes and overlaps inside one plate
//   without moving a footprint; the seam closing then consumes every piece its own plate covers by half
//   or more and fills every cell of the old band open by half or more, so that afterwards no cell
//   anywhere is open by half or more and no piece is half covered by its own plate.
// - The steps after keep the band's cells under half open again; the stock of felsic crust and basalt
//   with the interior stays exact throughout.
// Outside the band a rigid plate of two lattices can carry a seam's hole of under half a row diagonally
// over a mantle cell and open it by more (up to √2 of the row, docs/handoff/planet-globe.md), so after
// the reorganisation the outside is reported in the hand-over, not held to a gate per mantle cell.
#include "GlobeFixture.hpp"

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

// The largest share of a piece that other pieces of its own plate cover together.
double worstOwnCover(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    Coupling exact;
    couple(grid, globe.pieces, {}, true, exact);
    double worst = 0;
    std::vector<int> others;
    std::vector<PlanePolygon> parts;
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        const CrustPiece& piece = globe.pieces[k];
        others.clear();
        for (int32_t s = exact.pieceStart[k]; s < exact.pieceStart[k + 1]; ++s) {
            const int cell = exact.byPiece[size_t(s)].cell;
            for (int32_t w = exact.cellStart[size_t(cell)]; w < exact.cellStart[size_t(cell) + 1]; ++w) {
                const int l = exact.byCell[size_t(w)].piece;
                if (l != int(k) && globe.pieces[size_t(l)].plate == piece.plate && std::find(others.begin(), others.end(), l) == others.end()) others.push_back(l);
            }
        }
        if (others.empty()) continue;
        const TangentPlane plane = tangentPlaneAt(centreOf(grid, piece));
        const PlanePolygon own = projected(plane, footprintOf(grid, piece));
        parts.clear();
        for (const int l : others) parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(l)])), own));
        worst = std::max(worst, unionArea(parts).area / piece.area);
    }
    return worst;
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
    const GlobeCover closed = measureCover(globe);
    expect(closed.worstHole < 0.5, "a cell stays open by half or more after the seams close", closed.worstHole);
    expect(worstOwnCover(globe) < 0.5, "a piece stays half covered by its own plate after the seams close", worstOwnCover(globe));
    expect(globe.stats.sutured > 0 && globe.stats.fossil > 0, "the seams closed nothing", double(globe.stats.sutured));
    expect(drift(globe, start) < 1e-12, "the reorganisation loses crust", drift(globe, start));

    for (const int end = index + 27; index < end; ++index) {
        step(globe, index);
        if (index % 3 != 2) continue;
        const Covers c = coversOf(globe);
        expect(c.band.worstHole < 0.5, "the band leaves a cell open by half or more after the reorganisation", c.band.worstHole);
    }
    expect(drift(globe, start) < 1e-12, "the steps after the reorganisation lose crust", drift(globe, start));
}

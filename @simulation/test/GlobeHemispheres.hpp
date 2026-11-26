#pragma once

// Two hemispheres of one level-5 globe that turn in opposite senses about one pole in their common
// plane (S3-design.md §11 test 7): a ridge along half of the equator, a trench along the other half.
#include "GlobeFixture.hpp"

#include "chronology/constant/Timeline.hpp"
#include "geometry/constant/Sphere.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Band.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim::globe_test {

struct Hemispheres {
    Globe globe;
    CrustStock start;   // m³ at the start
    int step{};
};

// Plate 0 on the side of `split` (the equator by default), plate 1 on the other, turning at ±speed
// (m/yr at the pole's equator) about `pole`: in the dividing plane for a ridge and a trench, tilted out
// of it for oblique ones, along `split` for a transform. Every piece holds
// `crust` m of felsic crust with `cover` m of it as one dated deposit, at one ocean age.
inline Hemispheres hemispheres(Vec3 pole, double speed, float crust, float cover, Vec3 split = {0, 0, 1}) {
    Hemispheres h;
    Globe& globe = h.globe;
    globe = makeGlobe(42, 5, PlateSpeeds::Today, TectonicsStart);
    globe.epoch = EulerEpoch{};
    globe.epoch.start = globe.time - 1e11;
    globe.epoch.end = globe.time + 1e11;
    const Vec3 axis = normalized(pole);
    globe.epoch.plates.resize(2);
    globe.epoch.plates[0].euler = axis * (speed / PlanetRadius);
    globe.epoch.plates[1].euler = axis * (-speed / PlanetRadius);
    globe.motion.assign(2, GlobeMotion{});
    globe.applied.assign(2, Vec3{});
    for (CrustPiece& piece : globe.pieces) {
        piece.plate = dot(globe.grid->centre[size_t(piece.birth)], split) >= 0 ? 0 : 1;
        piece.column = CrustColumn{};
        piece.column.oceanAge = float(50 * Myr);
        piece.record.cover = Cover{};
        if (cover > 0) addDeposit(piece.record.cover, globe.time, cover, Facies{0, 40, 215, 0, 0, 0});
        piece.stock = {double(crust) * piece.area, 7000.0 * piece.area};
        refreshColumn(piece);
    }
    couple(*globe.grid, globe.pieces, {}, true, globe.coupling);
    h.start = totalStock(globe.pieces);
    return h;
}

inline void advance(Hemispheres& h, double dt) {
    advancePlates(h.globe, dt, h.step++);
    h.globe.time += dt;
}

// The stock with what the interior gained, against the start, relative.
inline double felsicDrift(const Hemispheres& h) {
    return (totalStock(h.globe.pieces).felsic + h.globe.ledger.interior.felsic - h.start.felsic) / h.start.felsic;
}
inline double basaltDrift(const Hemispheres& h) {
    return (totalStock(h.globe.pieces).basalt + h.globe.ledger.interior.basalt - h.start.basalt) / h.start.basalt;
}
// Holes outside the boundary band (S3-design.md §4). Births leave a cell open by under half, but a
// hole split across two mantle cells at its birth can later turn over one cell whole, so the worst
// share reaches over half (0.46–0.65 measured at L5); the share of cells open by a fifth or more
// stays small (2–4.6 %). See docs/handoff/planet-globe.md. The exact
// weights overcount overlaps, so both are lower bounds; the step's cloud-in-cell weights do not reach
// across a face edge at a cube vertex.
struct Holes {
    double worst{}, open{};   // the largest uncovered share of a cell; the share of cells covered under 0.8
};
inline Holes holesOf(const Globe& globe) {
    Band band;
    markBand(globe, 1, band);
    Coupling exact;
    couple(*globe.grid, globe.pieces, {}, true, exact);
    Holes holes;
    int open = 0, outside = 0;
    for (int cell = 0; cell < globe.grid->cells; ++cell) {
        if (band.decide[size_t(cell)]) continue;
        const double hole = 1 - exact.covered[size_t(cell)] / globe.grid->area[size_t(cell)];
        holes.worst = std::max(holes.worst, hole);
        open += hole > 0.2, ++outside;
    }
    holes.open = double(open) / std::max(1, outside);
    return holes;
}

inline double coverage(const Hemispheres& h) { return totalArea(h.globe.pieces) / (4 * 3.141592653589793 * PlanetRadius * PlanetRadius); }

} // namespace derivean::sim::globe_test

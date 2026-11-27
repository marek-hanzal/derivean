#include "planet/fn/Boundary.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

double shortestEdgeOf(const SphereGrid& grid) {
    double shortest = 1e30;
    for (const SphereEdge& edge : grid.edge) shortest = std::min(shortest, edge.length);
    return shortest;
}

// The fastest relative motion of two plates that meet in a deciding cell or its ring, m/yr.
double fastestRelative(const Globe& globe, const Band& band) {
    const Coupling& coupling = globe.coupling;
    double fastest = 0;
    std::vector<int> plates;
    for (const int32_t cell : band.cells) {
        plates.clear();
        const auto gather = [&](int c) {
            for (int32_t k = coupling.cellStart[size_t(c)]; k < coupling.cellStart[size_t(c) + 1]; ++k) {
                const int plate = globe.pieces[size_t(coupling.byCell[size_t(k)].piece)].plate;
                if (std::find(plates.begin(), plates.end(), plate) == plates.end()) plates.push_back(plate);
            }
        };
        gather(cell);
        for (const int32_t other : (*globe.rings)[size_t(cell)])
            if (other >= 0) gather(other);
        const Vec3 x = globe.grid->centre[size_t(cell)];
        for (size_t a = 0; a < plates.size(); ++a)
            for (size_t b = a + 1; b < plates.size(); ++b)
                fastest = std::max(fastest, length(velocityAt(globe.applied[size_t(plates[a])] - globe.applied[size_t(plates[b])], x)));
    }
    return fastest;
}

void turn(std::vector<CrustPiece>& pieces, const std::vector<Rotation>& step, const std::vector<int32_t>& which) {
    for (const int32_t k : which) {
        CrustPiece& piece = pieces[size_t(k)];
        piece.rotation = renormalized(compose(step[piece.plate], piece.rotation));
    }
}

bool continental(const CrustPiece& piece) { return piece.column.crust >= float(OceanicLimit); }

} // namespace

bool sinksUnder(const Globe& globe, int donor, int survivor) {
    const CrustPiece& d = globe.pieces[size_t(donor)];
    const CrustPiece& s = globe.pieces[size_t(survivor)];
    if (continental(d)) return false;
    if (continental(s)) return true;
    if (d.column.oceanAge != s.column.oceanAge) return d.column.oceanAge > s.column.oceanAge;
    return d.plate > s.plate;
}

Vec3 towards(Vec3 from, Vec3 to) {
    const Vec3 along = to - from * dot(from, to);
    const double size = length(along);
    return size > 0 ? along * (1.0 / size) : Vec3{};
}

void advancePlates(Globe& globe, double dt, int step, int refine) {
    auto mark = std::chrono::steady_clock::now();
    applyKinematics(globe, dt);
    const SphereGrid& grid = *globe.grid;
    const double shortest = shortestEdgeOf(grid);
    double fastest = 0;
    for (const Vec3& euler : globe.applied) fastest = std::max(fastest, length(euler) * PlanetRadius);
    const int sweep = std::max(1, int(std::ceil(fastest * dt / shortest)));

    Band band;
    markBand(globe, sweep, band);
    const int substeps = refine * substepCount(fastestRelative(globe, band), dt, shortest);
    clockPhase(globe, GlobePhase::Band, mark);
    std::vector<Rotation> turnBy(globe.applied.size());
    for (size_t p = 0; p < turnBy.size(); ++p) turnBy[p] = rotationOf(globe.applied[p], dt / substeps);

    for (int index = 0; index < substeps; ++index) {
        turn(globe.pieces, turnBy, band.pieces);
        coupleBand(globe, band);
        const Substep substep{step, index, dt / substeps};
        consumeInBand(globe, band, substep);
        bearInBand(globe, band, substep);
        ++globe.stats.substeps;
    }
    clockPhase(globe, GlobePhase::Substeps, mark);
    // Pieces away from the band turn by the same rotations in the same order, so two pieces with the
    // same history keep the same bits wherever they were.
    requireBandHeld(globe, band);
    std::vector<int32_t> rest;
    for (size_t k = 0; k < band.member.size(); ++k)
        if (!band.member[k]) rest.push_back(int32_t(k));
    for (int index = 0; index < substeps; ++index) turn(globe.pieces, turnBy, rest);

    clockPhase(globe, GlobePhase::Rest, mark);
    globe.stats.bandCells += long(band.cells.size());
    std::vector<CrustPiece> kept;
    std::vector<uint8_t> exact;
    kept.reserve(globe.pieces.size());
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        if (band.dead[k]) continue;
        kept.push_back(std::move(globe.pieces[k]));
        exact.push_back(band.member[k]);
    }
    // Births join in substep and cell order, not key order; the sort carries the exact flags along.
    std::vector<size_t> order(kept.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return kept[a].key < kept[b].key; });
    globe.pieces.clear();
    std::vector<uint8_t> flags;
    for (const size_t i : order) globe.pieces.push_back(std::move(kept[i])), flags.push_back(exact[i]);
    for (const uint8_t f : flags) globe.stats.exactPieces += f;
    couple(grid, globe.pieces, flags, false, globe.coupling);
    clockPhase(globe, GlobePhase::Couple, mark);
}

} // namespace derivean::sim

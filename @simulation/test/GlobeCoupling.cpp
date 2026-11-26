// Crust pieces and their coupling to the mantle grid (S3-design.md §4, §11 tests 1–3): a globe at rest
// stays bit for bit and couples as the identity; a rigid turn of the whole crust by 2π returns every
// piece and keeps the exact overlaps summing to each footprint in every substep; a continent carried
// across the face seam 0|4 and over a cube vertex keeps its stock, cover and footprint, its outline in
// the view and a continuous material frame.
#include "GlobeFixture.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Overlap.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Pieces.hpp"
#include "planet/fn/Rotate.hpp"

#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

constexpr double Pi = std::numbers::pi;
constexpr int Level = 5;

Vec3 fromLatLon(double latitude, double longitude) {
    const double phi = latitude * Pi / 180, lambda = longitude * Pi / 180;
    return {std::cos(phi) * std::cos(lambda), std::cos(phi) * std::sin(lambda), std::sin(phi)};
}

double crustView(const Globe& globe, int cell) {
    return viewOf(globe.coupling, globe.pieces, cell, [](const CrustPiece& piece) { return piece.column.crust; });
}

// A continent of 30 km crust round `centre` within `radius` m, carried rigidly about the z axis by
// `turn` rad in substeps; everything else ocean.
void carryContinent(Vec3 centre, double radius, double turn) {
    Globe globe = makeGlobe(3, Level, PlateSpeeds::Today, TectonicsStart);
    const SphereGrid& grid = *globe.grid;
    std::vector<int> patch;
    double patchArea = 0;
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        CrustPiece& piece = globe.pieces[k];
        const bool inside = distanceBetween(grid.centre[size_t(piece.birth)], centre) < radius;
        piece.stock.felsic = inside ? 30000.0 * piece.area : 0.0;
        piece.record.cover = Cover{};
        if (inside) {
            addDeposit(piece.record.cover, TectonicsStart, 123.0f + float(k % 7), Facies{0, 100, 155, 0, 0, 0});
            patch.push_back(int(k));
            patchArea += piece.area;
        }
        refreshColumn(piece);
    }
    // The outline may move by up to one ring of cells along its perimeter.
    double perimeter = 0;
    for (const int k : patch)
        for (int e = 0; e < 4; ++e)
            if (globe.pieces[size_t(neighbourOf(grid, k, e))].column.crust < 1.0f) {
                perimeter += grid.area[size_t(k)];
                break;
            }
    const std::vector<CrustPiece> before = globe.pieces;
    onePlate(globe, Vec3{0, 0, turn / (40 * PlanetStep)});
    for (int step = 0; step < 40; ++step) rotatePieces(globe.pieces, globe.applied, PlanetStep);
    couple(grid, globe.pieces, {}, true, globe.coupling);

    Sum total, startTotal;
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        const CrustPiece& piece = globe.pieces[k];
        expect(sameBits(piece.stock, before[k].stock) && sameBits(piece.record.cover, before[k].record.cover), "a turn leaves stock and cover bit for bit");
        expect(std::abs(areaOf(footprintOf(grid, piece)) / piece.area - 1) < 1e-12, "a turn keeps the footprint's area");
        total.add(piece.stock.felsic);
        startTotal.add(before[k].stock.felsic);
    }
    expect(total.value() == startTotal.value(), "the continent's volume is unchanged");
    double mask = 0;
    for (int cell = 0; cell < grid.cells; ++cell)
        if (crustView(globe, cell) > 15000) mask += grid.area[size_t(cell)];
    expect(std::abs(mask - patchArea) <= perimeter, "the outline keeps its shape within a ring of cells", (mask - patchArea) / perimeter);

    // The material frame along a line through the carried continent, across the seam.
    const Vec3 middle = rotate(rotationAbout({0, 0, 1}, turn), centre);
    const Vec3 east = normalized(cross(Vec3{0, 0, 1}, middle));
    Vec3 previousPoint, previousMaterial;
    for (int sample = 0; sample <= 60; ++sample) {
        const double angle = (sample / 60.0 - 0.5) * 0.8 * radius / PlanetRadius;
        const Vec3 point = rotate(rotationAbout(cross(middle, east), angle), middle);
        const int owner = dominantPiece(globe.coupling, indexOf(grid, cellOf(point, Level)));
        expect(owner >= 0 && globe.pieces[size_t(owner)].column.crust > 1.0f, "the line stays on the continent", owner);
        const Vec3 material = materialOf(globe.pieces[size_t(owner)], point);
        if (sample > 0) {
            const double jump = length((material - previousMaterial) - rotate(inverse(globe.pieces[size_t(owner)].rotation), point - previousPoint) * PlanetRadius);
            expect(jump < 1e-6, "the material frame is continuous across the seam", jump);
        }
        previousPoint = point, previousMaterial = material;
    }
}

} // namespace

namespace derivean::sim::globe_test {

void checkZeroRotation() {
    Globe globe = makeGlobe(11, Level, PlateSpeeds::Today, TectonicsStart);
    const std::vector<CrustPiece> before = globe.pieces;
    globe.applied.assign(1, Vec3{});
    for (CrustPiece& piece : globe.pieces) piece.plate = 0;
    for (int step = 0; step < 100; ++step) rotatePieces(globe.pieces, globe.applied, PlanetStep);
    for (size_t k = 0; k < before.size(); ++k)
        expect(sameBits(globe.pieces[k].rotation, before[k].rotation) && sameBits(globe.pieces[k].stock, before[k].stock), "a globe at rest stays bit for bit");
    couple(*globe.grid, globe.pieces, {}, true, globe.coupling);
    for (size_t k = 0; k < before.size(); ++k)
        for (int32_t s = globe.coupling.pieceStart[k]; s < globe.coupling.pieceStart[k + 1]; ++s) {
            const CellWeight& w = globe.coupling.byPiece[size_t(s)];
            const double expected = w.cell == globe.pieces[k].birth ? globe.pieces[k].area : 0.0;
            expect(std::abs(w.area - expected) <= 1e-12 * globe.pieces[k].area, "at rest the coupling is the identity", w.area / globe.pieces[k].area);
        }
}

void checkRigidTurn() {
    Globe globe = makeGlobe(5, Level, PlateSpeeds::Today, TectonicsStart);
    const SphereGrid& grid = *globe.grid;
    for (CrustPiece& piece : globe.pieces) piece.stock.felsic = 10000.0 * piece.area, refreshColumn(piece);
    Random random{9};
    constexpr int Steps = 40;
    onePlate(globe, random.direction() * (2 * Pi / (Steps * PlanetStep)));
    std::vector<CellWeight> weights;
    std::vector<int> cells, work;
    for (int step = 0; step < Steps; ++step) {
        rotatePieces(globe.pieces, globe.applied, PlanetStep);
        for (const CrustPiece& piece : globe.pieces) {
            const double sum = exactWeights(grid, piece, weights, cells, work);
            expect(std::abs(sum - piece.area) <= 1e-12 * piece.area, "exact overlaps sum to the footprint in every substep", sum / piece.area - 1);
        }
        if (step % 10 != 9) continue;
        couple(grid, globe.pieces, {}, true, globe.coupling);
        for (int cell = 0; cell < grid.cells; ++cell) {
            expect(std::abs(globe.coupling.covered[size_t(cell)] / grid.area[size_t(cell)] - 1) < 1e-9, "a rigid turn covers every cell once", globe.coupling.covered[size_t(cell)]);
            expect(std::abs(crustView(globe, cell) - 10000.0) < 1e-6, "the view of a constant field is constant", crustView(globe, cell));
        }
    }
    for (const CrustPiece& piece : globe.pieces)
        expect(length(centreOf(grid, piece) - grid.centre[size_t(piece.birth)]) < 1e-12, "a turn by 2π returns every piece", length(centreOf(grid, piece) - grid.centre[size_t(piece.birth)]));
}

void checkSeamCrossing() {
    // From longitude 20° on the equator (face 0) to -45°, the seam between faces 0 and 4.
    carryContinent(fromLatLon(0, 20), 900e3, -65 * Pi / 180);
    // Onto the cube vertex (1, -1, 1)/√3 at 35.26° N, 45° W, where faces 0, 2 and 4 meet.
    carryContinent(fromLatLon(35.2644, 15), 900e3, -60 * Pi / 180);
}

} // namespace derivean::sim::globe_test

void testGlobeCoupling() {
    checkZeroRotation();
    checkRigidTurn();
    checkSeamCrossing();
}

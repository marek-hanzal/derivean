#include "planet/fn/Pieces.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "geometry/fn/SpherePolygon.hpp"
#include "planet/constant/Globe.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace derivean::sim {
namespace {

constexpr uint64_t PlumeStream = 0x504c554d;    // the flat Primordial.cpp's stream
constexpr double PlumeWavelength = PlanetSpan / 4;   // m, the flat noise's lattice

double plume(uint64_t seed, Vec3 direction) {
    const Vec3 p = direction * (PlanetRadius / PlumeWavelength);
    return fractalNoise3(stream(seed, PlumeStream), p.x, p.y, p.z, 4, 0.55);
}

double quenchTimeAt(uint64_t seed, Vec3 direction) { return MagmaOceanEnd + 24 * Myr * smoothstep(0.08, 0.62, plume(seed, direction)); }

} // namespace

CrustColumn primordialColumnAt(uint64_t seed, Vec3 direction, int cell, double time) {
    CrustColumn column;
    const double heat = plume(seed, direction);
    column.oceanAge = float(std::max(0.0, time - quenchTimeAt(seed, direction)));
    column.quietSince = float(time - column.oceanAge);
    const double nucleus = smoothstep(0.30, 0.66, heat) * smoothstep(20 * Myr, TectonicsStart, time);
    column.crust = float(27000.0 * nucleus);
    column.basement = nucleus > 0.3 ? Basement::Craton : Basement::Primordial;
    column.fabric = float(6.283185307179586 * unit(hash(seed, cell, 77)));
    return column;
}

Globe makeGlobe(uint64_t seed, int level, PlateSpeeds speeds, double time) {
    Globe globe;
    globe.seed = seed;
    globe.speeds = speeds;
    globe.time = std::min(time, TectonicsStart);
    const auto grid = std::make_shared<const SphereGrid>(makeSphereGrid(level));
    globe.grid = grid;
    for (SphereField<float>* field : {&globe.elevation, &globe.swell, &globe.meltHeat, &globe.erosion, &globe.deposition, &globe.temperature})
        *field = sphereField<float>(level);
    globe.precipitation = sphereField<float>(level, 0.8f);
    globe.owner = sphereField<uint16_t>(level);
    globe.contact = sphereField<GlobeContact>(level);
    globe.pieces.resize(size_t(grid->cells));
    const double newest = std::floor(globe.time / RegionAnchor) * RegionAnchor;
    for (int cell = 0; cell < grid->cells; ++cell) {
        CrustPiece& piece = globe.pieces[size_t(cell)];
        const Vec3 centre = grid->centre[size_t(cell)];
        piece.key = {-1, cell, 0};
        piece.birth = cell;
        piece.area = grid->area[size_t(cell)];
        piece.column = primordialColumnAt(seed, centre, cell, globe.time);
        piece.stock = {double(piece.column.crust) * piece.area, OceanicCrust * piece.area};
        refreshColumn(piece);
        // The cooling body has a closed-form past, so its marks are evaluated rather than remembered.
        for (int back = 0; back < PastMarks; ++back) {
            const double anchor = std::max(0.0, newest - back * RegionAnchor);
            const CrustColumn then = primordialColumnAt(seed, centre, cell, anchor);
            piece.record.past[size_t(pastMarkIndex(anchor))] = {columnElevation(then), then.crust, 0.0f, 0.0f, 0.0f};
        }
    }
    return globe;
}

Vec3 centreOf(const SphereGrid& grid, const CrustPiece& piece) { return rotate(piece.rotation, grid.centre[size_t(piece.birth)]); }

SpherePolygon footprintOf(const SphereGrid& grid, const CrustPiece& piece) { return rotated(polygonOf(grid, piece.birth), piece.rotation); }

void refreshColumn(CrustPiece& piece) { piece.column.crust = float(piece.stock.felsic / piece.area); }

double takeFelsic(CrustPiece& piece, double volume) {
    const double taken = std::clamp(volume, 0.0, piece.stock.felsic);
    piece.stock.felsic -= taken;
    refreshColumn(piece);
    return taken;
}

void giveFelsic(CrustPiece& piece, double volume) {
    piece.stock.felsic += std::max(0.0, volume);
    refreshColumn(piece);
}

void globeFailure(const char* what, double value) {
    std::fprintf(stderr, "globe: %s (%.17g)\n", what, value);
    std::abort();
}

void sortPieces(std::vector<CrustPiece>& pieces) {
    const auto byKey = [](const CrustPiece& a, const CrustPiece& b) { return a.key < b.key; };
    if (!std::is_sorted(pieces.begin(), pieces.end(), byKey)) std::stable_sort(pieces.begin(), pieces.end(), byKey);
}

void Sum::add(double value) {
    const double next = total + value;
    carry += std::abs(total) >= std::abs(value) ? (total - next) + value : (value - next) + total;
    total = next;
}

CrustStock totalStock(const std::vector<CrustPiece>& pieces) {
    Sum felsic, basalt;
    for (const CrustPiece& piece : pieces) felsic.add(piece.stock.felsic), basalt.add(piece.stock.basalt);
    return {felsic.value(), basalt.value()};
}

double totalArea(const std::vector<CrustPiece>& pieces) {
    Sum area;
    for (const CrustPiece& piece : pieces) area.add(piece.area);
    return area.value();
}

} // namespace derivean::sim

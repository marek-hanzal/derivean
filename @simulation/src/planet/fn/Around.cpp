#include "planet/fn/Around.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr int Around = 8;           // pieces of the ordinary reach
constexpr int WideAround = 64;      // pieces of the wide reach
constexpr double WideReach = 1e6;   // m of great circle between the donor's centre and a piece's (chosen: about three level-5 cells)

std::vector<int> nearestFirst(const Globe& globe, const std::vector<std::pair<double, int>>& found, int limit) {
    std::vector<std::pair<double, int>> sorted = found;
    std::sort(sorted.begin(), sorted.end(), [&](const auto& a, const auto& c) {
        return a.first != c.first ? a.first < c.first : globe.pieces[size_t(a.second)].key < globe.pieces[size_t(c.second)].key;
    });
    std::vector<int> around;
    for (size_t i = 0; i < sorted.size() && int(i) < limit; ++i) around.push_back(sorted[i].second);
    return around;
}

} // namespace

std::vector<int> aroundOf(const Globe& globe, const Band& band, int k, const std::vector<int>& cells, const std::vector<int>& skip) {
    const CrustPiece& piece = globe.pieces[size_t(k)];
    const Vec3 x = centreOf(*globe.grid, piece);
    std::vector<std::pair<double, int>> found;
    const auto consider = [&](int l, double) {
        if (l == k || globe.pieces[size_t(l)].plate != piece.plate || std::find(skip.begin(), skip.end(), l) != skip.end()) return;
        if (std::find_if(found.begin(), found.end(), [&](const auto& f) { return f.second == l; }) != found.end()) return;
        const Vec3 d = centreOf(*globe.grid, globe.pieces[size_t(l)]) - x;
        found.push_back({dot(d, d), l});
    };
    for (const int cell : cells) {
        forLive(band, cell, consider);
        for (const int32_t other : (*globe.rings)[size_t(cell)])
            if (other >= 0) forLive(band, other, consider);
    }
    return nearestFirst(globe, found, Around);
}

std::vector<int> wideAroundOf(const Globe& globe, const Band& band, int k, const std::vector<int>& cells, const std::vector<int>& skip, const Substep& substep) {
    const SphereGrid& grid = *globe.grid;
    const CrustPiece& piece = globe.pieces[size_t(k)];
    const Vec3 x = centreOf(grid, piece);
    // A piece outside the band turns only after the substeps, so in substep i it stands i + 1 turns of
    // its plate behind the band's pieces; its centre is turned on to the substep's time for the reach.
    const int behind = substep.index + 1;
    std::vector<Rotation> turn(globe.applied.size());
    for (size_t p = 0; p < turn.size() && behind > 0; ++p) turn[p] = rotationOf(globe.applied[p], substep.duration);
    const auto centreNow = [&](int l) {
        Vec3 centre = centreOf(grid, globe.pieces[size_t(l)]);
        if (!band.member[size_t(l)])
            for (int t = 0; t < behind; ++t) centre = rotate(turn[globe.pieces[size_t(l)].plate], centre);
        return centre;
    };
    // The cells round the ordinary reach's and every cell whose centre lies within WideReach and a margin
    // of the step's motion and a footprint's size, so the coupling lists every piece that may lie within
    // WideReach at the substep's time: a superset of both reaches, filtered by the pieces' centres.
    const double margin = 3.0 * double(band.sweep + 2) * grid.reach[size_t(piece.birth)] * PlanetRadius;
    std::vector<int> near(cells);
    for (size_t i = 0; i < near.size(); ++i)
        for (const int32_t other : (*globe.rings)[size_t(near[i])])
            if (other >= 0 && std::find(near.begin(), near.end(), other) == near.end() &&
                (i < cells.size() || length(grid.centre[size_t(other)] - x) * PlanetRadius < WideReach + margin))
                near.push_back(other);
    const double chord = 2 * std::sin(WideReach / (2 * PlanetRadius));   // of WideReach on the unit sphere
    std::vector<std::pair<double, int>> found;
    const auto consider = [&](int l, bool ordinary) {
        if (l == k || band.dead[size_t(l)] || globe.pieces[size_t(l)].plate != piece.plate) return;
        if (std::find(skip.begin(), skip.end(), l) != skip.end()) return;
        if (std::find_if(found.begin(), found.end(), [&](const auto& f) { return f.second == l; }) != found.end()) return;
        const Vec3 d = centreNow(l) - x;
        if (!ordinary && dot(d, d) > chord * chord) return;
        found.push_back({dot(d, d), l});
    };
    // The ordinary reach keeps its contract (aroundOf's pieces, whatever their distance); every other
    // piece must lie within WideReach.
    for (const int l : aroundOf(globe, band, k, cells, skip)) consider(l, true);
    for (const int cell : near) {
        forLive(band, cell, [&](int l, double) { consider(l, false); });
        for (int32_t s = globe.coupling.cellStart[size_t(cell)]; s < globe.coupling.cellStart[size_t(cell) + 1]; ++s) consider(globe.coupling.byCell[size_t(s)].piece, false);
    }
    return nearestFirst(globe, found, WideAround);
}

} // namespace derivean::sim

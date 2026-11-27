#include "planet/fn/Around.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>

namespace derivean::sim {
namespace {

constexpr int Around = 8;           // pieces of the ordinary reach
constexpr int WideAround = 64;      // pieces of the wide reach
constexpr double WideReach = 1e6;   // m from the donor's centre (chosen: about three level-5 cells)

std::vector<int> nearestFirst(const Globe& globe, const CrustPiece& piece, const std::vector<int>& found, int limit) {
    const Vec3 x = centreOf(*globe.grid, piece);
    std::vector<std::pair<double, int>> sorted;
    for (const int l : found) {
        const Vec3 d = centreOf(*globe.grid, globe.pieces[size_t(l)]) - x;
        sorted.push_back({dot(d, d), l});
    }
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
    std::vector<int> found;
    const auto consider = [&](int l, double) {
        if (l == k || globe.pieces[size_t(l)].plate != piece.plate || std::find(skip.begin(), skip.end(), l) != skip.end()) return;
        if (std::find(found.begin(), found.end(), l) == found.end()) found.push_back(l);
    };
    for (const int cell : cells) {
        forLive(band, cell, consider);
        for (const int32_t other : (*globe.rings)[size_t(cell)])
            if (other >= 0) forLive(band, other, consider);
    }
    return nearestFirst(globe, piece, found, Around);
}

std::vector<int> wideAroundOf(const Globe& globe, const Band& band, int k, const std::vector<int>& cells, const std::vector<int>& skip) {
    const SphereGrid& grid = *globe.grid;
    const CrustPiece& piece = globe.pieces[size_t(k)];
    const Vec3 x = centreOf(grid, piece);
    // The cells and rings of the ordinary reach, then further rings within WideReach: a superset of it.
    std::vector<int> near(cells), found;
    for (size_t i = 0; i < near.size(); ++i)
        for (const int32_t other : (*globe.rings)[size_t(near[i])])
            if (other >= 0 && std::find(near.begin(), near.end(), other) == near.end() && (i < cells.size() || length(grid.centre[size_t(other)] - x) * PlanetRadius < WideReach))
                near.push_back(other);
    const auto consider = [&](int l) {
        if (l == k || band.dead[size_t(l)] || globe.pieces[size_t(l)].plate != piece.plate) return;
        if (std::find(skip.begin(), skip.end(), l) == skip.end() && std::find(found.begin(), found.end(), l) == found.end()) found.push_back(l);
    };
    for (const int cell : near) {
        forLive(band, cell, [&](int l, double) { consider(l); });
        for (int32_t s = globe.coupling.cellStart[size_t(cell)]; s < globe.coupling.cellStart[size_t(cell) + 1]; ++s) consider(globe.coupling.byCell[size_t(s)].piece);
    }
    return nearestFirst(globe, piece, found, WideAround);
}

} // namespace derivean::sim

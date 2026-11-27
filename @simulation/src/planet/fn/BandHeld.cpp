#include "planet/fn/BandHeld.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// The cells within `count` rings of one cell (a few dozen); `seen` is a per-cell stamp the caller keeps,
// `stamp` a value it holds for no cell yet.
void around(const std::vector<CellRing>& rings, int cell, int count, std::vector<int32_t>& out, std::vector<uint32_t>& seen, uint32_t stamp) {
    out.assign(1, cell);
    seen[size_t(cell)] = stamp;
    for (int ring = 0, from = 0; ring < count; ++ring) {
        const int to = int(out.size());
        for (int i = from; i < to; ++i)
            for (const int32_t other : rings[size_t(out[size_t(i)])])
                if (other >= 0 && seen[size_t(other)] != stamp) seen[size_t(other)] = stamp, out.push_back(other);
        from = to;
    }
}

} // namespace

void requireBandHeld(const Globe& globe, const Band& band, const std::vector<int32_t>& near, double dt, double shortest) {
    const SphereGrid& grid = *globe.grid;
    const double between = 0.25 * shortest / PlanetRadius;   // of a point of the arc from the nearest sample
    std::vector<int32_t> cells;
    std::vector<uint32_t> seen(size_t(grid.cells), 0);
    uint32_t stamp = 0;
    // Per plate the turns back to each sample, the same for all its pieces.
    std::vector<std::vector<Rotation>> back(globe.applied.size());
    for (const int32_t k : near) {
        const CrustPiece& piece = globe.pieces[size_t(k)];
        const Vec3 euler = globe.applied[piece.plate];
        const int samples = std::max(1, int(std::ceil(length(euler) * PlanetRadius * dt / (0.5 * shortest))));
        std::vector<Rotation>& turns = back[piece.plate];
        if (turns.empty())
            for (int j = 1; j <= samples; ++j) turns.push_back(rotationOf(euler, -dt * j / samples));
        for (int j = 0; j <= samples; ++j) {
            const Rotation turned = j == 0 ? piece.rotation : compose(turns[size_t(j - 1)], piece.rotation);
            const Vec3 at = rotate(turned, grid.centre[size_t(piece.birth)]);
            around(*globe.rings, indexOf(grid, cellOf(at, grid.level)), GuardRings, cells, seen, ++stamp);
            for (const int32_t c : cells) {
                if (!band.decide[size_t(c)]) continue;
                const Vec3 d = at - grid.centre[size_t(c)];
                const double apart = grid.reach[size_t(piece.birth)] + grid.reach[size_t(c)] + between;
                require(dot(d, d) >= apart * apart, "a piece outside the band reaches a deciding cell", double(k));
            }
        }
    }
}

} // namespace derivean::sim

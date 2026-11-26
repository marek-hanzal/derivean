#include "groundwater/fn/Refine.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {
namespace {

constexpr double CellReach = 46.0;   // m a vent may lie from where the local level put it: half the diagonal of a local cell

float groundOf(const Tile& tile, size_t cell) {
    return tile.now.rock[cell] + tile.now.sediment[Coarse][cell] + tile.now.sediment[Sand][cell] + tile.now.sediment[Fines][cell];
}

} // namespace

std::shared_ptr<const GroundwaterTile> groundwaterAt(World& world, TileKey local) { return world.groundwater.require(world.atlas, local); }

std::shared_ptr<const CaveField> caveFieldAt(World& world, Vec2 position) { return groundwaterAt(world, tileAt(Level::Local, position))->caves; }

Spring refineVent(World& world, const GroundwaterTile& tile, size_t index) {
    Spring spring = tile.springs[index];
    const SpringRecord& record = tile.records[index];
    const double fine = specOf(Level::Site).cell;
    const int64_t localX = int64_t(tile.key.x) * TileCells + record.cell % TileCells, localZ = int64_t(tile.key.z) * TileCells + record.cell / TileCells;
    const int64_t firstX = localX * Refinement, firstZ = localZ * Refinement;
    const std::shared_ptr<const Tile> site = world.atlas.require(tileAt(Level::Site, {(double(firstX) + 0.5) * fine, (double(firstZ) + 0.5) * fine}));
    const int64_t tileX = int64_t(site->key.x) * TileCells, tileZ = int64_t(site->key.z) * TileCells;
    const auto cellAt = [&](int64_t x, int64_t z) { return size_t(z - tileZ) * TileCells + size_t(x - tileX); };
    if (record.kind == SpringKind::Karst) {
        const int64_t x = std::clamp(int64_t(std::floor(spring.position.x / fine)), firstX, firstX + Refinement - 1);
        const int64_t z = std::clamp(int64_t(std::floor(spring.position.z / fine)), firstZ, firstZ + Refinement - 1);
        spring.elevation = groundOf(*site, cellAt(x, z));
        return spring;
    }
    // The lowest ground of the cell, or for water a bed turns out the ground nearest the height of
    // the bed's lip; of equals the one nearest to where the local level put the vent.
    double best = 1e30;
    for (int64_t z = firstZ; z < firstZ + Refinement; ++z)
        for (int64_t x = firstX; x < firstX + Refinement; ++x) {
            const float ground = groundOf(*site, cellAt(x, z));
            const Vec2 centre{(double(x) + 0.5) * fine, (double(z) + 0.5) * fine};
            const double misfit = (record.kind == SpringKind::Contact ? std::abs(double(ground) - tile.springs[index].elevation) : double(ground)) + 0.01 * length(centre - tile.springs[index].position);
            if (misfit >= best) continue;
            best = misfit;
            spring.position = centre, spring.elevation = ground;
        }
    return spring;
}

std::vector<Spring> springsIn(World& world, Vec2 low, Vec2 high) {
    std::vector<Spring> springs;
    const TileKey first = tileAt(Level::Local, {low.x - CellReach, low.z - CellReach}), last = tileAt(Level::Local, {high.x + CellReach, high.z + CellReach});
    for (int z = first.z; z <= last.z; ++z)
        for (int x = first.x; x <= last.x; ++x) {
            const std::shared_ptr<const GroundwaterTile> tile = groundwaterAt(world, {Level::Local, x, z});
            for (size_t index = 0; index < tile->springs.size(); ++index) {
                const Vec2 coarse = tile->springs[index].position;
                if (coarse.x < low.x - CellReach || coarse.z < low.z - CellReach || coarse.x >= high.x + CellReach || coarse.z >= high.z + CellReach) continue;
                const Spring spring = refineVent(world, *tile, index);
                if (spring.position.x >= low.x && spring.position.z >= low.z && spring.position.x < high.x && spring.position.z < high.z) springs.push_back(spring);
            }
        }
    return springs;
}

} // namespace derivean::sim

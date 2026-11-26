#include "groundwater/fn/Caves.hpp"
#include "groundwater/fn/Refine.hpp"
#include "groundwater/type/Aquifer.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {
namespace {

constexpr int LongestPath = 512;   // cells a spring's water is traced back
constexpr int MostRocks = 6;

// Rock along the way most of a vent's water came, from the vent upstream, each kind once.
std::vector<Material> crossedBy(const GroundwaterTile& tile, int32_t cell, int storey) {
    std::vector<Material> crossed;
    int x = cell % TileCells, z = cell / TileCells;
    for (int step = 0; step < LongestPath && int(crossed.size()) < MostRocks; ++step) {
        const size_t at = size_t(z) * TileCells + size_t(x);
        const Material rock = Material(tile.rock[storey][at]);
        if (rock != Material::Air && std::find(crossed.begin(), crossed.end(), rock) == crossed.end()) crossed.push_back(rock);
        const uint8_t from = tile.from[storey][at];
        if (from == 8) {
            storey = Upper;
            continue;
        }
        if (from > 8) break;
        x += NeighborX[from], z += NeighborZ[from];
        if (x < 0 || z < 0 || x >= TileCells || z >= TileCells) break;
    }
    return crossed;
}

} // namespace

SpringAccount explainSpring(World& world, Vec2 position, double reach) {
    SpringAccount account;
    const double wide = reach + 64.0;
    const TileKey first = tileAt(Level::Local, {position.x - wide, position.z - wide}), last = tileAt(Level::Local, {position.x + wide, position.z + wide});
    double nearest = reach;
    for (int z = first.z; z <= last.z; ++z)
        for (int x = first.x; x <= last.x; ++x) {
            const std::shared_ptr<const GroundwaterTile> tile = groundwaterAt(world, {Level::Local, x, z});
            for (size_t index = 0; index < tile->springs.size(); ++index) {
                if (length(tile->springs[index].position - position) > wide) continue;
                const Spring spring = refineVent(world, *tile, index);
                const double away = length(spring.position - position);
                if (away > nearest) continue;
                nearest = away;
                account.found = true;
                account.spring = spring, account.record = tile->records[index];
                account.crossed = crossedBy(*tile, account.record.cell, account.record.storey);
            }
        }
    return account;
}

CaveAccount explainCave(World& world, double x, double y, double z) {
    CaveAccount account;
    const std::shared_ptr<const GroundwaterTile> tile = groundwaterAt(world, tileAt(Level::Local, {x, z}));
    const int index = conduitAt(*tile->caves, x, y, z);
    if (index < 0) return account;
    account.found = true;
    account.conduit = tile->caves->conduits[size_t(index)];
    account.record = tile->passages[size_t(index)];
    for (size_t spring = 0; spring < tile->springs.size(); ++spring)
        if (tile->springs[spring].id == account.record.spring) account.outlet = refineVent(world, *tile, spring);
    return account;
}

} // namespace derivean::sim

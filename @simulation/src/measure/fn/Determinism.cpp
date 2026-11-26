#include "measure/fn/Determinism.hpp"

#include "measure/fn/Dependencies.hpp"
#include "simulation/fn/Query.hpp"

#include <future>

namespace derivean::sim {
namespace {

constexpr int BlockEdge = 16;   // voxels per side of the sampled block, a metre each

// The tile of a level on or next to the focus that is solved in the given pass.
TileKey tileOfPass(Level level, Vec2 focus, int pass) {
    const TileKey home = tileAt(level, focus);
    for (int dz = 0; dz <= 1; ++dz)
        for (int dx = 0; dx <= 1; ++dx)
            if (passOf({level, home.x + dx, home.z + dz}) == pass) return {level, home.x + dx, home.z + dz};
    return home;
}

void appendOnce(std::vector<TileKey>& keys, TileKey key) {
    if (std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
}

// The first-pass ground tile at or next to a point. A block of voxels centred on the middle of its
// east edge is cut from two ground tiles, the second of which is solved in a later pass.
TileKey blockTile(Vec2 near) { return tileOfPass(Level::Ground, near, 0); }

std::vector<Material> blockAt(World& world, Vec2 near) {
    const TileKey tile = blockTile(near);
    const Vec2 middle{(tile.x + 1) * tileSpan(Level::Ground), (tile.z + 0.5) * tileSpan(Level::Ground)};
    const float ground = surveyAt(world, middle, Level::Ground).ground;
    std::vector<Material> materials(size_t(BlockEdge) * BlockEdge * BlockEdge);
    sampleBlock(world, {middle.x - 0.5 * BlockEdge, std::floor(ground) - 0.75 * BlockEdge, middle.z - 0.5 * BlockEdge, 1.0, BlockEdge, BlockEdge, BlockEdge}, materials.data());
    return materials;
}

size_t changed(const std::vector<Material>& a, const std::vector<Material>& b) {
    size_t count = a.size() == b.size() ? 0 : std::max(a.size(), b.size());
    for (size_t i = 0; i < std::min(a.size(), b.size()); ++i) count += a[i] != b[i];
    return count;
}

struct Fresh {
    std::vector<std::shared_ptr<const Tile>> tiles;
    std::vector<Material> block;
    uint64_t planet{};
    double eroded{};
};

} // namespace

bool DeterminismReport::identical() const {
    if (!planetSame || replayed.cells || blockRevisited || blockFresh) return false;
    for (const Entry& entry : tiles)
        if (entry.difference.cells) return false;
    return true;
}

DeterminismReport checkDeterminism(World& first, Vec2 focus, Level finest, Level orderLevel, int orderPass, bool thorough) {
    DeterminismReport report;
    const TileKey ordered = tileOfPass(orderLevel, focus, orderPass);
    const TileKey deepest = tileAt(finest, focus);
    const bool voxels = finest == Level::Site;

    // Every tile compared: the ordered tile, the chain down to the focus, and what they wait for.
    std::vector<TileKey> keys{ordered};
    for (const TileKey& other : dependencies(ordered)) appendOnce(keys, other);
    for (int level = int(Level::Region); level <= int(finest); ++level) {
        const TileKey link = tileAt(Level(level), focus);
        appendOnce(keys, link);
        for (const TileKey& other : dependencies(link)) appendOnce(keys, other);
    }

    WorldOptions options = first.options;
    options.threads = 1;
    std::future<Fresh> other = std::async(std::launch::async, [options, keys, focus, voxels] {
        Fresh fresh;
        World second(options);
        fresh.tiles.resize(keys.size());
        for (size_t i = keys.size(); i-- > 0;) fresh.tiles[i] = second.atlas.require(keys[i]);
        if (voxels) fresh.block = blockAt(second, focus);
        fresh.planet = digestOf(second.planet);
        fresh.eroded = second.planet.ledger.eroded;
        return fresh;
    });

    first.atlas.require(deepest);
    // Half a metre inside the east edge of the first-pass site tile at the focus, between two cell
    // centres, so the cells across the edge carry weight. Its eastern neighbour is solved in a
    // later pass; where something has built it already there is nothing to compare.
    const TileKey probe = tileOfPass(Level::Site, focus, 0);
    const Vec2 edge{(probe.x + 1) * tileSpan(Level::Site) - 0.5, (probe.z + 0.5) * tileSpan(Level::Site) + 1.0};
    const bool measurable = voxels && !first.atlas.find({Level::Site, probe.x + 1, probe.z});
    const float alone = measurable ? surveyAt(first, edge, Level::Site).ground : 0.0f;
    const std::vector<Material> block = voxels ? blockAt(first, focus) : std::vector<Material>{};
    first.atlas.require(ordered);
    for (const TileKey& key : keys) first.atlas.require(key);
    if (measurable) {
        first.atlas.require({Level::Site, probe.x + 1, probe.z});
        report.surveyEdgeShift = std::abs(surveyAt(first, edge, Level::Site).ground - alone);
    }
    report.blockCells = block.size();
    if (voxels && thorough) {
        // Every ground tile around the two the block was cut from, then the same block again.
        const TileKey tile = blockTile(focus);
        for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 2; ++dx) first.atlas.require({Level::Ground, tile.x + dx, tile.z + dz});
        report.blockRevisitChecked = true;
        report.blockRevisited = changed(block, blockAt(first, focus));
    }

    // The same tile replayed a second time against the same finished neighbours.
    const Dependencies needed = requireDependencies(first, deepest);
    report.replayed = differenceOf(*first.atlas.require(deepest), *replayTile(first.atlas.setting(), deepest, needed.lookup()));

    const Fresh fresh = other.get();
    if (voxels) report.blockFresh = changed(block, fresh.block);
    report.planetDigest = digestOf(first.planet);
    report.planetSame = report.planetDigest == fresh.planet;
    const double eroded = std::max(std::abs(first.planet.ledger.eroded), std::abs(fresh.eroded));
    report.ledgerDrift = eroded > 0 ? std::abs(first.planet.ledger.eroded - fresh.eroded) / eroded : 0.0;
    for (size_t i = 0; i < keys.size(); ++i) {
        const auto built = first.atlas.find(keys[i]);
        if (built && fresh.tiles[i]) report.tiles.push_back({keys[i], digestOf(*built), differenceOf(*built, *fresh.tiles[i])});
    }
    return report;
}

} // namespace derivean::sim

#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Water.hpp"
#include "intervention/constant/Pace.hpp"
#include "landscape/fn/Forcing.hpp"

#include <limits>

namespace derivean::sim {
namespace {

bool seaAt(const Tile& tile, size_t cell, float ground) {
    return tile.water[cell] > ground && tile.water[cell] <= float(tile.seaLevel) + 0.5f;
}

// Calls each(cell in the group's grid, cell in the tile) for every cell of one tile.
template<typename Each>
void eachCell(const LiveSet& set, const EditedTile& tile, Each&& each) {
    const size_t baseX = size_t(tile.key.x - set.originX) * TileCells, baseZ = size_t(tile.key.z - set.originZ) * TileCells;
    for (size_t z = 0; z < size_t(TileCells); ++z)
        for (size_t x = 0; x < size_t(TileCells); ++x) each((baseZ + z) * size_t(set.grid.width) + baseX + x, z * TileCells + x);
}

// A tile's water as it stands in the tile; the rim holds none of its own.
void readWater(LiveSet& set, const EditedTile& tile) {
    eachCell(set, tile, [&](size_t to, size_t at) {
        const bool open = set.grid.kindOf(int32_t(to)) == CellKind::Open;
        set.grid.depth[to] = open ? tile.depth[at] : 0.0;
        set.grid.salt[to] = open ? tile.salt[at] : 0.0;
    });
}

void readColumns(LiveSet& set, const EditedTile& tile, const Ground& from) {
    WaterGrid& grid = set.grid;
    eachCell(set, tile, [&](size_t to, size_t at) {
        if (grid.kindOf(int32_t(to)) == CellKind::Rim) return;
        grid.solid.rock[to] = from.rock[at];
        grid.solid.datum[to] = from.datum[at];
        for (int c = 0; c < SedimentClasses; ++c) grid.solid.sediment[c][to] = from.sediment[c][at];
        grid.ground[to] = groundOf(grid.solid, to);
    });
}

// m, where a cell's water stands as the tile carries it: below the ground where it holds none.
float levelOf(float ground, double depth) { return depth > OpenWater ? float(double(ground) + depth) : ground - 1.0f; }

// A tile's live columns and water go into the grid, and the level the tile carries is derived from
// them there and then: queries read that level, also before any solver step stores the grid. The
// rim keeps the level it carries.
void readLive(LiveSet& set, EditedTile& tile) {
    readColumns(set, tile, tile.now);
    readWater(set, tile);
    eachCell(set, tile, [&](size_t from, size_t at) {
        if (set.grid.kindOf(int32_t(from)) != CellKind::Rim) tile.water[at] = levelOf(set.grid.ground[from], set.grid.depth[from]);
    });
}

// The grid of one group. It is first filled with the untouched tiles: they give the rim, the rivers
// that enter across it, and the discharge every bed is in balance with. The rim is the outermost
// ring of cells; it keeps the untouched ground and the untouched standing water, and where water
// crosses it inwards, on a bed or through a lake, that water's untouched flow and load enter. What
// happens upstream of the rim is not followed: the rivers that enter stay as the atlas has them.
// Then the live state goes in.
LiveSet buildSet(World& world, std::vector<EditedTile*> tiles) {
    LiveSet set;
    set.tiles = std::move(tiles);
    int32_t lowX = std::numeric_limits<int32_t>::max(), lowZ = lowX, highX = std::numeric_limits<int32_t>::min(), highZ = highX;
    for (const EditedTile* tile : set.tiles) {
        lowX = std::min(lowX, tile->key.x), highX = std::max(highX, tile->key.x);
        lowZ = std::min(lowZ, tile->key.z), highZ = std::max(highZ, tile->key.z);
    }
    set.originX = lowX, set.originZ = lowZ;
    set.tilesX = highX - lowX + 1, set.tilesZ = highZ - lowZ + 1;
    WaterGrid& grid = set.grid;
    shapeGrid(grid, set.tilesX * TileCells, set.tilesZ * TileCells, specOf(Level::Site).cell);
    std::fill(grid.kind.begin(), grid.kind.end(), uint8_t(CellKind::Outside));
    grid.sourceSalinity = RiverSalinity;
    for (const EditedTile* tile : set.tiles) {
        const Tile& pristine = *tile->pristine;
        eachCell(set, *tile, [&](size_t to, size_t) { grid.kind[to] = uint8_t(CellKind::Open); });
        readColumns(set, *tile, pristine.now);
        eachCell(set, *tile, [&](size_t to, size_t at) {
            grid.depth[to] = std::max(0.0, double(pristine.water[at]) - double(grid.ground[to]));
            grid.rain[to] = pristine.rain[at];
            grid.evaporation[to] = evaporationDemand(pristine.warmth[at]);
            grid.runoff[to] = runoffOf(pristine.rain[at], pristine.warmth[at]);
            grid.erodibility[to] = definitionOf(Material(pristine.rock[at])).erodibility;
        });
    }
    const std::vector<uint8_t> inside = grid.kind;
    for (size_t cell = 0; cell < grid.cells(); ++cell) {
        if (CellKind(inside[cell]) == CellKind::Outside) continue;
        for (int k = 0; k < 8; ++k) {
            const int32_t other = grid.neighbour(int32_t(cell), k);
            if (other < 0 || CellKind(inside[size_t(other)]) == CellKind::Outside) grid.kind[cell] = uint8_t(CellKind::Rim);
        }
    }
    for (const EditedTile* tile : set.tiles) {
        const Tile& pristine = *tile->pristine;
        eachCell(set, *tile, [&](size_t to, size_t at) {
            if (grid.kindOf(int32_t(to)) != CellKind::Rim) return;
            grid.held[to] = std::max(grid.ground[to], pristine.water[at]);
            grid.depth[to] = 0;
            grid.salinity[to] = float(RiverSalinity);
            if (seaAt(pristine, at, grid.ground[to])) {
                set.shore.push_back(int32_t(to));
                set.shoreLevel.push_back(pristine.water[at]);
                grid.salinity[to] = float(world.ocean.salinity);
            } else if (pristine.flowTo[at] != 255 && pristine.discharge[at] > 0) {
                const int32_t target = grid.neighbour(int32_t(to), pristine.flowTo[at]);
                if (target >= 0 && grid.kindOf(target) == CellKind::Open) grid.sources.push_back({target, pristine.discharge[at], pristine.load[at]});
            }
        });
    }
    const BasinTree untouched = buildBasins(grid);
    WaterFlows unused;
    const Lakes lakes = settle(grid, untouched, {}, unused);
    routeDischarge(grid, untouched, lakes);
    // A bed under standing water is in balance with nothing: once laid bare it cuts at full power.
    for (size_t cell = 0; cell < grid.cells(); ++cell) grid.balance[cell] = grid.depth[cell] > 0 ? 0.0f : float(streamPower(grid, untouched, cell));

    for (EditedTile* tile : set.tiles) readLive(set, *tile);
    return set;
}

} // namespace

std::vector<TileKey> tilesWithin(Vec2 low, Vec2 high) {
    const TileKey first = tileAt(Level::Site, low), last = tileAt(Level::Site, high);
    std::vector<TileKey> keys;
    for (int32_t z = first.z; z <= last.z; ++z)
        for (int32_t x = first.x; x <= last.x; ++x) keys.push_back({Level::Site, x, z});
    return keys;
}

EditedTile& activate(World& world, TileKey key) {
    Overlay& overlay = world.interventions.overlay;
    std::unique_ptr<EditedTile>& slot = overlay.tiles[{key.z, key.x}];
    if (slot) return *slot;
    slot = std::make_unique<EditedTile>();
    EditedTile& tile = *slot;
    tile.key = key;
    tile.pristine = world.atlas.require(key);
    tile.now = tile.pristine->now;
    tile.water = tile.pristine->water;
    tile.discharge = tile.pristine->discharge;
    const size_t cells = tile.water.size();
    const double area = specOf(Level::Site).cell * specOf(Level::Site).cell;
    tile.depth.assign(cells, 0.0);
    tile.salt.assign(cells, 0.0);
    for (size_t cell = 0; cell < cells; ++cell) {
        const float ground = groundOf(tile.now, cell);
        tile.depth[cell] = std::max(0.0, double(tile.water[cell]) - double(ground));
        tile.salt[cell] = tile.depth[cell] * (seaAt(*tile.pristine, cell, ground) ? world.ocean.salinity : RiverSalinity);
        overlay.adopted.water += tile.depth[cell] * area;
        overlay.adopted.salt += tile.salt[cell] * area;
    }
    return tile;
}

void regroup(World& world) {
    Overlay& overlay = world.interventions.overlay;
    const double now = world.interventions.calendar.seconds;
    std::map<std::pair<int32_t, int32_t>, int> groupOf;
    std::vector<std::vector<EditedTile*>> groups;
    for (const auto& entry : overlay.tiles) {
        if (groupOf.count(entry.first)) continue;
        std::vector<std::pair<int32_t, int32_t>> open{entry.first};
        groupOf[entry.first] = int(groups.size());
        for (size_t at = 0; at < open.size(); ++at) {
            for (int k = 0; k < 8; ++k) {
                const std::pair<int32_t, int32_t> other{open[at].first + NeighborZ[k], open[at].second + NeighborX[k]};
                if (!overlay.tiles.count(other) || groupOf.count(other)) continue;
                groupOf[other] = int(groups.size());
                open.push_back(other);
            }
        }
        std::sort(open.begin(), open.end());
        groups.emplace_back();
        for (const auto& key : open) groups.back().push_back(overlay.tiles[key].get());
    }
    std::vector<LiveSet> sets;
    for (std::vector<EditedTile*>& members : groups) {
        const auto same = std::find_if(overlay.sets.begin(), overlay.sets.end(), [&](const LiveSet& set) { return set.tiles == members; });
        bool changed = same == overlay.sets.end();
        if (changed) {
            sets.push_back(buildSet(world, members));
            sets.back().seconds = sets.back().shaped = now;
        } else {
            for (EditedTile* tile : members) {
                // An edit moves columns and may push water aside; both are read back.
                if (!tile->touched) continue;
                readLive(*same, *tile);
                changed = true;
            }
            sets.push_back(std::move(*same));
        }
        for (EditedTile* tile : members) tile->touched = false;
        if (!changed) continue;
        sets.back().stale = true;
        sets.back().disturbed = now;
        sets.back().awake = CalmSpan;
    }
    overlay.sets = std::move(sets);
}

void storeSet(LiveSet& set) {
    const WaterGrid& grid = set.grid;
    for (EditedTile* tile : set.tiles) {
        eachCell(set, *tile, [&](size_t from, size_t at) {
            if (grid.kindOf(int32_t(from)) == CellKind::Rim) {
                tile->water[at] = grid.held[from] > grid.ground[from] ? grid.held[from] : grid.ground[from] - 1.0f;
                return;
            }
            tile->now.rock[at] = grid.solid.rock[from];
            for (int c = 0; c < SedimentClasses; ++c) tile->now.sediment[c][at] = grid.solid.sediment[c][from];
            tile->depth[at] = grid.depth[from];
            tile->salt[at] = grid.salt[from];
            tile->water[at] = levelOf(grid.ground[from], grid.depth[from]);
            tile->discharge[at] = grid.discharge[from];
        });
    }
}

} // namespace derivean::sim

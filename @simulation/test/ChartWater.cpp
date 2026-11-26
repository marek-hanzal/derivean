#include "ChartExpect.hpp"

#include "simulation/fn/Reader.hpp"

using namespace derivean::sim;

namespace {

struct CellOf {
    const Tile* tile{};
    size_t at{};
};

CellOf cellOf(const World& world, Level level, int64_t x, int64_t z) {
    const int32_t tileX = int32_t(std::floor(double(x) / TileCells)), tileZ = int32_t(std::floor(double(z) / TileCells));
    const std::shared_ptr<const Tile> tile = world.atlas.find({level, tileX, tileZ});
    return {tile.get(), size_t(z - int64_t(tileZ) * TileCells) * TileCells + size_t(x - int64_t(tileX) * TileCells)};
}

float standingIn(const CellOf& cell) {
    if (!cell.tile) return -1e30f;
    const Ground& now = cell.tile->now;
    const float surface = now.rock[cell.at] + now.sediment[Coarse][cell.at] + now.sediment[Sand][cell.at] + now.sediment[Fines][cell.at];
    return cell.tile->water[cell.at] > surface ? cell.tile->water[cell.at] : -1e30f;
}

struct Magnified {
    int onPath{}, reached{}, streams{};
};

// The pixels a chart read from `level`, whose cells are wider than they are, checked against those
// cells themselves: a pixel carries the largest flow whose straight path from cell centre to cell
// centre passes within half a pixel of it and none otherwise; it stands in water exactly when its
// own cell's level, or failing that the highest level standing in the cells around, lies above its
// ground; and on a path it is a stream as deep as the running water of the cell the path starts in.
Magnified checkMagnified(const World& world, const Chart& chart, Level level) {
    Magnified found;
    const double cell = specOf(level).cell;
    expect(chart.pitch < cell, "the magnified chart is not finer than its cells", chart.pitch);
    TileReader reader{world.atlas, level};
    for (int j = 0; j < chart.size; ++j)
        for (int i = 0; i < chart.size; ++i) {
            const size_t at = size_t(j) * size_t(chart.size) + size_t(i);
            if (chart.level[at] != uint8_t(level)) continue;
            const Vec2 centre = centreOf(chart, i, j);
            const int64_t cellX = int64_t(std::floor(centre.x / cell)), cellZ = int64_t(std::floor(centre.z / cell));
            float flow = 0, around = -1e30f;
            CellOf source;
            Vec2 start;
            for (int dz = -1; dz <= 1; ++dz)
                for (int dx = -1; dx <= 1; ++dx) {
                    const CellOf other = cellOf(world, level, cellX + dx, cellZ + dz);
                    around = std::max(around, standingIn(other));
                    if (!other.tile || other.tile->flowTo[other.at] > 7 || other.tile->discharge[other.at] <= flow) continue;
                    const Vec2 from{(double(cellX + dx) + 0.5) * cell, (double(cellZ + dz) + 0.5) * cell};
                    const Vec2 path{NeighborX[other.tile->flowTo[other.at]] * cell, NeighborZ[other.tile->flowTo[other.at]] * cell};
                    const double along = std::clamp(dot(centre - from, path) / dot(path, path), 0.0, 1.0);
                    if (length(centre - (from + path * along)) > 0.5 * chart.pitch) continue;
                    flow = other.tile->discharge[other.at];
                    source = other;
                    start = from;
                }
            expect(chart.discharge[at] == flow, "a magnified pixel does not carry the flow of the path through it", chart.discharge[at] - flow);
            found.onPath += flow > 0;

            const float own = standingIn(cellOf(world, level, cellX, cellZ));
            const float standing = own > chart.ground[at] + 0.01f ? own : around;
            const bool flooded = standing > chart.ground[at] + 0.01f;
            const bool drawn = chart.waters[at] == uint8_t(ChartWater::Sea) || chart.waters[at] == uint8_t(ChartWater::Lake);
            expect(flooded == drawn && (!drawn || chart.water[at] == standing), "a magnified pixel does not stand in the water that reaches it", chart.water[at]);
            found.reached += drawn && !(own > chart.ground[at] + 0.01f);
            if (drawn) continue;

            // Running water of the cell the path starts in: read at this pixel for its own cell, at that cell's centre otherwise.
            SurfaceSample running;
            const bool homeward = source.tile && int64_t(std::floor(start.x / cell)) == cellX && int64_t(std::floor(start.z / cell)) == cellZ;
            const bool runs = source.tile && standingIn(source) < -1e29f && readSurface(reader, homeward ? centre : start, running) && running.water > running.ground + 0.01f;
            expect((chart.waters[at] == uint8_t(ChartWater::Stream)) == runs, "a magnified pixel is a stream off a path, or dry on the path of running water", chart.discharge[at]);
            if (runs) expect(chart.water[at] == chart.ground[at] + (running.water - running.ground), "a stream pixel is not as deep as the cell its path starts in", chart.water[at]);
            found.streams += runs;
        }
    return found;
}

} // namespace

// Also charts finer than region cells around the region's greatest river and around a shore, if it has one.
void checkChartWater(const World& world, Vec2 focus, const Chart& site, const Chart& local) {
    checkMagnified(world, site, Level::Local);
    checkMagnified(world, local, Level::Region);
    const TileKey key = tileAt(Level::Region, focus);
    const std::shared_ptr<const Tile> tile = world.atlas.find(key);
    expect(tile != nullptr, "the region tile under the focus is not finished");
    const double cell = specOf(Level::Region).cell;
    const auto centreOfCell = [&](int x, int z) { return Vec2{(double(key.x) * TileCells + x + 0.5) * cell, (double(key.z) * TileCells + z + 0.5) * cell}; };
    int river = -1, shore = -1;
    for (int z = 1; z < TileCells - 1; ++z)
        for (int x = 1; x < TileCells - 1; ++x) {
            const int at = z * TileCells + x;
            const bool wet = standingIn({tile.get(), size_t(at)}) > -1e29f;
            if (!wet && tile->flowTo[size_t(at)] <= 7 && (river < 0 || tile->discharge[size_t(at)] > tile->discharge[size_t(river)])) river = at;
            if (wet && shore < 0 && standingIn({tile.get(), size_t(at + 1)}) < -1e29f) shore = at;
        }
    // 512 m pixels read the region alone, whatever finer tiles other checks have built.
    expect(river >= 0, "the region tile has no running water to chart");
    const Chart along = chartOf(world, {centreOfCell(river % TileCells, river / TileCells), 24576, 48});
    const Magnified found = checkMagnified(world, along, Level::Region);
    expect(found.onPath > 0 && found.streams > 0, "no pixel lies on the path of the region's greatest river, or none is a stream", found.onPath);
    if (shore < 0) return;
    checkMagnified(world, chartOf(world, {centreOfCell(shore % TileCells, shore / TileCells), 24576, 48}), Level::Region);
}

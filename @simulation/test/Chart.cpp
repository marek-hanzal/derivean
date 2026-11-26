#include "LandscapeFixture.hpp"

#include "chart/fn/Chart.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Query.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Chart: " << message << " (" << value << ")\n";
    std::exit(1);
}

Vec2 centreOf(const Chart& chart, int i, int j) {
    return {chart.origin.x + (i + 0.5) * chart.pitch, chart.origin.z + (j + 0.5) * chart.pitch};
}

bool same(const Chart& a, const Chart& b) {
    const auto equal = [](const auto& left, const auto& right) {
        return left.size() == right.size() && std::memcmp(left.data(), right.data(), left.size() * sizeof(left[0])) == 0;
    };
    return a.size == b.size && equal(a.ground, b.ground) && equal(a.water, b.water) && equal(a.discharge, b.discharge) && equal(a.ice, b.ice) &&
           equal(a.warmth, b.warmth) && equal(a.cover, b.cover) && equal(a.bedrock, b.bedrock) && equal(a.waters, b.waters) && equal(a.level, b.level);
}

// Every pixel is surfaceAt at its centre where a tile the pitch may read is finished, and the planet elsewhere.
// Returns how many pixels came from each level.
std::array<int, 4> checkAgainstSurface(const World& world, const Chart& chart) {
    std::array<int, 4> byLevel{};
    Level finest{};
    const bool tiles = chartFinest(chart.pitch, finest);
    for (int j = 0; j < chart.size; ++j)
        for (int i = 0; i < chart.size; ++i) {
            const size_t at = size_t(j) * size_t(chart.size) + size_t(i);
            const Vec2 centre = centreOf(chart, i, j);
            const SurfaceSample sample = tiles ? surfaceAt(world, centre, finest) : SurfaceSample{};
            ++byLevel[chart.level[at]];
            expect((chart.waters[at] == uint8_t(ChartWater::Dry)) == std::isnan(chart.water[at]), "a pixel's water level and water kind disagree", double(at));
            if (!sample.found) {
                const Vec2 point = planetPoint(world.planet.seed, centre);
                expect(chart.level[at] == 0, "a pixel without a finished tile does not say it is the planet's", chart.level[at]);
                expect(chart.ground[at] == float(samplePlanet(world.planet.elevation, point)), "a planet pixel is not the planet's elevation there", chart.ground[at]);
                expect(std::isnan(chart.water[at]) == (chart.ground[at] >= world.planet.seaLevel), "a planet pixel is wet above sea level or dry below it", chart.ground[at]);
                continue;
            }
            expect(chart.level[at] == uint8_t(sample.level), "a pixel names another level than surfaceAt", chart.level[at]);
            expect(chart.ground[at] == sample.ground, "a pixel's ground differs from surfaceAt", chart.ground[at] - sample.ground);
            expect(chart.cover[at] == sample.ground - sample.rock && chart.bedrock[at] == uint8_t(sample.bedrock), "a pixel's cover or rock differs from surfaceAt", chart.cover[at]);
            expect(chart.warmth[at] == sample.warmth, "a pixel's warmth differs from surfaceAt", chart.warmth[at] - sample.warmth);
            const bool wet = sample.water > sample.ground + 0.01f;
            if (chart.pitch >= specOf(sample.level).cell) {
                expect(wet ? chart.water[at] == sample.water : std::isnan(chart.water[at]), "a pixel's water differs from surfaceAt", chart.water[at]);
                expect(chart.discharge[at] >= sample.discharge, "a pixel lost the flow through its own centre", chart.discharge[at] - sample.discharge);
                continue;
            }
            // Finer than the cells it reads, a chart keeps surfaceAt's water except that a stream is its path
            // and not its cell, and that water standing next door reaches in; checkMagnified holds it to that.
            const ChartWater kind = ChartWater(chart.waters[at]);
            if (kind == ChartWater::Stream) expect(wet && chart.water[at] == sample.water && chart.discharge[at] >= sample.discharge, "a stream pixel is not surfaceAt's running water on its path", chart.water[at]);
            if (kind == ChartWater::Dry) expect(!wet || chart.discharge[at] < sample.discharge, "a pixel on the path of its cell's stream is dry", chart.discharge[at]);
        }
    return byLevel;
}

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
    int onPath{}, reached{};
};

// A chart finer than the cells of `level`, checked against those cells themselves: a pixel carries
// the largest flow whose straight path from cell centre to cell centre passes within half a pixel of
// it and none otherwise, and it stands in water exactly when its own cell's level, or failing that
// the highest level standing in the cells around, lies above its ground.
Magnified checkMagnified(const World& world, const Chart& chart, Level level) {
    Magnified found;
    const double cell = specOf(level).cell;
    expect(chart.pitch < cell, "the magnified chart is not finer than its cells", chart.pitch);
    for (int j = 0; j < chart.size; ++j)
        for (int i = 0; i < chart.size; ++i) {
            const size_t at = size_t(j) * size_t(chart.size) + size_t(i);
            if (chart.level[at] != uint8_t(level)) continue;
            const Vec2 centre = centreOf(chart, i, j);
            const int64_t cellX = int64_t(std::floor(centre.x / cell)), cellZ = int64_t(std::floor(centre.z / cell));
            float flow = 0, around = -1e30f;
            for (int dz = -1; dz <= 1; ++dz)
                for (int dx = -1; dx <= 1; ++dx) {
                    const CellOf other = cellOf(world, level, cellX + dx, cellZ + dz);
                    around = std::max(around, standingIn(other));
                    if (!other.tile || other.tile->flowTo[other.at] > 7) continue;
                    const Vec2 from{(double(cellX + dx) + 0.5) * cell, (double(cellZ + dz) + 0.5) * cell};
                    const Vec2 path{NeighborX[other.tile->flowTo[other.at]] * cell, NeighborZ[other.tile->flowTo[other.at]] * cell};
                    const double along = std::clamp(dot(centre - from, path) / dot(path, path), 0.0, 1.0);
                    if (length(centre - (from + path * along)) <= 0.5 * chart.pitch) flow = std::max(flow, other.tile->discharge[other.at]);
                }
            expect(chart.discharge[at] == flow, "a magnified pixel does not carry the flow of the path through it", chart.discharge[at] - flow);
            found.onPath += flow > 0;

            const float own = standingIn(cellOf(world, level, cellX, cellZ));
            const float standing = own > chart.ground[at] + 0.01f ? own : around;
            const bool flooded = standing > chart.ground[at] + 0.01f;
            const bool drawn = chart.waters[at] == uint8_t(ChartWater::Sea) || chart.waters[at] == uint8_t(ChartWater::Lake);
            expect(flooded == drawn && (!drawn || chart.water[at] == standing), "a magnified pixel does not stand in the water that reaches it", chart.water[at]);
            found.reached += drawn && !(own > chart.ground[at] + 0.01f);
        }
    return found;
}

// Charts finer than region cells around the region's greatest river and around a shore, if it has one.
void checkMagnifiedWater(const World& world, Vec2 focus) {
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
    checkAgainstSurface(world, along);
    expect(checkMagnified(world, along, Level::Region).onPath > 0, "no pixel lies on the path of the region's greatest river");
    if (shore < 0) return;
    const Chart beside = chartOf(world, {centreOfCell(shore % TileCells, shore / TileCells), 24576, 48});
    checkAgainstSurface(world, beside);
    checkMagnified(world, beside, Level::Region);
}

// The four quarters of a square, asked for one by one, are the square: a map may be put together from parts.
void checkQuarters(const World& world, const ChartRequest& whole) {
    const Chart chart = chartOf(world, whole);
    const int half = whole.size / 2;
    for (int quarter = 0; quarter < 4; ++quarter) {
        const int qi = quarter & 1, qj = quarter >> 1;
        const Chart part = chartOf(world, {{whole.centre.x + (qi - 0.5) * 0.5 * whole.span, whole.centre.z + (qj - 0.5) * 0.5 * whole.span}, 0.5 * whole.span, half});
        for (int j = 0; j < half; ++j)
            for (int i = 0; i < half; ++i) {
                const size_t from = size_t(j) * size_t(half) + size_t(i), to = size_t(j + qj * half) * size_t(whole.size) + size_t(i + qi * half);
                expect(part.ground[from] == chart.ground[to] && part.level[from] == chart.level[to] && part.discharge[from] == chart.discharge[to],
                       "a quarter of a chart differs from the chart", part.ground[from] - chart.ground[to]);
            }
    }
}

// Positions beyond the planet's span land on the same planet: a square across the corner where the
// span ends is read whole, and every pixel is the planet at the image of its place inside the span.
void checkSeam(const World& world) {
    const Planet& planet = world.planet;
    for (const double span : {1024e3, 16384e3}) {
        const Chart chart = chartOf(world, {{PlanetSpan, 0}, span, 96});
        expect(chart.size == 96, "a chart across the seam came back empty");
        int east = 0, west = 0;
        for (int j = 0; j < chart.size; ++j)
            for (int i = 0; i < chart.size; ++i) {
                const size_t at = size_t(j) * size_t(chart.size) + size_t(i);
                const Vec2 centre = centreOf(chart, i, j);
                (centre.x < PlanetSpan ? west : east) += 1;
                Vec2 inside = planetPoint(planet.seed, centre);
                inside = {inside.x - PlanetSpan * std::floor(inside.x / PlanetSpan), inside.z - PlanetSpan * std::floor(inside.z / PlanetSpan)};
                const float expected = float(sample(planet.elevation, inside.x / PlanetCell - 0.5, inside.z / PlanetCell - 0.5, Kernel::Spline, Edge::Wrap));
                expect(std::isfinite(chart.ground[at]) && chart.level[at] == 0, "a pixel across the seam is not answered by the planet", chart.ground[at]);
                expect(std::abs(chart.ground[at] - expected) < 1e-2f, "a pixel across the seam is not the planet at its image inside the span", chart.ground[at] - expected);
            }
        expect(east > 0 && west > 0, "the seam chart does not straddle the seam");
    }
}

} // namespace

void testChart() {
    World& world = testWorld();
    const Vec2 focus = testFocus();
    surveyAt(world, focus);
    const size_t before = world.atlas.finished();

    // Pitches that may read site, local and region tiles, and one too coarse for any tile.
    const Chart site = chartOf(world, {focus, 1536, 96});
    const Chart cells = chartOf(world, {focus, 384, 96});   // a pixel is exactly a site cell
    const Chart local = chartOf(world, {focus, 24576, 96});
    const Chart region = chartOf(world, {focus, 393216, 96});
    const Chart planet = chartOf(world, {focus, PlanetSpan, 96});
    const auto bySite = checkAgainstSurface(world, site), byLocal = checkAgainstSurface(world, local);
    expect(checkAgainstSurface(world, cells)[3] == 96 * 96, "a chart of the site tile under the focus read something else");
    const auto byRegion = checkAgainstSurface(world, region), byPlanet = checkAgainstSurface(world, planet);
    expect(bySite[3] > 0 && bySite[2] > 0, "the site chart does not mix site tiles with the local tile around them", bySite[3]);
    expect(byLocal[2] > 0 && byLocal[1] > 0 && byLocal[3] == 0, "the local chart does not mix the local tile with the region around it", byLocal[2]);
    expect(byRegion[1] > 0 && byRegion[0] > 0 && byRegion[2] == 0, "the region chart does not mix the region tile with the planet around it", byRegion[1]);
    expect(byPlanet[0] == 96 * 96, "a chart too coarse for tiles read some", byPlanet[0]);
    expect(site.tiles == before, "a chart does not carry the number of finished tiles", double(site.tiles));

    expect(same(site, chartOf(world, {focus, 1536, 96})) && same(region, chartOf(world, {focus, 393216, 96})), "the same request over the same tiles gave two charts");
    const Vec2 corner{std::floor(focus.x / 4096) * 4096, std::floor(focus.z / 4096) * 4096};
    checkQuarters(world, {corner, 2048, 128});
    checkQuarters(world, {corner, 32768, 128});
    checkSeam(world);
    checkMagnifiedWater(world, focus);

    expect(world.atlas.finished() == before, "reading charts built a tile", double(world.atlas.finished() - before));
}

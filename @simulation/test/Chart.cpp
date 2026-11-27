#include "ChartExpect.hpp"
#include "LandscapeFixture.hpp"

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Reader.hpp"

#include <cstring>

using namespace derivean::sim;

namespace {

bool same(const Chart& a, const Chart& b) {
    const auto equal = [](const auto& left, const auto& right) {
        return left.size() == right.size() && std::memcmp(left.data(), right.data(), left.size() * sizeof(left[0])) == 0;
    };
    return a.size == b.size && equal(a.ground, b.ground) && equal(a.water, b.water) && equal(a.discharge, b.discharge) && equal(a.ice, b.ice) &&
           equal(a.warmth, b.warmth) && equal(a.cover, b.cover) && equal(a.bedrock, b.bedrock) && equal(a.waters, b.waters) && equal(a.level, b.level);
}

// The untouched surface at a point from the finest finished level up to `finest`: what the tiles hold, without
// anything the session has changed.
SurfaceSample untouchedAt(const World& world, Vec2 position, Level finest) {
    SurfaceSample sample;
    for (int level = int(finest); level >= int(Level::Region); --level) {
        TileReader reader{world.atlas, Level(level)};
        if (readSurface(reader, position, sample)) break;
    }
    return sample;
}

// Every pixel is that surface at its centre where a tile the pitch may read is finished, and the planet
// elsewhere. Returns how many pixels came from each level.
std::array<int, 5> checkAgainstSurface(const World& world, const Chart& chart) {
    std::array<int, 5> byLevel{};
    Level finest{};
    const bool tiles = chartFinest(chart.pitch, finest);
    for (int j = 0; j < chart.size; ++j)
        for (int i = 0; i < chart.size; ++i) {
            const size_t at = size_t(j) * size_t(chart.size) + size_t(i);
            const Vec2 centre = centreOf(chart, i, j);
            const SurfaceSample sample = tiles ? untouchedAt(world, centre, finest) : SurfaceSample{};
            ++byLevel[chart.level[at]];
            expect((chart.waters[at] == uint8_t(ChartWater::Dry)) == std::isnan(chart.water[at]), "a pixel's water level and water kind disagree", double(at));
            if (!sample.found) {
                const Vec2 point = planetPoint(world.planet.seed, centre);
                expect(chart.level[at] == 0, "a pixel without a finished tile does not say it is the planet's", chart.level[at]);
                expect(chart.ground[at] == float(samplePlanet(world.planet.elevation, point)), "a planet pixel is not the planet's elevation there", chart.ground[at]);
                expect(std::isnan(chart.water[at]) == (chart.ground[at] >= world.planet.seaLevel), "a planet pixel is wet above sea level or dry below it", chart.ground[at]);
                continue;
            }
            expect(chart.level[at] == uint8_t(sample.level), "a pixel names another level than the surface", chart.level[at]);
            expect(chart.ground[at] == sample.ground, "a pixel's ground differs from the surface", chart.ground[at] - sample.ground);
            expect(chart.cover[at] == sample.ground - sample.rock && chart.bedrock[at] == uint8_t(sample.bedrock), "a pixel's cover or rock differs from the surface", chart.cover[at]);
            expect(chart.warmth[at] == sample.warmth, "a pixel's warmth differs from the surface", chart.warmth[at] - sample.warmth);
            // Finer than the cells it reads, a chart draws water by rules of its own; checkMagnified holds it to them.
            if (chart.pitch < specOf(sample.level).cell) continue;
            const bool wet = sample.water > sample.ground + 0.01f;
            expect(wet ? chart.water[at] == sample.water : std::isnan(chart.water[at]), "a pixel's water differs from the surface", chart.water[at]);
            expect(chart.discharge[at] >= sample.discharge, "a pixel lost the flow through its own centre", chart.discharge[at] - sample.discharge);
        }
    return byLevel;
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
    // Earlier checks on the shared world may have finished the Local tiles all round the focus, so
    // the local chart is centred on the eastern edge of what is finished to read both kinds of pixel.
    Vec2 edge = focus;
    while (world.atlas.find(tileAt(Level::Local, edge))) edge.x += tileSpan(Level::Local);
    edge.x = std::floor(edge.x / tileSpan(Level::Local)) * tileSpan(Level::Local);
    const Chart local = chartOf(world, {edge, 24576, 96});
    // Likewise the region chart: ground tiles ask for the ground water of their Local tile, which
    // finishes the 3 x 3 Region tiles around it, as wide as this chart.
    Vec2 regionEdge = focus;
    while (world.atlas.find(tileAt(Level::Region, regionEdge))) regionEdge.x += tileSpan(Level::Region);
    regionEdge.x = std::floor(regionEdge.x / tileSpan(Level::Region)) * tileSpan(Level::Region);
    const Chart region = chartOf(world, {regionEdge, 393216, 96});
    const Chart planet = chartOf(world, {focus, PlanetSpan, 96});
    const auto bySite = checkAgainstSurface(world, site), byLocal = checkAgainstSurface(world, local);
    expect(checkAgainstSurface(world, cells)[3] == 96 * 96, "a chart of the site tile under the focus read something else");
    const auto byRegion = checkAgainstSurface(world, region), byPlanet = checkAgainstSurface(world, planet);
    expect(bySite[3] > 0 && bySite[2] > 0, "the site chart does not mix site tiles with the local tile around them", bySite[3]);
    expect(byLocal[2] > 0 && byLocal[1] > 0 && byLocal[3] == 0, "the local chart does not mix the local tile with the region around it", byLocal[2]);
    expect(byRegion[1] > 0 && byRegion[0] > 0 && byRegion[2] == 0, "the region chart does not mix the region tile with the planet around it", byRegion[1]);
    expect(byPlanet[0] == 96 * 96, "a chart too coarse for tiles read some", byPlanet[0]);
    expect(site.tiles == before, "a chart does not carry the number of finished tiles", double(site.tiles));

    expect(same(site, chartOf(world, {focus, 1536, 96})) && same(region, chartOf(world, {regionEdge, 393216, 96})), "the same request over the same tiles gave two charts");
    const Vec2 corner{std::floor(focus.x / 4096) * 4096, std::floor(focus.z / 4096) * 4096};
    checkQuarters(world, {corner, 2048, 128});
    checkQuarters(world, {corner, 32768, 128});
    checkSeam(world);
    checkChartWater(world, focus, site, local);

    expect(world.atlas.finished() == before, "reading charts built a tile", double(world.atlas.finished() - before));

    // A pitch of half a metre reads the ground tile the voxels are cut from, and the site around it.
    surveyAt(world, focus, Level::Ground);
    const TileKey under = tileAt(Level::Ground, focus);
    const Vec2 middle{(under.x + 0.5) * tileSpan(Level::Ground), (under.z + 0.5) * tileSpan(Level::Ground)};
    const auto byGround = checkAgainstSurface(world, chartOf(world, {middle, 2 * tileSpan(Level::Ground), 256}));
    expect(byGround[4] >= 128 * 128 && byGround[4] + byGround[3] == 256 * 256, "a chart fine enough does not read the ground tile under it", byGround[4]);
}

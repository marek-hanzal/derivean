#include "ground/fn/Stamp.hpp"
#include "horizon/type/Beds.hpp"
#include "horizon/type/CubeChunk.hpp"
#include "horizon/type/StandingWater.hpp"
#include "horizon/type/Tint.hpp"
#include "nature/type/SurfaceReader.hpp"
#include "simulation/fn/Live.hpp"
#include "simulation/fn/Reader.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <optional>

namespace derivean {
namespace {

constexpr int Halo = 1;                          // columns beyond the chunk that its faces and their occlusion read
constexpr int Side = CubeSide + 2 * Halo;
constexpr int Shore = Halo + 1;                  // columns beyond the chunk whose water level is known: a halo column asks its neighbours
constexpr int Ring = Shore + WaterRing;          // columns beyond the chunk that are sampled
constexpr int Read = CubeSide + 2 * Ring;
constexpr float TopVoxel = 0.5f * VoxelSize;     // m under the ground at which the middle of the top voxel lies
constexpr float GlacierDepth = 1.0f;             // m of ice from which the ground reads as a glacier
constexpr float RockSkin = 0.05f;                // m into bare rock at which its bed is read
constexpr float SheetGap = 0.02f;                // m between water levels from which they are two water bodies
constexpr float HeldSlope = 1.0f;                // rise over run up to which loose cover lies on a slope
constexpr int WaterProbes = 3;                   // per axis; closer than a site tile is wide at every level whose water shows near the eye

struct Column {
    sim::SurfaceSample sample;
    int top{};   // cubes below this index are solid
};

// What the cubes of one column are made of. The layering is only looked up for a column that shows rock.
struct Filling {
    const sim::World& world;
    SurfaceReader& reader;
    sim::Vec2 place;
    const sim::SurfaceSample& sample;
    std::optional<Beds> beds;
};

// The cube whose centre lies `depth` metres under the ground: the cover where it is thicker than
// that, else the bed there.
sim::Material under(Filling& filling, float depth) {
    const sim::SurfaceSample& sample = filling.sample;
    const float loose = sample.deposit + sample.sediment[sim::Coarse] + sample.sediment[sim::Sand] + sample.sediment[sim::Fines];
    if (depth < loose) return surfaceMaterial(sample, depth);
    if (!filling.beds) filling.beds = bedsAt(filling.world, filling.place, sample.datum);
    return rockAt(*filling.beds, double(sample.ground) - std::max(depth, loose + RockSkin));
}

// The top cube of a column: what a voxel at the surface shows.
sim::Material top(Filling& filling) {
    return iceAt(filling.reader, filling.sample, filling.place) > GlacierDepth ? sim::Material::Ice : under(filling, TopVoxel);
}

// The stones of the ground tiles over a lattice of cubes, Air elsewhere: a cube is a stone's where
// its centre lies in the stone, as a voxel is (ground/fn/Stamp.hpp), and moves with edited ground
// as the voxels' stones do (simulation/fn/Block.cpp).
std::vector<sim::Material> stoneCubes(const sim::World& world, const sim::CubeLattice& lattice) {
    std::vector<sim::Material> stamped(size_t(lattice.countX) * size_t(lattice.countY) * size_t(lattice.countZ), sim::Material::Air);
    const bool edited = !world.interventions.overlay.tiles.empty();
    const std::function<float(double, double)> lift = [&](double x, double z) { return sim::changeAt(world, x, z).ground(); };
    sim::TileReader fine{world.atlas, sim::Level::Ground};
    const double maxX = lattice.originX + lattice.countX * lattice.step, maxZ = lattice.originZ + lattice.countZ * lattice.step;
    const sim::TileKey low = sim::tileAt(sim::Level::Ground, {lattice.originX - sim::ClastReach, lattice.originZ - sim::ClastReach});
    const sim::TileKey high = sim::tileAt(sim::Level::Ground, {maxX + sim::ClastReach, maxZ + sim::ClastReach});
    for (int z = low.z; z <= high.z; ++z)
        for (int x = low.x; x <= high.x; ++x)
            if (const sim::Tile* tile = fine.tile(x, z)) sim::stampClasts(*tile, lattice, stamped.data(), edited ? &lift : nullptr);
    return stamped;
}

} // namespace

sim::Level cubeSource(int level) {
    const double edge = edgeOf(level);
    if (edge < 1.0) return sim::Level::Ground;
    if (edge <= 16.0) return sim::Level::Site;
    return edge <= 256.0 ? sim::Level::Local : sim::Level::Region;
}

double cubeReadMargin(int level) {
    return Ring * edgeOf(level) + EaseCells * sim::specOf(cubeSource(level)).cell + (cubeSource(level) == sim::Level::Ground ? sim::ClastReach : 0.0);
}

bool cubeFinal(int level, uint64_t detail) {
    for (int probe = 0; probe < 4; ++probe)
        if (((detail >> (3 * probe)) & 7) < uint64_t(cubeSource(level))) return false;
    return true;
}

uint64_t cubeDetail(const sim::World& world, CubeKey key) {
    const double edge = edgeOf(key.level), size = CubeSide * edge, x = key.x * size, z = key.z * size;
    const sim::Level source = cubeSource(key.level);
    SurfaceReader ground = surfaceReader(world, source), water = surfaceReader(world, sim::Level::Site);
    // The square the ground is read from, with the tiles easing looks at, is smaller than a tile of
    // the source level: its corners touch every tile it overlaps.
    const double reach = cubeReadMargin(key.level), wide = size + 2 * Ring * edge;
    uint64_t detail = 0;
    int probe = 0;
    for (double dz : {-reach, size + reach})
        for (double dx : {-reach, size + reach}) detail |= uint64_t(surfaceOrPlanet(ground, {x + dx, z + dz}).level) << (3 * probe++);
    for (int j = 0; j < WaterProbes; ++j)
        for (int i = 0; i < WaterProbes; ++i) {
            const sim::Vec2 place{x - Ring * edge + i * wide / (WaterProbes - 1), z - Ring * edge + j * wide / (WaterProbes - 1)};
            detail |= uint64_t(surfaceOrPlanet(water, place).level) << (3 * probe++);
        }
    return detail;
}

CubeColumn buildCubes(const sim::World& world, CubeKey key) {
    const auto start = std::chrono::steady_clock::now();
    CubeColumn result;
    result.key = key;
    result.detail = cubeDetail(world, key);
    const double edge = edgeOf(key.level), originX = key.x * CubeSide * edge, originZ = key.z * CubeSide * edge;
    const auto centre = [&](int i, int j) { return sim::Vec2{originX + (i + 0.5) * edge, originZ + (j + 0.5) * edge}; };

    SurfaceReader reader = surfaceReader(world, cubeSource(key.level));
    std::vector<sim::SurfaceSample> samples(size_t(Read) * Read);
    for (int j = -Ring; j < CubeSide + Ring; ++j)
        for (int i = -Ring; i < CubeSide + Ring; ++i) samples[size_t(j + Ring) * Read + size_t(i + Ring)] = surfaceEased(reader, centre(i, j));
    const auto sampled = [&](int i, int j) -> const sim::SurfaceSample& { return samples[size_t(j + Ring) * Read + size_t(i + Ring)]; };
    constexpr int known = CubeSide + 2 * Shore;
    const sim::Vec2 first = centre(-Shore, -Shore);
    const std::vector<float> water = standingWater(world, {first.x, first.z, edge, known}, [&](int i, int j) { return sampled(i - Shore, j - Shore).ground; });

    std::vector<Column> columns(size_t(Side) * Side);
    const auto column = [&](int i, int j) -> Column& { return columns[size_t(j + Halo) * Side + size_t(i + Halo)]; };
    const auto levelAt = [&](int i, int j) { return water[size_t(j + Shore) * known + size_t(i + Shore)]; };
    // Under water only where the level is known all around: where its trace ends the column is shore.
    const auto wet = [&](int i, int j) {
        if (!(sampled(i, j).ground < levelAt(i, j) - WetDepth)) return false;
        for (int b = j - 1; b <= j + 1; ++b)
            for (int a = i - 1; a <= i + 1; ++a)
                if (std::isnan(levelAt(a, b))) return false;
        return true;
    };
    int lowest = std::numeric_limits<int>::max(), highest = std::numeric_limits<int>::lowest();
    uint8_t dataLevel = uint8_t(sim::Level::Ground);
    for (int j = -Halo; j < CubeSide + Halo; ++j)
        for (int i = -Halo; i < CubeSide + Halo; ++i) {
            Column& here = column(i, j);
            here.sample = sampled(i, j);
            here.top = int(std::floor(here.sample.ground / edge + 0.5));
            if (const float level = levelAt(i, j); !std::isnan(level)) {
                const int above = int(std::ceil(level / edge));
                here.top = wet(i, j) ? std::min(here.top, above - 1) : std::max(here.top, above);
            }
            lowest = std::min(lowest, here.top);
            highest = std::max(highest, here.top);
            dataLevel = std::min(dataLevel, uint8_t(here.sample.level));
        }

    // Stones lie on the ground the levels cut from the ground level show, as on the voxels; the
    // coarser levels' data has none.
    const int below = lowest - 2;
    std::vector<sim::Material> stones;
    if (cubeSource(key.level) == sim::Level::Ground) {
        const sim::CubeLattice lattice{originX - Halo * edge, below * edge, originZ - Halo * edge, edge, Side, highest + int(std::ceil(sim::ClastReach / edge)) - below, Side};
        stones = stoneCubes(world, lattice);
        for (size_t at = 0; at < stones.size(); ++at)
            if (stones[at] != sim::Material::Air) highest = std::max(highest, below + int(at / (size_t(Side) * Side)) + 1);
    }

    // Solid from two cubes under the lowest top, so no face is made that no neighbour could show.
    Voxels cubes({{-Halo, below, -Halo}, {CubeSide + Halo, highest + 1, CubeSide + Halo}});
    for (int j = -Halo; j < CubeSide + Halo; ++j)
        for (int i = -Halo; i < CubeSide + Halo; ++i) {
            const Column& here = column(i, j);
            for (int y = below; y < here.top; ++y) cubes[{i, y, j}] = Material::Bedrock;
            if (i < 0 || j < 0 || i >= CubeSide || j >= CubeSide) continue;
            Filling filling{world, reader, centre(i, j), here.sample, std::nullopt};
            const Material surface = top(filling);
            cubes[{i, here.top - 1, j}] = surface;
            // A step between columns stands for the slope the cubes were cut from. Where that slope
            // holds its cover the side of the step is that cover too; only a face too steep for it,
            // or bare, shows the beds.
            const float slope = float(std::hypot(sampled(i + 1, j).ground - sampled(i - 1, j).ground, sampled(i, j + 1).ground - sampled(i, j - 1).ground) / (2 * edge));
            const bool covered = slope <= HeldSlope && here.sample.ground - here.sample.rock > TopVoxel;
            const int open = std::min({column(i - 1, j).top, column(i + 1, j).top, column(i, j - 1).top, column(i, j + 1).top});
            for (int y = open; y < here.top - 1; ++y)
                cubes[{i, y, j}] = covered ? surface : under(filling, float(std::max(0.0, here.sample.ground - (y + 0.5) * edge)));
        }
    for (size_t at = 0; at < stones.size(); ++at)
        if (stones[at] != sim::Material::Air)
            cubes[{int(at % Side) - Halo, below + int(at / (size_t(Side) * Side)), int(at / Side % Side) - Halo}] = stones[at];

    // Chunks are cut where their lattice lies; a face keeps to its chunk, counted from its corner.
    for (int level = floorDiv(lowest - 1, CubeSide); level <= floorDiv(highest - 1, CubeSide); ++level) {
        const Bounds part{{0, std::max(level * CubeSide, lowest - 1), 0}, {CubeSide, std::min((level + 1) * CubeSide, highest), CubeSide}};
        CubeChunk chunk{0, key.level, {key.x, level, key.z}, meshFaces(cubes, part), {}, dataLevel};
        const int16_t rise = int16_t(part.min.y - level * CubeSide);
        for (size_t face = 0; face < chunk.faces.size(); face += FaceWords) chunk.faces[face + (chunk.faces[face] == 1 ? 2 : chunk.faces[face] == 0 ? 3 : 4)] += rise;
        result.faces += chunk.faces.size() / FaceWords;
        if (!chunk.faces.empty()) result.chunks.push_back(std::move(chunk));
    }

    // One sheet per water body: levels closer than SheetGap are one, drawn at the highest of them,
    // which every column under it ends below.
    std::vector<float> levels;
    for (int j = 0; j < CubeSide; ++j)
        for (int i = 0; i < CubeSide; ++i)
            if (wet(i, j)) levels.push_back(levelAt(i, j));
    std::sort(levels.begin(), levels.end());
    const auto always = [](int32_t) { return true; };
    for (size_t first = 0; first < levels.size();) {
        size_t last = first;
        while (last + 1 < levels.size() && levels[last + 1] - levels[last] < SheetGap) ++last;
        WaterSheet sheet{double(levels[last]), {}};
        std::vector<int32_t> mask(size_t(CubeSide) * CubeSide);
        for (int j = 0; j < CubeSide; ++j)
            for (int i = 0; i < CubeSide; ++i) mask[size_t(i) * CubeSide + j] = wet(i, j) && levelAt(i, j) >= levels[first] && levelAt(i, j) <= levels[last];
        mergeMask(mask, CubeSide, CubeSide, always, always, [&](int u, int v, int width, int height, int32_t) {
            sheet.faces.insert(sheet.faces.end(), {1, 1, 0, int16_t(u), int16_t(v), int16_t(width), int16_t(height), int16_t(Material::Water), 0});
        });
        result.chunks.back().water.push_back(std::move(sheet));
        first = last + 1;
    }
    result.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return result;
}

} // namespace derivean

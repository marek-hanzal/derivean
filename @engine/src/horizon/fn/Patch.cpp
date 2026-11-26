#include "horizon/type/Patch.hpp"
#include "horizon/type/StandingWater.hpp"
#include "horizon/type/Tint.hpp"
#include "nature/type/SurfaceReader.hpp"
#include "simulation/fn/Query.hpp"
#include "voxel/constant/Materials.hpp"
#include "voxel/type/Grid.hpp"

#include <chrono>
#include <cmath>
#include <limits>

namespace derivean {
namespace {

constexpr int Ring = 2 * ShadeDensity;        // shading samples around the patch: the shore rule reads two vertices out
constexpr int Padded = ShadeSide + 2 * Ring;
constexpr int Probes[5][2]{{0, 0}, {PatchCells, 0}, {0, PatchCells}, {PatchCells, PatchCells}, {PatchCells / 2, PatchCells / 2}};
constexpr float HiddenDepth = 1.0f;           // m of water under which the shape of the bed does not show
constexpr float GlacierDepth = 1.0f;          // m of ice from which the ground reads as a glacier

uint32_t levelBits(const sim::SurfaceSample& sample, int probe) { return uint32_t(sample.level) << (3 * probe); }

// Set while a tile of the patch's own level is missing within easing distance of it: the patch then
// holds eased samples and is due again when that tile arrives, even if no probe changes level.
constexpr uint32_t Eased = 1u << 31;

uint32_t easedBit(const sim::World& world, PatchKey key) {
    const sim::Level source = sourceFor(key.level);
    const double size = sizeOf(key.level), reach = 0.5 * size + Ring * cellOf(key.level) / ShadeDensity + EaseCells * sim::specOf(source).cell;
    return sim::readyAround(world, {(key.x + 0.5) * size, (key.z + 0.5) * size}, reach, source) ? 0 : Eased;
}

constexpr float TopVoxel = 0.5f * VoxelSize;   // m under the ground at which the middle of the top voxel lies on average
constexpr int SkyReach = Ring;                // shading samples looked along for what hides the sky
constexpr int SkyRays[8][2]{{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
// Thresholds of an ordered dither: where cover thins out over rock, each texel shows one or the
// other in the share a voxel column would, instead of a blend that is neither.
constexpr float Dither[4][4]{{0.5f, 8.5f, 2.5f, 10.5f}, {12.5f, 4.5f, 14.5f, 6.5f}, {3.5f, 11.5f, 1.5f, 9.5f}, {15.5f, 7.5f, 13.5f, 5.5f}};

// The lesser of the slopes to either side, none across a crest or a pit: a ledge keeps its lip.
double edgeSlope(float before, float here, float after, double spacing) {
    const double back = (here - before) / spacing, ahead = (after - here) / spacing;
    if (back * ahead <= 0) return 0;
    return std::abs(back) < std::abs(ahead) ? back : ahead;
}

} // namespace

sim::Level sourceFor(int level) {
    const double sample = cellOf(level) / ShadeDensity;
    if (sample < 2.0) return sim::Level::Ground;
    if (sample <= 16.0) return sim::Level::Site;
    return sample <= 256.0 ? sim::Level::Local : sim::Level::Region;
}

uint32_t detailAt(const sim::World& world, PatchKey key) {
    const double cell = cellOf(key.level), size = sizeOf(key.level);
    SurfaceReader reader = surfaceReader(world, sourceFor(key.level));
    uint32_t detail = easedBit(world, key);
    for (int probe = 0; probe < 5; ++probe)
        detail |= levelBits(surfaceEased(reader, {key.x * size + Probes[probe][0] * cell, key.z * size + Probes[probe][1] * cell}), probe);
    return detail;
}

FarPatch buildPatch(const sim::World& world, PatchKey key) {
    const auto start = std::chrono::steady_clock::now();
    FarPatch patch;
    patch.key = key;
    const double cell = cellOf(key.level), size = sizeOf(key.level), fine = cell / ShadeDensity;
    const double originX = key.x * size, originZ = key.z * size;

    // One lattice of samples at shading density serves everything: its even points are the vertices.
    SurfaceReader reader = surfaceReader(world, sourceFor(key.level));
    std::vector<sim::SurfaceSample> samples(size_t(Padded) * Padded);
    for (int b = 0; b < Padded; ++b)
        for (int a = 0; a < Padded; ++a) samples[size_t(b) * Padded + a] = surfaceEased(reader, {originX + (a - Ring) * fine, originZ + (b - Ring) * fine});
    const auto shade = [&](int a, int b) -> const sim::SurfaceSample& { return samples[size_t(b + Ring) * Padded + size_t(a + Ring)]; };
    const auto at = [&](int i, int j) -> const sim::SurfaceSample& { return shade(i * ShadeDensity, j * ShadeDensity); };
    patch.heights.resize(PatchVertices);
    patch.normals.resize(size_t(PatchVertices) * 3);
    patch.colors.resize(size_t(PatchVertices) * 3);
    patch.low = std::numeric_limits<float>::max();
    patch.high = std::numeric_limits<float>::lowest();
    for (int j = 0; j < PatchSide; ++j)
        for (int i = 0; i < PatchSide; ++i) {
            const size_t vertex = size_t(j) * PatchSide + i;
            const sim::SurfaceSample& sample = at(i, j);
            patch.heights[vertex] = sample.ground;
            patch.dataLevel = vertex ? std::min(patch.dataLevel, uint8_t(sample.level)) : uint8_t(sample.level);
            patch.low = std::min(patch.low, sample.ground);
            patch.high = std::max(patch.high, sample.ground);
            const double slopeX = (at(i + 1, j).ground - at(i - 1, j).ground) / (2 * cell);
            const double slopeZ = (at(i, j + 1).ground - at(i, j - 1).ground) / (2 * cell);
            const double scale = 127.0 / std::sqrt(slopeX * slopeX + slopeZ * slopeZ + 1);
            patch.normals[vertex * 3] = int8_t(std::lround(-slopeX * scale));
            patch.normals[vertex * 3 + 1] = int8_t(std::lround(scale));
            patch.normals[vertex * 3 + 2] = int8_t(std::lround(-slopeZ * scale));
            const sim::Vec2 place{originX + i * cell, originZ + j * cell};
            const uint32_t color = materialInfo(iceAt(reader, sample, place) > GlacierDepth ? Material::Ice : surfaceMaterial(sample, TopVoxel)).color;
            patch.colors[vertex * 3] = uint8_t(color >> 16);
            patch.colors[vertex * 3 + 1] = uint8_t(color >> 8);
            patch.colors[vertex * 3 + 2] = uint8_t(color);

        }
    SurfaceReader shore = surfaceReader(world, sim::Level::Site);
    patch.water = standingWater(world, {originX, originZ, cell, PatchSide}, [&](int i, int j) {
        constexpr int known = Ring / ShadeDensity;
        if (i >= -known && j >= -known && i < PatchSide + known && j < PatchSide + known) return at(i, j).ground;
        return surfaceOrPlanet(shore, {originX + i * cell, originZ + j * cell}).ground;
    });

    // The shading raster, and with it how far the samples between the vertices stand off the
    // surface the vertices span: what one more level of geometry would add.
    patch.shade.resize(size_t(ShadeSide) * ShadeSide * 4);
    const int64_t latticeX = int64_t(key.x) * PatchCells * ShadeDensity, latticeZ = int64_t(key.z) * PatchCells * ShadeDensity;
    for (int b = 0; b < ShadeSide; ++b)
        for (int a = 0; a < ShadeSide; ++a) {
            const sim::SurfaceSample& sample = shade(a, b);
            const float here = sample.ground;
            const double slopeX = edgeSlope(shade(a - 1, b).ground, here, shade(a + 1, b).ground, fine);
            const double slopeZ = edgeSlope(shade(a, b - 1).ground, here, shade(a, b + 1).ground, fine);
            const double scale = 127.0 / std::sqrt(slopeX * slopeX + slopeZ * slopeZ + 1);
            float hidden = 0;
            for (const auto& ray : SkyRays) {
                double rise = 0;
                for (int reach = 1; reach <= SkyReach; ++reach)
                    rise = std::max(rise, (shade(a + ray[0] * reach, b + ray[1] * reach).ground - here) / (reach * fine * std::hypot(ray[0], ray[1])));
                hidden += float(rise / std::sqrt(1 + rise * rise));
            }
            uint8_t* texel = &patch.shade[(size_t(b) * ShadeSide + a) * 4];
            texel[0] = uint8_t(128 + std::lround(-slopeX * scale));
            texel[1] = uint8_t(128 + std::lround(-slopeZ * scale));
            texel[2] = uint8_t(iceAt(reader, sample, {originX + a * fine, originZ + b * fine}) > GlacierDepth
                                   ? Material::Ice : surfaceMaterial(sample, Dither[(latticeZ + b) & 3][(latticeX + a) & 3] / 16 * VoxelSize));
            texel[3] = uint8_t(std::lround(255.0f * (1 - hidden / 8)));

            const int da = a & 1, db = b & 1;
            if ((!da && !db) || sample.water > here + HiddenDepth) continue;
            const float spanned = 0.5f * (shade(a - da, b - db).ground + shade(a + da, b + db).ground);
            patch.error = std::max(patch.error, std::abs(here - spanned));
        }

    patch.morph = patch.heights;
    if (key.level < CoarsestLevel) {
        const sim::Level coarser = sourceFor(key.level + 1);
        SurfaceReader coarse = surfaceReader(world, coarser);
        const auto parent = [&](int i, int j) {
            if (coarser == reader.finest) return patch.heights[size_t(j) * PatchSide + i];
            return surfaceEased(coarse, {originX + i * cell, originZ + j * cell}).ground;
        };
        for (int j = 0; j < PatchSide; j += 2)
            for (int i = 0; i < PatchSide; i += 2) patch.morph[size_t(j) * PatchSide + i] = parent(i, j);
        const auto held = [&](int i, int j) { return patch.morph[size_t(j) * PatchSide + i]; };
        for (int j = 0; j < PatchSide; ++j)
            for (int i = 0; i < PatchSide; ++i) {
                if (!(i & 1) && !(j & 1)) continue;
                const int di = i & 1, dj = j & 1;
                patch.morph[size_t(j) * PatchSide + i] = 0.5f * (held(i - di, j - dj) + held(i + di, j + dj));
            }
    }

    patch.detail = easedBit(world, key);
    for (int probe = 0; probe < 5; ++probe) patch.detail |= levelBits(at(Probes[probe][0], Probes[probe][1]), probe);
    patch.skirt = float(std::max(4.0, cell));
    patch.base = std::floor(patch.low);
    for (auto* heights : {&patch.heights, &patch.morph, &patch.water})
        for (float& height : *heights) height = float(double(height) - patch.base);
    patch.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return patch;
}

} // namespace derivean

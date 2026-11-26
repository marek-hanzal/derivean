#include "nature/service/Nature.hpp"
#include "simulation/fn/Query.hpp"
#include "terrain/type/Column.hpp"

#include "PlanetSession.hpp"

#include <random>

using namespace derivean;

namespace {

bool same(const ColumnResult& a, const ColumnResult& b) {
    if (a.x != b.x || a.z != b.z || a.low != b.low || a.high != b.high || a.sky != b.sky || a.chunks.size() != b.chunks.size()) return false;
    for (size_t i = 0; i < a.chunks.size(); ++i) {
        const ColumnChunk& left = a.chunks[i];
        const ColumnChunk& right = b.chunks[i];
        if (left.voxels.key != right.voxels.key || left.voxels.occupied != right.voxels.occupied ||
            left.voxels.openFaces != right.voxels.openFaces || left.faces != right.faces ||
            left.voxels.tiles.size() != right.voxels.tiles.size()) return false;
        for (size_t t = 0; t < left.voxels.tiles.size(); ++t) {
            const auto& [leftKey, leftTile] = left.voxels.tiles[t];
            const auto& [rightKey, rightTile] = right.voxels.tiles[t];
            if (leftKey != rightKey || leftTile.uniform != rightTile.uniform || leftTile.occupied != rightTile.occupied ||
                bool(leftTile.detail) != bool(rightTile.detail) || (leftTile.detail && *leftTile.detail != *rightTile.detail)) return false;
        }
    }
    return true;
}

// A block of three by three columns around the point and, apart from it, one column that carries a
// player's hole and a block built above the ground.
std::vector<ColumnRequest> requests(Position around) {
    const Cell here = chunkOf(cellAt(around));
    std::vector<ColumnRequest> list;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) list.push_back({here.x + dx, here.z + dz, 0, true, here.y - 2, here.y + 1, true, {}});
    list.push_back({here.x + 4, here.z - 3, 0, true, 0, -1, true, {}});
    ChunkEdits edits;
    edits.fill(Untouched);
    for (size_t i = 0; i < 4000; ++i) edits[i * 7 % edits.size()] = uint8_t(Material::Air);
    list.back().edits.emplace_back(here.y - 1, edits);
    edits.fill(Untouched);
    edits[123] = uint8_t(Material::Basalt);
    list.back().edits.emplace_back(here.y + 6, edits);
    return list;
}

// Every chunk of the untouched columns, the grown ends of their bands included, against the
// simulation's own answer, and the faces built with the middle column against a mesh of the loaded voxels.
void matchesSimulation(sim::World& world, const std::vector<ColumnRequest>& list) {
    VoxelWorld loaded;
    loaded.streamed.emplace();
    std::mt19937 random(11);
    FaceMesh middle;
    Cell middleKey{};
    for (size_t i = 0; i + 1 < list.size(); ++i) {
        ColumnResult column = generateColumn(world, list[i]);
        loaded.streamed->sky[{column.x, 0, column.z}] = column.sky;
        for (ColumnChunk& chunk : column.chunks) {
            const Cell key = chunk.voxels.key;
            if (i == 4 && key.y == column.low + 1) {
                middle = chunk.faces;
                middleKey = key;
            }
            loadChunk(loaded, std::move(chunk.voxels));
            for (int probe = 0; probe < 80; ++probe) {
                const Cell cell = chunkBounds(key).min + Cell{int(random() % ChunkSide), int(random() % ChunkSide), int(random() % ChunkSide)};
                const Position center = cellCenter(cell);
                expect(materialAt(loaded, cell) == sim::materialAt(world, center[0], center[1], center[2]),
                       "A generated voxel differs from the simulation's material at its centre");
            }
        }
        const Cell above{column.x, column.sky, column.z};
        const Position open = cellCenter(chunkBounds(above).min);
        expect(sim::materialAt(world, open[0], open[1], open[2]) == Material::Air, "Rock stands above a column's sky");
    }
    const Bounds area = chunkBounds(middleKey);
    expect(!middle.empty() && middle == meshFaces(sampleWorld(loaded, area.padded(1)), area),
           "Faces built with a chunk differ from a mesh of the loaded voxels around it");
}

} // namespace

// The same columns from the same seed must hold the same voxels whatever built the tiles under
// them: many workers in the order the game happened to ask, or one worker asked back to front.
void testDeterminism(sim::World& world, Position around) {
    const std::vector<ColumnRequest> list = requests(around);
    std::vector<ColumnResult> first, again, alone;
    for (const ColumnRequest& request : list) first.push_back(generateColumn(world, request));
    for (auto request = list.rbegin(); request != list.rend(); ++request) again.push_back(generateColumn(world, *request));
    for (size_t i = 0; i < list.size(); ++i)
        expect(same(first[i], again[list.size() - 1 - i]), "A column generated twice differs from itself");
    expect(first.back().chunks.size() >= 3 && first.back().chunks.back().voxels.key.y == chunkOf(cellAt(around)).y + 6 &&
           first.back().chunks.back().voxels.occupied == 1, "Edits must be part of the generated column");
    matchesSimulation(world, list);

    const auto start = std::chrono::steady_clock::now();
    Nature single(NatureOptions{world.options.seed, world.options.age, 1});
    for (auto request = list.rbegin(); request != list.rend(); ++request) alone.push_back(generateColumn(single.world, *request));
    for (size_t i = 0; i < list.size(); ++i)
        expect(same(first[i], alone[list.size() - 1 - i]), "Voxels depend on the worker count or on the order tiles were built in");
    std::cout << "single-worker world generated the same columns in "
              << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << " s\n";
}

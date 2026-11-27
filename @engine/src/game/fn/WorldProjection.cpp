#include "game/type/WorldProjection.hpp"

#include <chrono>

namespace derivean {
namespace {

constexpr double MeshBudgetMs = 1.0;
constexpr Cell Sides[]{{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}, {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1}};

using Clock = std::chrono::steady_clock;

int distance(Cell a, Cell b) {
    return std::max({std::abs(a.x - b.x), std::abs(a.y - b.y), std::abs(a.z - b.z)});
}

Position chunkOrigin(Cell chunk) { return {chunk.x * ChunkMeters, chunk.y * ChunkMeters, chunk.z * ChunkMeters}; }

// Every voxel write is journalled; this is where the journal becomes the edits that outlive the
// chunk and the record the simulation will read.
void keepEdits(Game& game) {
    auto& journal = game.world.streamed->journal;
    if (journal.empty()) return;
    recordEdits(game.edits, journal, game.clock);
    std::optional<Cell> noted;
    for (const VoxelChange& change : journal) {
        const Cell chunk = chunkOf(change.point);
        if (noted == chunk) continue;
        noteEdited(game.stream, chunk);
        noted = chunk;
    }
    journal.clear();
}

const StreamColumn* resolved(const Game& game, int x, int z) {
    const auto column = game.stream.columns.find({x, 0, z});
    return column != game.stream.columns.end() && column->second.resolved ? &column->second : nullptr;
}

// Faces towards rock that is not loaded are left out; the chunk is meshed again when that rock arrives.
ChunkMesh meshChunk(Game& game, Cell key) {
    FaceMesh faces;
    if (game.world.chunks.contains(key)) {
        const Bounds area = chunkBounds(key);
        faces = meshFaces(sampleWorld(game.world, area.padded(1)), area);
    }
    // Faces and their corner occlusion read every chunk that touches this one.
    bool settled = true;
    for (int y = -1; y <= 1; ++y)
        for (int z = -1; z <= 1; ++z)
            for (int x = -1; x <= 1; ++x) settled &= known(game.world, key + Cell{x, y, z});
    if (settled) game.view.provisional.erase(key);
    else game.view.provisional.insert(key);
    game.visible.insert(key);
    game.view.boxStale = true;
    return {key, chunkOrigin(key), std::move(faces)};
}

void absorb(Game& game, Frame& frame) {
    TerrainStream& stream = game.stream;
    StreamView& view = game.view;
    for (Cell key : stream.dropped) {
        if (game.visible.erase(key)) {
            frame.unloaded.push_back(key);
            view.water.erase(key);
        }
        view.provisional.erase(key);
        game.dirtyTerrain.erase(key);
    }
    // Faces built with a chunk assume untouched ground beside it and, above and below, what its own
    // task produced. An edit made next to it since the task was ordered voids them.
    const auto edited = [&](Cell key) {
        return game.edits.chunks.contains(key + Cell{0, 1, 0}) || game.edits.chunks.contains(key + Cell{0, -1, 0}) ||
               std::any_of(std::begin(Sides), std::end(Sides), [&](Cell side) { return game.edits.levels.contains({key.x + side.x, 0, key.z + side.z}); });
    };
    for (LoadedChunk& chunk : stream.loaded) {
        // A column can enter the world and be dropped again by the plan of the same frame.
        if (!game.world.streamed->loaded.contains(chunk.key)) continue;
        if (chunk.occupied) {
            if (chunk.meshed && !edited(chunk.key)) {
                frame.chunks.push_back({chunk.key, chunkOrigin(chunk.key), std::move(chunk.faces)});
                game.visible.insert(chunk.key);
            } else {
                view.queue.push_back(chunk.key);
            }
        }
        if (view.provisional.empty()) continue;
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
                for (int x = -1; x <= 1; ++x)
                    if (const Cell beside = chunk.key + Cell{x, y, z}; view.provisional.contains(beside)) view.queue.push_back(beside);
    }
    if (!stream.dropped.empty() || !stream.loaded.empty()) view.boxStale = true;
    stream.dropped.clear();
    stream.loaded.clear();
}

void meshQueue(Game& game, Frame& frame, Cell center) {
    StreamView& view = game.view;
    if (view.queue.empty()) return;
    std::sort(view.queue.begin(), view.queue.end(), [&](Cell a, Cell b) {
        const int da = distance(a, center), db = distance(b, center);
        return da != db ? da < db : a < b;
    });
    view.queue.erase(std::unique(view.queue.begin(), view.queue.end()), view.queue.end());
    const auto start = Clock::now();
    size_t done = 0;
    while (done < view.queue.size()) {
        const Cell key = view.queue[done++];
        if (!game.world.streamed->loaded.contains(key)) continue;
        frame.chunks.push_back(meshChunk(game, key));
        if (std::chrono::duration<double, std::milli>(Clock::now() - start).count() > MeshBudgetMs) break;
    }
    view.queue.erase(view.queue.begin(), view.queue.begin() + done);
}

void projectWater(Game& game, Frame& frame, Cell center, const std::unordered_set<Cell, CellHash>& edited) {
    StreamView& view = game.view;
    std::erase_if(view.water, [&](Cell key) {
        if (game.water.chunks.contains(key) && distance(key, center) <= Visibility) return false;
        frame.waterMeshes.push_back({key, chunkOrigin(key), {}});
        return true;
    });
    const bool waterDue = game.water.ticks >= game.lastWaterProjection + 2;
    for (const auto& [key, bricks] : game.water.chunks) {
        if (distance(key, center) > Visibility) continue;
        const bool enters = view.water.insert(key).second;
        if (!enters && !edited.contains(key) && !(waterDue && game.water.meshDirty.contains(key))) continue;
        frame.waterMeshes.push_back({key, chunkOrigin(key), meshWater(game.water, game.world, key)});
        game.water.meshDirty.erase(key);
    }
    if (waterDue) game.lastWaterProjection = game.water.ticks;
    std::erase_if(game.water.meshDirty, [&](Cell key) { return !view.water.contains(key); });
}

// The largest square of columns around the player in which every column is in and meshed.
std::optional<VoxelBox> voxelBox(const Game& game, Cell center) {
    const auto complete = [&](int x, int z, int& low, int& high) {
        const StreamColumn* column = resolved(game, x, z);
        if (!column) return false;
        for (int level : column->levels)
            if (const Cell key{x, level, z}; game.world.chunks.contains(key) && !game.visible.contains(key)) return false;
        low = std::min(low, column->low);
        high = std::max(high, column->high);
        return true;
    };
    int low = INT32_MAX, high = INT32_MIN, radius = -1;
    for (int ring = 0; ring <= LoadRadius; ++ring) {
        bool whole = true;
        int ringLow = low, ringHigh = high;
        // The ring's columns in rows: whole rows at its ends, the two sides between them.
        for (int dz = -ring; dz <= ring && whole; ++dz) {
            const int step = std::abs(dz) == ring ? 1 : 2 * ring;
            for (int dx = -ring; dx <= ring && whole; dx += step) whole = complete(center.x + dx, center.z + dz, ringLow, ringHigh);
        }
        if (!whole) break;
        radius = ring;
        low = ringLow;
        high = ringHigh;
    }
    if (radius < 0) return std::nullopt;
    return VoxelBox{{(center.x - radius) * ChunkMeters, low * ChunkMeters, (center.z - radius) * ChunkMeters},
                    {(center.x + radius + 1) * ChunkMeters, (high + 1) * ChunkMeters, (center.z + radius + 1) * ChunkMeters}};
}

} // namespace

void projectScene(Game& game, Frame& frame) {
    StreamView& view = game.view;
    const Cell center = chunkOf(cellAt(game.physics.eye()));
    keepEdits(game);
    absorb(game, frame);

    const std::unordered_set<Cell, CellHash> edited = std::move(game.dirtyTerrain);
    game.dirtyTerrain.clear();
    for (Cell key : edited) {
        // An edit can also put the first voxel into a chunk that was open sky.
        if (!game.visible.contains(key) && !(game.world.chunks.contains(key) && game.world.streamed->loaded.contains(key))) continue;
        frame.chunks.push_back(meshChunk(game, key));
    }
    meshQueue(game, frame, center);
    projectWater(game, frame, center, edited);

    if (view.boxStale || center != view.boxCenter) view.box = voxelBox(game, center);
    view.boxStale = false;
    view.boxCenter = center;
    frame.voxelBox = view.box;
}

void markTerrainChanged(Game& game, std::span<const Cell> changed) {
    if (changed.empty()) return;
    keepEdits(game);
    for (Cell point : changed) {
        game.editedTiles.insert(tileOf(point));
        const Cell key = chunkOf(point);
        game.dirtyColliders.insert(key);
        game.dirtyTerrain.insert(key);
        // Faces and their corner occlusion read every cell that touches the changed one.
        const Cell inside = point - key * ChunkSide;
        if (std::min({inside.x, inside.y, inside.z}) > 0 && std::max({inside.x, inside.y, inside.z}) < ChunkSide - 1) continue;
        for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
                for (int x = -1; x <= 1; ++x) game.dirtyTerrain.insert(chunkOf(point + Cell{x, y, z}));
    }
    wakeWater(game.water, changed);
    game.physics.wakeDebris(boundsOf(changed).padded(1));
}

} // namespace derivean

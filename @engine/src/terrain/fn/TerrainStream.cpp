#include "terrain/type/TerrainStream.hpp"
#include "nature/service/Nature.hpp"
#include "nature/type/SurfaceReader.hpp"
#include "particles/type/VoxelContact.hpp"
#include "ground/type/Clast.hpp"
#include "relief/type/Form.hpp"
#include "simulation/fn/Query.hpp"

#include <chrono>
#include <cmath>

namespace derivean {
namespace {

constexpr double CommitBudgetMs = 0.6;
constexpr int ReplanCalls = 8;
constexpr int MaxDrops = 32;            // columns unloaded in one frame; leaving the ground must not unload it all at once
constexpr double StreamSpeed = 100.0;   // m/s above which columns would be stale before they arrive
// Metres of site and of ground surface generateColumn reads around a column: what sim::sampleBlock
// asks for around a block, and a voxel more for the faces of the chunks next door. A column is
// ordered only once those tiles are finished; otherwise the task would build them itself and hold
// an engine worker for seconds.
constexpr double SitePadding = sim::BlockSiteReach + 2 * VoxelSize;
constexpr double GroundPadding = sim::BlockGroundReach + 2 * VoxelSize;

using Clock = std::chrono::steady_clock;

int chebyshev(Cell a, Cell b) { return std::max(std::abs(a.x - b.x), std::abs(a.z - b.z)); }

Cell columnOf(Cell chunk) { return {chunk.x, 0, chunk.z}; }

bool tilesFinished(const sim::World& world, Cell column) {
    const sim::Vec2 middle{(column.x + 0.5) * ChunkMeters, (column.z + 0.5) * ChunkMeters};
    return sim::readyAround(world, middle, 0.5 * ChunkMeters + SitePadding, sim::Level::Site) &&
           sim::readyAround(world, middle, 0.5 * ChunkMeters + GroundPadding, sim::Level::Ground);
}

void drop(TerrainStream& stream, VoxelWorld& world, Cell column, StreamColumn& state) {
    for (int level : state.levels) {
        const Cell key{column.x, level, column.z};
        unloadChunk(world, key);
        stream.dropped.push_back(key);
    }
    world.streamed->sky.erase(column);
}

void commit(TerrainStream& stream, VoxelWorld& world, ColumnResult&& result) {
    --stream.flying;
    const Cell column{result.x, 0, result.z};
    const auto found = stream.columns.find(column);
    if (found == stream.columns.end() || found->second.ticket != result.ticket) return;
    StreamColumn& state = found->second;
    state.ticket = 0;
    for (ColumnChunk& chunk : result.chunks) {
        ChunkVoxels& voxels = chunk.voxels;
        if (world.streamed->loaded.contains(voxels.key)) continue;
        state.levels.push_back(voxels.key.y);
        stream.loaded.push_back({voxels.key, voxels.openFaces, voxels.occupied != 0, chunk.meshed, std::move(chunk.faces)});
        loadChunk(world, std::move(voxels));
        ++stream.builtChunks;
    }
    ++stream.builtColumns;
    stream.builtMs += result.milliseconds;
    if (result.surface) {
        state.resolved = true;
        state.low = result.low;
        state.high = result.high;
        world.streamed->sky[column] = result.sky;
    }
    stream.generationMs += result.milliseconds;
}

void submit(TerrainStream& stream, Nature& nature, const EditStore& edits, Cell column, StreamColumn& state,
            bool surface, int low, int high) {
    ColumnRequest request{column.x, column.z, ++stream.tickets, surface, low, high, true, {}};
    // A face's occlusion reads the cells diagonally across an edge too.
    for (int z = -1; z <= 1; ++z)
        for (int x = -1; x <= 1; ++x)
            if (x || z) request.mesh &= !edits.levels.contains(column + Cell{x, 0, z});
    if (const auto levels = edits.levels.find(column); levels != edits.levels.end()) {
        for (int level : levels->second) {
            // An edited level that is not part of this order still borders it; only the game thread sees both.
            if (!surface && (level == low - 1 || level == high + 1)) request.mesh = false;
            if (surface || (level >= low && level <= high))
                request.edits.emplace_back(level, *edits.chunks.at({column.x, level, column.z}));
        }
    }
    state.ticket = request.ticket;
    ++stream.flying;
    nature.crew->submit([&world = nature.world, box = stream.finished, request = std::move(request)] {
        box->post(generateColumn(world, request));
    });
}

// Levels inside the player's reach that the column does not hold yet and cannot assume to be air.
bool reachMissing(const VoxelWorld& world, Cell column, Cell here, int& low, int& high) {
    low = here.y + ReachRadius + 1;
    high = here.y - ReachRadius - 1;
    for (int level = here.y - ReachRadius; level <= here.y + ReachRadius; ++level) {
        if (known(world, {column.x, level, column.z})) continue;
        low = std::min(low, level);
        high = std::max(high, level);
    }
    return high >= low;
}

void trim(TerrainStream& stream, VoxelWorld& world, const EditStore& edits, Cell column, StreamColumn& state, Cell here) {
    const auto edited = edits.levels.find(column);
    const bool near = chebyshev(column, here) <= ReachRadius + 2;
    std::erase_if(state.levels, [&](int level) {
        if (level >= state.low && level <= state.high) return false;
        if (near && std::abs(level - here.y) <= ReachRadius + 2) return false;
        if (edited != edits.levels.end() && std::find(edited->second.begin(), edited->second.end(), level) != edited->second.end())
            return false;
        const Cell key{column.x, level, column.z};
        unloadChunk(world, key);
        stream.dropped.push_back(key);
        return true;
    });
}

sim::Vec2 middleOf(Cell column) { return {(column.x + 0.5) * ChunkMeters, (column.z + 0.5) * ChunkMeters}; }

size_t capacity(const Nature& nature) { return 2 * size_t(nature.crew->threads()) + 2; }

// Orders columns from the backlog, nearest first, while the crew has room for them.
void order(TerrainStream& stream, Nature& nature, const EditStore& edits, Cell here) {
    SurfaceReader reader = surfaceReader(nature.world);
    while (!stream.backlog.empty() && stream.flying < capacity(nature)) {
        const Cell column = stream.backlog.back();
        stream.backlog.pop_back();
        if (stream.columns.contains(column) || !tilesFinished(nature.world, column)) continue;
        StreamColumn& state = stream.columns[column];
        state.ground = surfaceAt(reader, middleOf(column)).ground;
        const bool reach = chebyshev(column, here) <= ReachRadius;
        submit(stream, nature, edits, column, state, true, reach ? here.y - ReachRadius : 0, reach ? here.y + ReachRadius : -1);
    }
}

void plan(TerrainStream& stream, Nature& nature, VoxelWorld& world, const EditStore& edits, Position feet, Cell here, double speed) {
    int drops = 0;
    for (auto it = stream.columns.begin(); it != stream.columns.end();) {
        StreamColumn& state = it->second;
        const bool far = chebyshev(it->first, here) > KeepRadius || feet[1] - state.ground > KeepHeight;
        if (far && drops < MaxDrops) {
            drop(stream, world, it->first, state);
            it = stream.columns.erase(it);
            ++drops;
            continue;
        }
        if (state.resolved && !far) trim(stream, world, edits, it->first, state, here);
        ++it;
    }
    stream.planned = drops < MaxDrops;

    SurfaceReader reader = surfaceReader(nature.world);
    std::vector<std::pair<int, Cell>> wanted;
    stream.missing = 0;
    for (int dz = -LoadRadius; dz <= LoadRadius; ++dz)
        for (int dx = -LoadRadius; dx <= LoadRadius; ++dx) {
            const Cell column{here.x + dx, 0, here.z + dz};
            const auto found = stream.columns.find(column);
            if (found != stream.columns.end() && found->second.resolved) {
                int low, high;
                if (!found->second.ticket && chebyshev(column, here) <= ReachRadius && stream.flying < capacity(nature) &&
                    reachMissing(world, column, here, low, high))
                    submit(stream, nature, edits, column, found->second, false, low, high);
                continue;
            }
            const sim::SurfaceSample ground = surfaceAt(reader, middleOf(column));
            if (!ground.found || feet[1] - ground.ground > LoadHeight) continue;
            ++stream.missing;
            if (found == stream.columns.end() && ground.level == sim::Level::Site) wanted.emplace_back(dx * dx + dz * dz, column);
        }
    stream.backlog.clear();
    if (speed > StreamSpeed) return;
    std::sort(wanted.begin(), wanted.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    for (const auto& [distance, column] : wanted) stream.backlog.push_back(column);
}

} // namespace

void advanceStream(TerrainStream& stream, Nature& nature, VoxelWorld& world, const EditStore& edits,
                   Position feet, double speed) {
    stream.generationMs = 0;
    for (ColumnResult& result : stream.finished->collect()) stream.arrived.push_back(std::move(result));
    const auto start = Clock::now();
    while (!stream.arrived.empty()) {
        commit(stream, world, std::move(stream.arrived.front()));
        stream.arrived.pop_front();
        if (std::chrono::duration<double, std::milli>(Clock::now() - start).count() > CommitBudgetMs) break;
    }

    const Cell here = chunkOf(cellAt(feet));
    if (!stream.planned || here != stream.center || ++stream.calls % ReplanCalls == 0) {
        stream.center = here;
        plan(stream, nature, world, edits, feet, here, speed);
    }
    order(stream, nature, edits, here);
}

void noteEdited(TerrainStream& stream, Cell chunk) {
    const auto found = stream.columns.find(columnOf(chunk));
    if (found == stream.columns.end()) return;
    auto& levels = found->second.levels;
    if (std::find(levels.begin(), levels.end(), chunk.y) == levels.end()) levels.push_back(chunk.y);
}

} // namespace derivean

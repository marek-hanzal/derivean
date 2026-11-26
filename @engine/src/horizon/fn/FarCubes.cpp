#include "horizon/type/FarCubes.hpp"
#include "nature/service/Nature.hpp"

#include <algorithm>
#include <cmath>

namespace derivean {
namespace {

constexpr int Margin = 1;                // chunks built beyond a level's reach, so its box stays whole while the eye moves
constexpr double KeepFactor = 1.25;      // a level in use stays until the eye is this much further from needing it
constexpr double Foresight = 2.0;        // s of travel a level's reach must outlast to be worth building
constexpr size_t MaxAddedPerFrame = 32;
constexpr size_t AuditSlice = 64;        // shown chunks compared with the finished tiles per frame
constexpr int BelowVoxels = 64;          // what a build's priority lies under that of the voxels round the player

double reachOf(const FarCubes& far, int level) { return level == CoarsestCubes ? CubeView : far.reach * edgeOf(level); }
CubeLevel& levelOf(FarCubes& far, int level) { return far.levels[size_t(level - FinestCubes)]; }
int ringOf(Cell a, Cell b) { return std::max(std::abs(a.x - b.x), std::abs(a.z - b.z)); }

// Metres from the eye to the nearest point of a chunk's square, along the axis it is further on.
double squareDistance(Position eye, int level, Cell chunk) {
    const double size = CubeSide * edgeOf(level);
    return std::max({chunk.x * size - eye[0], eye[0] - (chunk.x + 1) * size, chunk.z * size - eye[2], eye[2] - (chunk.z + 1) * size, 0.0});
}

void forget(FarCubes& far, CubeLevel& level, CubeSlot& slot, CubeDelta& delta) {
    if (slot.shown.empty()) return;
    delta.removed.insert(delta.removed.end(), slot.shown.begin(), slot.shown.end());
    far.shownByData[slot.dataLevel] -= slot.shown.size();
    --level.shown;
    slot.shown.clear();
}

void rest(FarCubes& far, CubeLevel& level, CubeDelta& delta) {
    for (auto& [chunk, slot] : level.slots) forget(far, level, slot, delta);
    level.slots.clear();
    level.todo.clear();
    level.active = false;
}

// The square of chunks around the eye that the level wants, and the order to build what is missing.
void plan(FarCubes& far, int index, Position eye, CubeDelta& delta) {
    CubeLevel& level = levelOf(far, index);
    const double size = CubeSide * edgeOf(index);
    const Cell centre{int(std::floor(eye[0] / size)), 0, int(std::floor(eye[2] / size))};
    if (level.active && centre == level.centre) return;
    level.active = true;
    level.centre = centre;
    const int load = int(std::ceil(reachOf(far, index) / size)) + Margin;
    for (auto slot = level.slots.begin(); slot != level.slots.end();) {
        if (ringOf(slot->first, centre) <= load + 1) { ++slot; continue; }
        forget(far, level, slot->second, delta);
        slot = level.slots.erase(slot);
    }
    level.todo.clear();
    for (int z = centre.z - load; z <= centre.z + load; ++z)
        for (int x = centre.x - load; x <= centre.x + load; ++x) {
            const CubeSlot& slot = level.slots[{x, 0, z}];
            if ((slot.shown.empty() || slot.stale) && !slot.built && !slot.ticket) level.todo.push_back({x, 0, z});
        }
    // What is missing shrinks a box; what is only stale waits behind it.
    const auto later = [&](Cell chunk) { return std::pair(level.slots.at(chunk).shown.empty(), -ringOf(chunk, centre)); };
    std::sort(level.todo.begin(), level.todo.end(), [&](Cell a, Cell b) { return later(a) < later(b); });
}

void receive(FarCubes& far) {
    for (CubeColumn& column : far.finished->collect()) {
        --far.flying;
        CubeLevel& level = levelOf(far, column.key.level);
        const auto slot = level.slots.find({column.key.x, 0, column.key.z});
        if (slot == level.slots.end() || slot->second.ticket != column.ticket) continue;
        ++level.builds;
        level.buildMs += column.milliseconds;
        level.faces += column.faces;
        slot->second.ticket = 0;
        far.ready.push_back(column.key);
        slot->second.built = std::make_unique<CubeColumn>(std::move(column));
    }
}

// Shown columns whose ground the simulation now knows better go back on their level's list, behind
// what is missing.
void compare(FarCubes& far, const sim::World& world) {
    if (far.audit.empty() && world.atlas.finished() != far.tilesSeen) {
        far.tilesSeen = world.atlas.finished();
        for (int index = FinestCubes; index <= CoarsestCubes; ++index)
            for (const auto& [chunk, slot] : levelOf(far, index).slots)
                if (!slot.shown.empty()) far.audit.push_back({index, chunk.x, chunk.z});
    }
    for (size_t checked = 0; checked < AuditSlice && !far.audit.empty(); ++checked) {
        const CubeKey key = far.audit.back();
        far.audit.pop_back();
        CubeLevel& level = levelOf(far, key.level);
        const auto slot = level.slots.find({key.x, 0, key.z});
        if (slot == level.slots.end() || slot->second.shown.empty() || slot->second.ticket || slot->second.built) continue;
        if (slot->second.stale || cubeDetail(world, key) == slot->second.detail) continue;
        slot->second.stale = true;
        level.todo.insert(level.todo.begin(), {key.x, 0, key.z});
    }
}

// Coarse levels first: they are the whole view until the finer ones arrive. All of it queues
// behind the voxel columns and colliders the player stands on.
void build(FarCubes& far, Nature& nature) {
    const size_t capacity = 2 * size_t(nature.crew->threads());
    for (int index = CoarsestCubes; index >= FinestCubes && far.flying < capacity; --index) {
        CubeLevel& level = levelOf(far, index);
        while (far.flying < capacity && !level.todo.empty()) {
            const Cell chunk = level.todo.back();
            level.todo.pop_back();
            const auto slot = level.slots.find(chunk);
            if (slot == level.slots.end() || slot->second.ticket || slot->second.built) continue;
            slot->second.ticket = ++far.tickets;
            slot->second.stale = false;
            ++far.flying;
            nature.crew->submit([&world = nature.world, box = far.finished, key = CubeKey{index, chunk.x, chunk.z}, ticket = far.tickets] {
                CubeColumn built = buildCubes(world, key);
                built.ticket = ticket;
                box->post(std::move(built));
            }, index - BelowVoxels);
        }
    }
}

void show(FarCubes& far, CubeDelta& delta) {
    size_t taken = 0;
    for (size_t columns = 0; taken < far.ready.size() && columns < MaxAddedPerFrame; ++taken) {
        const CubeKey key = far.ready[taken];
        CubeLevel& level = levelOf(far, key.level);
        const auto found = level.slots.find({key.x, 0, key.z});
        if (found == level.slots.end() || !found->second.built) continue;
        ++columns;
        CubeSlot& slot = found->second;
        forget(far, level, slot, delta);
        slot.detail = slot.built->detail;
        slot.dataLevel = slot.built->chunks.front().dataLevel;
        for (CubeChunk& chunk : slot.built->chunks) {
            slot.shown.push_back(chunk.id = ++far.ids);
            delta.added.push_back(std::move(chunk));
        }
        far.shownByData[slot.dataLevel] += slot.shown.size();
        ++level.shown;
        slot.built.reset();
        // A tile may have finished between the build's samples and this frame.
        far.audit.push_back(key);
    }
    far.ready.erase(far.ready.begin(), far.ready.begin() + std::ptrdiff_t(taken));
}

} // namespace

void advanceCubes(FarCubes& far, Nature& nature, Position eye, double clearance, double speed, CubeDelta& delta) {
    receive(far);
    for (int index = CoarsestCubes; index >= FinestCubes; --index) {
        CubeLevel& level = levelOf(far, index);
        const double reach = reachOf(far, index) * (level.active ? KeepFactor : 1.0);
        if (clearance < reach && speed * Foresight < reach) plan(far, index, eye, delta);
        else if (level.active) rest(far, level, delta);
    }
    compare(far, nature.world);
    build(far, nature);
    show(far, delta);

    // A level may be drawn as far as every chunk of it is held, and no further than the coarser one.
    far.missing = 0;
    double outer = CubeView;
    for (int index = CoarsestCubes; index >= FinestCubes; --index) {
        CubeLevel& level = levelOf(far, index);
        level.box.reset();
        if (!level.active) { outer = 0; continue; }
        double side = std::min(outer, reachOf(far, index));
        for (const auto& [chunk, slot] : level.slots) {
            if (!slot.shown.empty()) continue;
            const double distance = squareDistance(eye, index, chunk);
            far.missing += distance < reachOf(far, index);
            side = std::min(side, distance - CubeBand * edgeOf(index));
        }
        outer = std::max(side, 0.0);
        if (side > 0) level.box = LevelBox{{eye[0] - side, eye[1] - side, eye[2] - side}, {eye[0] + side, eye[1] + side, eye[2] + side}};
    }
}

} // namespace derivean

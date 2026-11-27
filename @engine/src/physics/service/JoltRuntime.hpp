#pragma once

#include "structure/type/Collapse.hpp"
#include "particles/type/VoxelContact.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <array>
#include <memory>
#include <optional>

namespace derivean {

inline constexpr float PlayerEyeHeight = 1.62f;
inline constexpr float PlayerRadius = 0.31f;
inline constexpr float PlayerHeight = 1.72f;

using Rotation = std::array<float, 4>;
using TerrainShape = JPH::RefConst<JPH::Shape>;

struct Pose {
    Position position;
    Rotation rotation;
};

struct RayHit {
    Cell voxel;
    bool terrain;
    Cell face;
};

// What the player's body does in one step. The game decides; the runtime only carries it out.
struct PlayerMove {
    enum class Mode { Walk, Fly, Hold };
    Mode mode{Mode::Walk};
    std::array<float, 3> velocity{};   // m/s asked for: along the ground on foot, in any direction in flight
    bool jump{};
    float lift{};                      // share of gravity standing water carries; above one it pushes up
};

// Collider of a block of voxels, relative to its minimum corner. Safe on any thread once a
// JoltRuntime exists.
TerrainShape terrainShape(const Voxels& voxels);

class JoltRuntime {
public:
    JoltRuntime();
    ~JoltRuntime();
    JoltRuntime(const JoltRuntime&) = delete;
    JoltRuntime& operator=(const JoltRuntime&) = delete;

    JPH::BodyID addTerrain(const Voxels& voxels);
    JPH::BodyID addTerrain(const TerrainShape& shape, Cell origin);
    JPH::BodyID addDebris(const VoxelPiece& piece);
    void removeBody(JPH::BodyID id);
    void wakeDebris(Bounds area);
    // A suspended body keeps its place and takes no part in the dynamics until it is released,
    // when it goes on with the motion it had.
    void suspend(JPH::BodyID id, bool suspended);
    // pose, extent and verticalExtent take no lock: they are called on the game thread only, never
    // during an update, when Jolt's own jobs move the bodies.
    Pose pose(JPH::BodyID id) const;
    std::pair<Position, Position> extent(JPH::BodyID id) const;   // corners of the body's bounds
    Position eye() const;
    Position feet() const;
    Position velocity() const;
    void returnPlayer(Position feet);
    std::pair<double, double> verticalExtent(JPH::BodyID id) const;
    std::optional<MovingContact> sweepMoving(Position from, Position to, double half) const;
    bool overlapsMovingVoxel(Cell point) const;
    void movePlayer(float dt, const PlayerMove& move);
    std::optional<RayHit> aim(Position direction) const;

private:
    struct Resources;
    std::unique_ptr<Resources> resources_;
};

} // namespace derivean

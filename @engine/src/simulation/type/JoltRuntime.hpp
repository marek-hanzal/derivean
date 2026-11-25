#pragma once

#include "structure/type/Collapse.hpp"
#include "particles/type/VoxelContact.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

#include <array>
#include <memory>
#include <optional>

namespace derivean {

inline constexpr float PlayerEyeHeight = 1.62f;
inline constexpr float PlayerRadius = 0.31f;
inline constexpr float PlayerHeight = 1.72f;

using Rotation = std::array<float, 4>;

struct Pose {
    Position position;
    Rotation rotation;
};

struct RayHit {
    Cell voxel;
    bool terrain;
    Cell face;
};

class JoltRuntime {
public:
    JoltRuntime();
    ~JoltRuntime();
    JoltRuntime(const JoltRuntime&) = delete;
    JoltRuntime& operator=(const JoltRuntime&) = delete;

    JPH::BodyID addTerrain(const Voxels& voxels);
    JPH::BodyID addDebris(const VoxelPiece& piece);
    void removeBody(JPH::BodyID id);
    void wakeDebris(Bounds area);
    Pose pose(JPH::BodyID id) const;
    Position eye() const;
    void returnPlayer(Position feet);
    std::pair<double, double> verticalExtent(JPH::BodyID id) const;
    std::optional<MovingContact> sweepMoving(Position from, Position to, double half) const;
    bool overlapsMovingVoxel(Cell point) const;
    void movePlayer(float dt, float x, float z, bool jump, bool sprint);
    std::optional<RayHit> aim(Position direction) const;

private:
    struct Resources;
    std::unique_ptr<Resources> resources_;
};

} // namespace derivean

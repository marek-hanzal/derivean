#include "physics/service/JoltResources.hpp"
#include "voxel/type/Mesh.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/RegisterTypes.h>

#include <cstdarg>
#include <cstdio>

namespace derivean {
namespace {

void trace(const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
}

class InitializeJolt {
public:
    InitializeJolt() {
        JPH::RegisterDefaultAllocator();
        JPH::Trace = trace;
        JPH::Factory::sInstance = &factory_;
        JPH::RegisterTypes();
    }
    ~InitializeJolt() {
        JPH::UnregisterTypes();
        JPH::Factory::sInstance = nullptr;
    }
private:
    JPH::Factory factory_;
};

} // namespace

TerrainShape terrainShape(const Voxels& voxels) {
    const Cell origin = voxels.bounds.min;
    JPH::StaticCompoundShapeSettings settings;
    for (const Bounds box : mergeSolids(voxels)) {
        const Cell extent = box.size();
        const Cell minimum = box.min - origin;
        const JPH::Vec3 half = JPH::Vec3(float(extent.x), float(extent.y), float(extent.z)) * (0.5f * VoxelSize);
        const JPH::Vec3 position(float(minimum.x) * VoxelSize, float(minimum.y) * VoxelSize, float(minimum.z) * VoxelSize);
        settings.AddShape(position + half, JPH::Quat::sIdentity(), new JPH::BoxShape(half, 0.006f));
    }
    const auto shape = settings.Create();
    if (shape.HasError()) throw std::runtime_error(shape.GetError().c_str());
    return shape.Get();
}

JoltRuntime::JoltRuntime() {
    static InitializeJolt library;
    resources_ = std::make_unique<Resources>();
}

JoltRuntime::~JoltRuntime() = default;

JPH::BodyID JoltRuntime::addTerrain(const Voxels& voxels) {
    return addTerrain(terrainShape(voxels), voxels.bounds.min);
}

JPH::BodyID JoltRuntime::addTerrain(const TerrainShape& shape, Cell origin) {
    JPH::BodyCreationSettings settings(shape, meters(origin), JPH::Quat::sIdentity(), JPH::EMotionType::Static, TerrainLayer);
    settings.mFriction = 0.83f;
    return resources_->addBody(settings, JPH::EActivation::DontActivate);
}

JPH::BodyID JoltRuntime::addDebris(const VoxelPiece& piece) {
    auto shape = terrainShape(piece.voxels);
    JPH::BodyCreationSettings settings(shape, meters(piece.voxels.bounds.min), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, MovingLayer);
    settings.mFriction = piece.friction;
    settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = piece.mass;
    settings.mMaxLinearVelocity = 45.0f;
    return resources_->addBody(settings, JPH::EActivation::Activate);
}

void JoltRuntime::removeBody(JPH::BodyID id) {
    resources_->halted.erase(id.GetIndexAndSequenceNumber());
    auto& bodies = resources_->system.GetBodyInterface();
    bodies.RemoveBody(id);
    bodies.DestroyBody(id);
}

void JoltRuntime::wakeDebris(Bounds area) {
    const JPH::AABox bounds(meters(area.min), meters(area.max));
    resources_->system.GetBodyInterface().ActivateBodiesInAABox(bounds, {}, DebrisOnly{});
}

void JoltRuntime::suspend(JPH::BodyID id, bool suspended) {
    auto& bodies = resources_->system.GetBodyInterface();
    auto& halted = resources_->halted;
    if (suspended) {
        // Jolt drops the velocities of a body that turns static.
        halted[id.GetIndexAndSequenceNumber()] = {bodies.GetLinearVelocity(id), bodies.GetAngularVelocity(id)};
        bodies.SetMotionType(id, JPH::EMotionType::Static, JPH::EActivation::DontActivate);
        return;
    }
    bodies.SetMotionType(id, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
    if (const auto motion = halted.find(id.GetIndexAndSequenceNumber()); motion != halted.end()) {
        bodies.SetLinearAndAngularVelocity(id, motion->second.first, motion->second.second);
        halted.erase(motion);
    }
}

Pose JoltRuntime::pose(JPH::BodyID id) const {
    JPH::RVec3 position;
    JPH::Quat rotation;
    resources_->system.GetBodyInterfaceNoLock().GetPositionAndRotation(id, position, rotation);
    return {asPosition(position), {rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW()}};
}

std::pair<Position, Position> JoltRuntime::extent(JPH::BodyID id) const {
    JPH::BodyLockRead lock(resources_->system.GetBodyLockInterfaceNoLock(), id);
    const auto& box = lock.GetBody().GetWorldSpaceBounds();
    return {asPosition(JPH::RVec3(box.mMin)), asPosition(JPH::RVec3(box.mMax))};
}

std::pair<double, double> JoltRuntime::verticalExtent(JPH::BodyID id) const {
    JPH::BodyLockRead lock(resources_->system.GetBodyLockInterfaceNoLock(), id);
    const auto& box = lock.GetBody().GetWorldSpaceBounds();
    return {box.mMin.GetY(), box.mMax.GetY()};
}

} // namespace derivean

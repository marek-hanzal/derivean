/* long-file: 270 lines; Jolt lifecycle, filters and shape queries use one private resource owner. */
#include "simulation/type/JoltRuntime.hpp"
#include "physics/constant/Gravity.hpp"
#include "voxel/type/Mesh.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>

namespace derivean {
namespace {

constexpr JPH::ObjectLayer Terrain = 0;
constexpr JPH::ObjectLayer Moving = 1;
constexpr float Reach = 6.0f;

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

struct Layers final : JPH::BroadPhaseLayerInterface {
    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return JPH::BroadPhaseLayer(JPH::uint8(layer));
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        return layer.GetValue() == Terrain ? "Terrain" : "Moving";
    }
#endif
};

struct BroadPhasePairs final : JPH::ObjectVsBroadPhaseLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer other) const override {
        return layer != Terrain || other.GetValue() != Terrain;
    }
};

struct DebrisOnly final : JPH::ObjectLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer layer) const override { return layer == Moving; }
};

struct ObjectPairs final : JPH::ObjectLayerPairFilter {
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::ObjectLayer other) const override {
        return layer != Terrain || other != Terrain;
    }
};

JPH::RVec3 meters(Cell p) {
    return {double(p.x) * VoxelSize, double(p.y) * VoxelSize, double(p.z) * VoxelSize};
}

Position asPosition(JPH::RVec3Arg p) { return {p.GetX(), p.GetY(), p.GetZ()}; }

JPH::RefConst<JPH::Shape> compound(const Voxels& voxels) {
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

} // namespace

struct JoltRuntime::Resources {
    Layers layers;
    BroadPhasePairs broadPhasePairs;
    ObjectPairs objectPairs;
    JPH::TempAllocatorImpl arena{32 * 1024 * 1024};
    JPH::JobSystemThreadPool jobs{JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2};
    JPH::PhysicsSystem system;
    JPH::Ref<JPH::CharacterVirtual> walker;
    JPH::RefConst<JPH::Shape> parcelCube{new JPH::BoxShape(JPH::Vec3::sReplicate(0.5f), 0.01f)};

    Resources() {
        system.Init(8192, 0, 32768, 8192, layers, broadPhasePairs, objectPairs);
        system.SetGravity(JPH::Vec3(0, -Gravity, 0));
        JPH::CharacterVirtualSettings settings;
        const auto capsule = new JPH::CapsuleShape(PlayerHeight * 0.5f - PlayerRadius, PlayerRadius);
        const auto shape = new JPH::RotatedTranslatedShape(JPH::Vec3(0, PlayerHeight * 0.5f, 0), JPH::Quat::sIdentity(), capsule);
        settings.mShape = shape;
        settings.mInnerBodyShape = shape;
        settings.mInnerBodyLayer = Moving;
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.32f);
        settings.mCharacterPadding = 0.015f;
        walker = new JPH::CharacterVirtual(&settings, JPH::RVec3(0.5, 0.025, 1.7),
                                           JPH::Quat::sIdentity(), 0, &system);
    }

    JPH::BodyID addBody(const JPH::BodyCreationSettings& settings, JPH::EActivation activation) {
        auto id = system.GetBodyInterface().CreateAndAddBody(settings, activation);
        if (id.IsInvalid()) throw std::runtime_error("Jolt body capacity exhausted");
        return id;
    }
};

JoltRuntime::JoltRuntime() {
    static InitializeJolt library;
    resources_ = std::make_unique<Resources>();
}

JoltRuntime::~JoltRuntime() = default;

JPH::BodyID JoltRuntime::addTerrain(const Voxels& voxels) {
    auto shape = compound(voxels);
    JPH::BodyCreationSettings settings(shape, meters(voxels.bounds.min), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Terrain);
    settings.mFriction = 0.83f;
    return resources_->addBody(settings, JPH::EActivation::DontActivate);
}

JPH::BodyID JoltRuntime::addDebris(const VoxelPiece& piece) {
    auto shape = compound(piece.voxels);
    JPH::BodyCreationSettings settings(shape, meters(piece.voxels.bounds.min), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Moving);
    settings.mFriction = piece.friction;
    settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    settings.mMassPropertiesOverride.mMass = piece.mass;
    settings.mMaxLinearVelocity = 45.0f;
    return resources_->addBody(settings, JPH::EActivation::Activate);
}

void JoltRuntime::removeBody(JPH::BodyID id) {
    auto& bodies = resources_->system.GetBodyInterface();
    bodies.RemoveBody(id);
    bodies.DestroyBody(id);
}

void JoltRuntime::wakeDebris(Bounds area) {
    const JPH::AABox bounds(meters(area.min), meters(area.max));
    resources_->system.GetBodyInterface().ActivateBodiesInAABox(bounds, {}, DebrisOnly{});
}

Pose JoltRuntime::pose(JPH::BodyID id) const {
    JPH::RVec3 position;
    JPH::Quat rotation;
    resources_->system.GetBodyInterface().GetPositionAndRotation(id, position, rotation);
    return {asPosition(position), {rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW()}};
}

Position JoltRuntime::eye() const {
    return asPosition(resources_->walker->GetPosition() + JPH::RVec3(0, PlayerEyeHeight, 0));
}

void JoltRuntime::movePlayer(float dt, float x, float z, bool jump, bool sprint) {
    auto& simulation = *resources_;
    auto& walker = *simulation.walker;
    const auto gravity = simulation.system.GetGravity();
    const auto previous = walker.GetLinearVelocity();
    const auto floor = walker.GetGroundVelocity();
    const bool grounded = walker.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround
        && previous.GetY() <= floor.GetY() + 0.1f;
    const float speed = sprint ? 7.2f : 4.6f;
    JPH::Vec3 velocity = grounded ? floor : JPH::Vec3(0, previous.GetY(), 0);
    if (grounded && jump) velocity += JPH::Vec3(0, 5.0f, 0);
    walker.SetLinearVelocity(velocity + JPH::Vec3(x * speed, 0, z * speed) + gravity * dt);
    JPH::CharacterVirtual::ExtendedUpdateSettings settings;
    settings.mWalkStairsStepUp = JPH::Vec3(0, 0.27f, 0);
    settings.mStickToFloorStepDown = JPH::Vec3(0, -0.28f, 0);
    walker.ExtendedUpdate(dt, gravity, settings,
        simulation.system.GetDefaultBroadPhaseLayerFilter(Moving), simulation.system.GetDefaultLayerFilter(Moving),
        JPH::IgnoreSingleBodyFilter(walker.GetInnerBodyID()), {}, simulation.arena);
    simulation.system.Update(dt, 1, &simulation.arena, &simulation.jobs);
}

std::optional<RayHit> JoltRuntime::aim(Position direction) const {
    auto& physics = resources_->system;
    const Position from = eye();
    const JPH::Vec3 vector{float(direction[0]), float(direction[1]), float(direction[2])};
    if (vector.IsNearZero()) return std::nullopt;
    const JPH::RRayCast ray(JPH::RVec3(from[0], from[1], from[2]), vector.Normalized() * Reach);
    JPH::RayCastResult hit;
    if (!physics.GetNarrowPhaseQuery().CastRay(ray, hit, {}, {},
            JPH::IgnoreSingleBodyFilter(resources_->walker->GetInnerBodyID()))) return std::nullopt;
    const JPH::BodyLockRead lock(physics.GetBodyLockInterface(), hit.mBodyID);
    if (!lock.Succeeded()) return std::nullopt;
    const auto& body = lock.GetBody();
    const auto contact = ray.GetPointOnRay(hit.mFraction);
    const auto normal = body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, contact);
    const auto inside = contact - JPH::RVec3(normal) * 0.001;
    const Cell voxel{int(std::floor(inside.GetX() / VoxelSize)), int(std::floor(inside.GetY() / VoxelSize)),
                     int(std::floor(inside.GetZ() / VoxelSize))};
    const JPH::RVec3 outside = contact + JPH::RVec3(normal) * 0.001;
    const Cell adjacent = cellAt(asPosition(outside));
    return RayHit{voxel, body.GetObjectLayer() == Terrain, adjacent - voxel};
}

void JoltRuntime::returnPlayer(Position feet) {
    resources_->walker->SetPosition(JPH::RVec3(feet[0], feet[1], feet[2]));
    resources_->walker->SetLinearVelocity(JPH::Vec3::sZero());
}

std::pair<double, double> JoltRuntime::verticalExtent(JPH::BodyID id) const {
    JPH::BodyLockRead lock(resources_->system.GetBodyLockInterface(), id);
    const auto& box = lock.GetBody().GetWorldSpaceBounds();
    return {box.mMin.GetY(), box.mMax.GetY()};
}

std::optional<MovingContact> JoltRuntime::sweepMoving(Position from, Position to, double half) const {
    const JPH::Vec3 delta(float(to[0] - from[0]), float(to[1] - from[1]), float(to[2] - from[2]));
    if (delta.LengthSq() < 1e-12f) return std::nullopt;
    const JPH::RVec3 start(from[0], from[1], from[2]);
    const auto cast = JPH::RShapeCast(resources_->parcelCube, JPH::Vec3::sReplicate(float(2 * half)),
                                    JPH::RMat44::sTranslation(start), delta);
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> hits;
    resources_->system.GetNarrowPhaseQuery().CastShape(cast, {}, start, hits, {}, DebrisOnly{});
    if (!hits.HadHit()) return std::nullopt;
    const auto& hit = hits.mHit;
    const auto normal = (-hit.mPenetrationAxis).NormalizedOr(JPH::Vec3::sAxisY());
    if (delta.Dot(normal) >= 0) return std::nullopt;
    const auto velocity = hit.mBodyID2 == resources_->walker->GetInnerBodyID()
        ? resources_->walker->GetLinearVelocity()
        : resources_->system.GetBodyInterface().GetPointVelocity(hit.mBodyID2, cast.GetPointOnRay(hit.mFraction));
    return MovingContact{hit.mFraction, {normal.GetX(), normal.GetY(), normal.GetZ()},
                         {velocity.GetX(), velocity.GetY(), velocity.GetZ()}};
}

bool JoltRuntime::overlapsMovingVoxel(Cell point) const {
    const JPH::RVec3 center = meters(point) + JPH::RVec3::sReplicate(VoxelSize * 0.5);
    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> hits;
    resources_->system.GetNarrowPhaseQuery().CollideShape(resources_->parcelCube,
        JPH::Vec3::sReplicate(VoxelSize * 0.98f), JPH::RMat44::sTranslation(center), {}, center, hits, {}, DebrisOnly{});
    return hits.HadHit();
}

} // namespace derivean

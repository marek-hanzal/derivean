#include "physics/service/JoltResources.hpp"

#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

#include <cmath>

namespace derivean {
namespace {

constexpr float Reach = 6.0f;

} // namespace

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
    return RayHit{voxel, body.GetObjectLayer() == TerrainLayer, adjacent - voxel};
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

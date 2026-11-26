#pragma once

// Private to the JoltRuntime translation units: the Jolt objects one runtime owns.

#include "physics/service/JoltRuntime.hpp"
#include "physics/constant/Gravity.hpp"

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <stdexcept>
#include <unordered_map>

namespace derivean {

inline constexpr JPH::ObjectLayer TerrainLayer = 0;
inline constexpr JPH::ObjectLayer MovingLayer = 1;

struct Layers final : JPH::BroadPhaseLayerInterface {
    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return JPH::BroadPhaseLayer(JPH::uint8(layer));
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        return layer.GetValue() == TerrainLayer ? "Terrain" : "Moving";
    }
#endif
};

struct BroadPhasePairs final : JPH::ObjectVsBroadPhaseLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer other) const override {
        return layer != TerrainLayer || other.GetValue() != TerrainLayer;
    }
};

struct DebrisOnly final : JPH::ObjectLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer layer) const override { return layer == MovingLayer; }
};

struct ObjectPairs final : JPH::ObjectLayerPairFilter {
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::ObjectLayer other) const override {
        return layer != TerrainLayer || other != TerrainLayer;
    }
};

inline JPH::RVec3 meters(Cell p) {
    return {double(p.x) * VoxelSize, double(p.y) * VoxelSize, double(p.z) * VoxelSize};
}

inline Position asPosition(JPH::RVec3Arg p) { return {p.GetX(), p.GetY(), p.GetZ()}; }

struct JoltRuntime::Resources {
    Layers layers;
    BroadPhasePairs broadPhasePairs;
    ObjectPairs objectPairs;
    JPH::TempAllocatorImpl arena{32 * 1024 * 1024};
    JPH::JobSystemThreadPool jobs{JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 2};
    JPH::PhysicsSystem system;
    JPH::Ref<JPH::CharacterVirtual> walker;
    JPH::RefConst<JPH::Shape> parcelCube{new JPH::BoxShape(JPH::Vec3::sReplicate(0.5f), 0.01f)};
    std::unordered_map<JPH::uint32, std::pair<JPH::Vec3, JPH::Vec3>> halted;   // motion of suspended bodies, by body id

    Resources() {
        system.Init(8192, 0, 32768, 8192, layers, broadPhasePairs, objectPairs);
        system.SetGravity(JPH::Vec3(0, -Gravity, 0));
        JPH::CharacterVirtualSettings settings;
        const auto capsule = new JPH::CapsuleShape(PlayerHeight * 0.5f - PlayerRadius, PlayerRadius);
        const auto shape = new JPH::RotatedTranslatedShape(JPH::Vec3(0, PlayerHeight * 0.5f, 0), JPH::Quat::sIdentity(), capsule);
        settings.mShape = shape;
        settings.mInnerBodyShape = shape;
        settings.mInnerBodyLayer = MovingLayer;
        settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -0.32f);
        settings.mCharacterPadding = 0.015f;
        walker = new JPH::CharacterVirtual(&settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, &system);
    }

    JPH::BodyID addBody(const JPH::BodyCreationSettings& settings, JPH::EActivation activation) {
        auto id = system.GetBodyInterface().CreateAndAddBody(settings, activation);
        if (id.IsInvalid()) throw std::runtime_error("Jolt body capacity exhausted");
        return id;
    }
};

} // namespace derivean

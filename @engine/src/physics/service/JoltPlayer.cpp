#include "physics/service/JoltResources.hpp"

#include <algorithm>

namespace derivean {
namespace {

constexpr float JumpSpeed = 5.0f;         // m/s
constexpr float SwimStroke = 3.0f;        // m/s a jump gives while afloat
constexpr float WaterDamping = 3.0f;      // 1/s, vertical

} // namespace

Position JoltRuntime::eye() const {
    return asPosition(resources_->walker->GetPosition() + JPH::RVec3(0, PlayerEyeHeight, 0));
}

Position JoltRuntime::feet() const { return asPosition(resources_->walker->GetPosition()); }

Position JoltRuntime::velocity() const {
    const auto velocity = resources_->walker->GetLinearVelocity();
    return {velocity.GetX(), velocity.GetY(), velocity.GetZ()};
}

void JoltRuntime::returnPlayer(Position feet) {
    resources_->walker->SetPosition(JPH::RVec3(feet[0], feet[1], feet[2]));
    resources_->walker->SetLinearVelocity(JPH::Vec3::sZero());
}

void JoltRuntime::movePlayer(float dt, const PlayerMove& move) {
    auto& simulation = *resources_;
    auto& walker = *simulation.walker;
    const auto gravity = simulation.system.GetGravity();
    const JPH::Vec3 asked(move.velocity[0], move.velocity[1], move.velocity[2]);
    const auto broadPhase = simulation.system.GetDefaultBroadPhaseLayerFilter(MovingLayer);
    const auto layers = simulation.system.GetDefaultLayerFilter(MovingLayer);
    const JPH::IgnoreSingleBodyFilter self(walker.GetInnerBodyID());

    if (move.mode == PlayerMove::Mode::Hold) {
        walker.SetLinearVelocity(JPH::Vec3::sZero());
    } else if (move.mode == PlayerMove::Mode::Fly) {
        walker.SetLinearVelocity(asked);
        walker.Update(dt, JPH::Vec3::sZero(), broadPhase, layers, self, {}, simulation.arena);
    } else {
        const auto previous = walker.GetLinearVelocity();
        const auto floor = walker.GetGroundVelocity();
        const bool grounded = walker.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround
            && previous.GetY() <= floor.GetY() + 0.1f;
        JPH::Vec3 velocity = grounded ? floor : JPH::Vec3(0, previous.GetY(), 0);
        if (move.lift > 0) {
            velocity.SetY(velocity.GetY() * std::max(0.0f, 1 - WaterDamping * move.lift * dt));
            if (move.jump && move.lift >= 1) velocity.SetY(std::max(velocity.GetY(), SwimStroke));
        }
        if (grounded && move.jump) velocity += JPH::Vec3(0, JumpSpeed, 0);
        walker.SetLinearVelocity(velocity + JPH::Vec3(asked.GetX(), 0, asked.GetZ()) + gravity * (dt * (1 - move.lift)));
        JPH::CharacterVirtual::ExtendedUpdateSettings settings;
        settings.mWalkStairsStepUp = JPH::Vec3(0, 0.27f, 0);
        settings.mStickToFloorStepDown = JPH::Vec3(0, -0.28f, 0);
        walker.ExtendedUpdate(dt, gravity, settings, broadPhase, layers, self, {}, simulation.arena);
    }
    simulation.system.Update(dt, 1, &simulation.arena, &simulation.jobs);
}

} // namespace derivean

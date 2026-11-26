#include "groundwater/fn/Pavement.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "groundwater/fn/Conductivity.hpp"

namespace derivean::sim {
namespace {

constexpr float BareCover = 0.15f;   // m of loose cover under which the rock still shows as pavement
constexpr uint64_t GrikeStream = 0x4752494b;

} // namespace

Pavement pavementOf(Material rock, float cover, float runoff) {
    if (!fissuredBySolution(definitionOf(rock)) || cover >= BareCover || runoff <= 0) return {};
    const float bare = 1.0f - cover / BareCover;
    Pavement pavement;
    pavement.halfWidth = std::clamp(0.06f + 0.18f * runoff, 0.05f, 0.3f) * bare;
    pavement.depth = std::clamp(9.0f * pavement.halfWidth, 0.5f, 3.0f) * bare;
    return pavement;
}

float grikeDepth(uint64_t seed, const Pavement& pavement, const RockMass& mass, Vec2 material, float depth) {
    if (pavement.halfWidth <= 0) return 0;
    const uint64_t vary = hash(mass.domain, GrikeStream);
    float deepest = 0;
    for (int set = 0; set < JointSets; ++set) {
        const JointSet& joints = mass.sets[set];
        const float distance = jointDistance(seed, mass, set, 0, material, depth);
        if (distance > 2.0f * pavement.halfWidth) continue;
        // A grike is wider along some of its length and deeper along some than along the rest.
        const double c = std::cos(joints.strike), s = std::sin(joints.strike);
        const double along = (material.x * c + material.z * s) / (2.5 * joints.spacing), row = std::floor((-material.x * s + material.z * c) / joints.spacing + 0.5);
        const float wide = float(0.5 + 0.5 * gradientNoise(vary + uint64_t(set), along, row * 0.37));
        const float deep = float(0.5 + 0.5 * gradientNoise(vary + 7 + uint64_t(set), along * 0.7, row * 0.53));
        const float half = pavement.halfWidth * (0.4f + 1.2f * wide);
        if (distance >= half) continue;
        // Walls stand nearly straight; the floor is rounded.
        const float across = distance / half;
        deepest = std::max(deepest, pavement.depth * (0.5f + 0.8f * deep) * std::sqrt(1.0f - across * across * across * across));
    }
    return deepest;
}

float grikeShare(const Pavement& pavement, const RockMass& mass) {
    float share = 0;
    for (int set = 0; set < JointSets; ++set) share += 2.0f * pavement.halfWidth * mass.sets[set].persistence / mass.sets[set].spacing;
    return std::min(1.0f, share);
}

} // namespace derivean::sim

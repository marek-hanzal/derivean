#include "horizon/type/Tint.hpp"

namespace derivean {
namespace {

constexpr float SoilDepth = 0.4f;    // m of the fine cover that is living soil
constexpr float ThinFines = 0.08f;   // m of fines under which they are a film, not a soil
constexpr float Drowned = 0.01f;     // m of water over ground that carries no soil

} // namespace

sim::Material surfaceMaterial(const sim::SurfaceSample& sample, float depth) {
    if (depth < sample.deposit) return sample.depositRock;
    const float coarse = sample.sediment[sim::Coarse], sand = sample.sediment[sim::Sand], fines = sample.sediment[sim::Fines];
    const float aboveRock = coarse + sand + fines - (depth - sample.deposit);
    if (aboveRock < 0) return sample.bedrock;
    if (aboveRock < coarse) return sample.coarseAs;
    if (aboveRock < coarse + sand) return sim::Material::Sand;
    const bool bare = fines < ThinFines || sample.water > sample.ground + Drowned;
    return depth - sample.deposit < SoilDepth && !bare ? sim::Material::Soil : sim::Material::Silt;
}

} // namespace derivean

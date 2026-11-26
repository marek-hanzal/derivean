#include "groundwater/fn/Recharge.hpp"

#include "groundwater/fn/Conductivity.hpp"

namespace derivean::sim {
namespace {

constexpr Material CoverRock[SedimentClasses] = {Material::Gravel, Material::Sand, Material::Silt};
constexpr float BareCatch = 0.35f;      // share of the rain on bare jointed rock that finds a joint before it has run off
constexpr float SolutionGain = 30.0f;   // how much wider solution has opened the joints of soluble rock at the surface
constexpr float OpenedDepth = 2.0f;     // m of bare rock whose pores and joints a storm can fill

} // namespace

float infiltrationShare(const Soak& soak) {
    if (soak.rain <= 1e-4f || soak.ice > 1.0f) return 0;
    const float intensity = soak.rain / (WetShare * PerYear);
    const MaterialDefinition& rock = definitionOf(soak.rock);
    const bool soluble = solubleRock(rock);
    const float joints = jointConductivity(rock, 0, 1) * (soluble ? SolutionGain : 1.0f);

    // The topmost layer thick enough to count decides how fast water gets in.
    float entry = -1, store = 0;
    for (int c = SedimentClasses - 1; c >= 0; --c) {
        const MaterialDefinition& layer = definitionOf(CoverRock[c]);
        store += layer.porosity * soak.cover[c];
        if (entry < 0 && soak.cover[c] >= ThinLayer) entry = layer.conductivity / (layer.conductivity + intensity);
    }
    if (entry < 0)
        entry = rock.conductivity / (rock.conductivity + intensity) +
                (soluble ? 0.9f : BareCatch) * (1.0f - rock.conductivity / (rock.conductivity + intensity)) * joints / (joints + intensity);
    // What gets in must find room until the storm is over: in the cover, or in rock that takes it on.
    store += rock.porosity * OpenedDepth + (rock.conductivity + joints) * StormLength;
    const float held = std::clamp(store / (StormDepth * soak.rain), 0.0f, 1.0f);
    const float steep = 1.0f / (1.0f + (soak.slope / SteepShed) * (soak.slope / SteepShed));
    const float thawed = float(smoothstep(-3.0, 1.0, soak.warmth));
    return std::min(MostRecharge, entry * held * steep * thawed);
}

} // namespace derivean::sim

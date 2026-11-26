#pragma once

#include "groundwater/constant/Hydraulics.hpp"
#include "material/constant/Materials.hpp"

namespace derivean::sim {

inline bool solubleRock(const MaterialDefinition& rock) { return rock.solubility >= SolubleEnough; }

// Carbonate rock: soluble, but slowly enough that water widens its joints into a lasting net of
// fissures. Salt goes so fast that it is either whole and tight or gone.
inline bool fissuredBySolution(const MaterialDefinition& rock) { return solubleRock(rock) && rock.solubility < 500.0f; }

// m/s along the joints of a rock. `depth` is metres below the rock surface and `open` the share of
// the rock that lies in the zone weather has opened. Rock that weathers to clay, or that is too
// weak to hold a crack open, seals its joints: that is what makes shale, mudstone and salt hold
// water up. In carbonate rock the water itself keeps them open and widens them.
inline float jointConductivity(const MaterialDefinition& rock, float depth, float open) {
    if (rock.jointSpacing <= 0) return 0;
    const float fine = rock.weathersTo.clay + rock.weathersTo.silt;
    float brittle = float(smoothstep(30.0, 70.0, rock.compressiveStrength) * (1.0 - smoothstep(0.4, 0.7, fine)));
    if (fissuredBySolution(rock)) brittle = std::max(brittle, 0.6f) * SolutionGain;
    const float close = std::clamp(JointReference / rock.jointSpacing, 0.3f, 4.0f);
    return JointConductivity * brittle * close * (1.0f + (OpenJointGain - 1.0f) * open) * std::exp(-depth / JointDecay);
}

// What the three classes of loose cover are to water: coarse, sand, fines.
inline constexpr Material CoverRock[3] = {Material::Gravel, Material::Sand, Material::Silt};

// Share of a bed's matrix conductivity that is left across its bedding.
inline float acrossBedding(const MaterialDefinition& rock) { return 1.0f - 0.9f * std::clamp(rock.bedding, 0.0f, 1.0f); }

} // namespace derivean::sim

#pragma once

#include "groundwater/constant/Hydraulics.hpp"
#include "material/constant/Materials.hpp"

namespace derivean::sim {

// m/s along the joints of a rock. `depth` is metres below the rock surface and `open` the share of
// the rock that lies in the zone weather has opened. Rock that weathers to clay, or that is too
// weak to hold a crack open, seals its joints: that is what makes shale, mudstone and salt hold water up.
inline float jointConductivity(const MaterialDefinition& rock, float depth, float open) {
    if (rock.jointSpacing <= 0) return 0;
    const float fine = rock.weathersTo.clay + rock.weathersTo.silt;
    const float brittle = float(smoothstep(30.0, 70.0, rock.compressiveStrength) * (1.0 - smoothstep(0.4, 0.7, fine)));
    const float close = std::clamp(JointReference / rock.jointSpacing, 0.3f, 4.0f);
    return JointConductivity * brittle * close * (1.0f + (OpenJointGain - 1.0f) * open) * std::exp(-depth / JointDecay);
}

inline bool solubleRock(const MaterialDefinition& rock) { return rock.solubility >= SolubleEnough; }

// Share of a bed's matrix conductivity that is left across its bedding.
inline float acrossBedding(const MaterialDefinition& rock) { return 1.0f - 0.9f * std::clamp(rock.bedding, 0.0f, 1.0f); }

} // namespace derivean::sim

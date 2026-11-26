#pragma once

#include "groundwater/type/SpillStop.hpp"
#include "simulation/type/InfluenceGraph.hpp"

#include <array>

namespace derivean::sim {

enum WaterStage : std::size_t {
    TerrainAtStart, KarstPreparation, TerrainBeforeFlood, Flood, Routing, Overflow,
    Dissolution, Erosion, WaterStageCount
};

inline constexpr auto WaterStages = [] {
    std::array<InfluenceStage, WaterStageCount> stages{};
    stages[TerrainAtStart] = {"Terrain at start of step", 0};
    stages[KarstPreparation] = {"Karst preparation and return", 1};
    stages[TerrainBeforeFlood] = {"Terrain after uplift and slope work", 2};
    stages[Flood] = {"Flood levels", 3};
    stages[Routing] = {"Surface routing", 4, &HollowSpillStop};
    stages[Overflow] = {"Hollow overflow", 4, &HollowSpillStop};
    stages[Dissolution] = {"Karst dissolution", 5};
    stages[Erosion] = {"River erosion and deposition", 6};
    return stages;
}();
// Feedback through terrain and soluble rock in the landscape replay.
inline constexpr std::array WaterLinks{
    InfluenceLink{TerrainAtStart, KarstPreparation, InfluenceTiming::SameStep, "surface and rock"},
    InfluenceLink{TerrainAtStart, TerrainBeforeFlood, InfluenceTiming::SameStep, "terrain"},
    InfluenceLink{TerrainBeforeFlood, Flood, InfluenceTiming::SameStep, "surface"},
    InfluenceLink{KarstPreparation, Flood, InfluenceTiming::SameStep, "draining hollows"},
    InfluenceLink{Flood, KarstPreparation, InfluenceTiming::NextStep, "lake levels and spills"},
    InfluenceLink{Flood, Routing, InfluenceTiming::SameStep, "flood levels and order"},
    InfluenceLink{KarstPreparation, Routing, InfluenceTiming::SameStep, "risen water and sink capacity"},
    InfluenceLink{Routing, KarstPreparation, InfluenceTiming::NextStep, "sunk water and prior discharge"},
    InfluenceLink{Routing, Overflow, InfluenceTiming::SameStep, "ponded water and route shares"},
    InfluenceLink{Overflow, Routing, InfluenceTiming::SameStep, "adds discharge and sinking along fixed shares"},
    InfluenceLink{Routing, Dissolution, InfluenceTiming::SameStep, "sunk water"},
    InfluenceLink{Routing, Erosion, InfluenceTiming::SameStep, "discharge and routes"},
    InfluenceLink{Dissolution, Erosion, InfluenceTiming::SameStep, "ground after dissolution"},
    InfluenceLink{Dissolution, TerrainAtStart, InfluenceTiming::NextStep, "dissolved rock"},
    InfluenceLink{Erosion, TerrainAtStart, InfluenceTiming::NextStep, "updated ground"},
};
static_assert(validInfluences(WaterStages, WaterLinks));

} // namespace derivean::sim

#pragma once

#include "chronology/type/Calendar.hpp"

#include <cmath>

namespace derivean::sim {

// Thresholds the stages of the ground level share.
inline constexpr float BedFlow = 1.0e5f;        // m³/yr above which water keeps a bed of its own
inline constexpr float StepFlow = 3.0e5f;       // m³/yr above which a steep bed organises into steps
inline constexpr float RunningFlow = 4.0e5f;    // m³/yr above which a stream is wider than a ground cell and shows as water
inline constexpr float PoolSupply = 3.0e3f;     // m³/yr a hollow must receive to hold water through the year
inline constexpr float FailingFace = 1.0e-5f;   // m/yr of rock breaking off that makes a cell a source of scree
inline constexpr float LeastIce = 5.0f;         // m of ice at the glacial maximum that leaves till
inline constexpr float FrostWork = 0.10f;       // share of the year in the cracking window above which frost shapes the ground

// m: depth of a stream of `flow` m³/yr by the usual hydraulic geometry.
inline float streamDepth(float flow) { return 0.3f * std::pow(flow / float(YearSeconds), 0.36f); }

// m: width of its channel.
inline float streamWidth(float flow) { return 4.5f * std::sqrt(flow / float(YearSeconds)); }

// m: the largest stone it moves on a bed of the given fall (critical Shields stress 0.045, quartz density).
inline float competence(float flow, float fall) { return 13.5f * streamDepth(flow) * fall; }

// m: length of the channel above a point that carries this much water (Hack's law, 0.4 m/yr of runoff).
inline float reachOf(float flow) { return 1400.0f * std::pow(flow / 0.4e6f, 0.6f); }

} // namespace derivean::sim

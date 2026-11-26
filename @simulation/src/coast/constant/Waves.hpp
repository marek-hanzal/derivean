#pragma once

#include "chronology/type/Calendar.hpp"
#include "landscape/type/Level.hpp"

namespace derivean::sim {

// Waves are kept as eight trains, one travelling towards each neighbour of a cell (NeighborX/Z).
// A train's state is the square of its significant height in m², which is its energy up to a constant.
inline constexpr int WaveDirections = 8;

// The wind of a place blows from every quarter some of the time, and hardest from the prevailing
// one: a storm there is StormGain times the annual mean, and every quarter sees StormCalm of mean wind.
inline constexpr float StormGain = 2.5f;
inline constexpr float StormCalm = 2.0f;       // m/s

// Fetch-limited growth in deep water (Sverdrup–Munk–Bretschneider): g·H/U² = 0.0016·(g·F/U²)^½,
// so H² grows by FetchGrowth·U² for every metre of open water, up to the fully developed sea
// H = FullSea·U².
inline constexpr float FetchGrowth = 2.61e-7f;
inline constexpr float FullSea = 0.0248f;
inline constexpr float Breaker = 0.5f;         // significant height a depth of one metre lets pass
// A wave loses height over a rough shallow bed as dH/dx = −BedFriction·H²/h² (friction factor 0.1):
// it is what lets a shore platform or a shoal shelter what lies behind it.
inline constexpr float BedFriction = 0.0106f;
inline constexpr float WaveSpread = 0.2f;      // share of a train that arrives from each side of its heading

// Half the range the waterline moves through. The sea has the spring tide of the calendar; a lake
// has its seasonal levels and seiches.
inline constexpr float SeaSwing = float(SpringTide);
inline constexpr float LakeSwing = 0.25f;
inline constexpr float SeaBand = 0.5f;         // m above the sea level up to which standing water is the sea
// Height over deep-water wavelength of a wind sea (Hs/L0 ≈ 0.04, Tp ≈ 4·√Hs): it sets the period,
// the period how deep the bed lies under the wave and how far its swash runs up a beach.
inline constexpr float WindSeaSteepness = 0.04f;
inline constexpr float HighestBerm = 3.0f;     // m above high water

// Rock withstands waves lower than Hc = Withstand·(compressive strength in MPa, as jointed and
// crushed at the cell): the force of a breaking wave, ρgH, against the strength it must overcome
// (Sunamura 1977), so the threshold is linear in the strength: ρgHc/strength ≈ 2e-4. Above it each
// train of height H cuts the foot back by Quarry·(H − Hc)/Hc² metres a year, and a face goes back
// by the mean over the WaveDirections trains. One train of a metre or two takes chalk at
// decimetres a year, sandstone at millimetres, and sound granite only above three metres.
inline constexpr float Withstand = 0.02f;
inline constexpr float Quarry = 0.02f;
inline constexpr float CrushedStrength = 0.15f;   // share of its strength rock keeps where a cell is all joint corridor
// yr the sea has stood near its present level: the time waves have had to cut a shore of the
// finer levels, whose settling is not a calendar. Every step of a region holds a high stand as long.
inline constexpr double StandTime = 6000.0;
// m below still water to which a face is cut. A Region step spans several glacial cycles: what its
// sea leaves is the ramp planed while the level swung, not a bench at one level.
inline constexpr float BenchDepth[LevelCount] = {0, 2.0f, 0, 0, 0};
inline constexpr float NotchBench = 10.0f;     // m of cut from which the waves break on the bench before they reach the face

// Wave heights are kept per tile in steps of WaveQuantum: up to 12.75 m.
inline constexpr float WaveQuantum = 0.05f;

} // namespace derivean::sim

#pragma once

namespace derivean::sim {

// Wind sorts the finer half out of the Sand class; that is the grain dunes are made of.
inline constexpr double DuneGrain = 2.5e-4;         // m
inline constexpr double GrainDensity = 2650.0;      // kg/m³, quartz
inline constexpr double SandBulkDensity = 1600.0;   // kg/m³ of a dune's sand as it lies
inline constexpr double NearGroundAir = 1.2;        // kg/m³

// The model's low-band wind is taken as the wind ten metres above a rippled sand bed.
inline constexpr double WindHeight = 10.0;          // m
inline constexpr double SandRoughness = 3e-4;        // m

// The lee of a crest is calm below a line that falls at this slope from it: sand that lands there stays.
inline constexpr float ShadowTan = 0.194f;          // 11°
// A step steeper than this stops a hopping grain at its foot: sand ramps up a cliff, it does not jump it.
inline constexpr float ClimbTan = 1.0f;

// Stones left behind once the sand and dust between them are gone lie one stone thick; this much of
// the Coarse class at the surface closes it against the wind.
inline constexpr float LagArmour = 0.06f;           // m

// A cover wetter than its climate can dry between rains does not blow. With no plants the surface
// of loose sand dries within days, so even a humid climate leaves the wind part of the year.
inline constexpr double DampGain = 3.0;             // how fast the dry share of the year falls with rain over evaporation
// Sand this close above standing or running water is held damp by it.
inline constexpr float CapillaryRise = 0.5f;        // m

// A stream of dry country runs a few days a year; one that gathers more than PerennialFlow comes
// from wetter ground and runs most of the year wherever it is.
inline constexpr float FloodShare = 0.02f;       // share of the year a stream of the driest country runs
inline constexpr float PerennialFlow = 1e9f;     // m³/yr of a stream that runs half the year in any climate

// Dust is two sizes: coarse silt that comes down within tens of kilometres and makes loess, and
// fine dust that stays up for hundreds. Rain washes both out sooner.
inline constexpr int DustSizes = 2;
inline constexpr float DustShare[DustSizes] = {0.6f, 0.4f};
inline constexpr float DustFall[DustSizes] = {60e3f, 600e3f};   // m of travel over which all but 1/e has settled in dry air
inline constexpr float DustWashRain = 0.5f;                     // m/yr of rain that doubles the fall-out
// Fines raised per sand moved: the Sahara gives up some 0.5–1 Gt a year from 9 million km², 0.04–0.07
// mm of loess. The far dust and the haze of the weather both raise dust at this ratio to the drift.
inline constexpr float DustRise = 5e-5f;                        // m/yr of fines a hyper-arid floor gives up under DustDrift
inline constexpr float DustDrift = 30.0f;                       // m²/yr of drift potential, that of a trade-wind desert

// m/yr: the fastest the floor of a continental playa rises when averaged over its wet and dry
// phases. Death Valley's core DV93-1 holds some 186 m of halite and mud laid over about 200 kyr
// (Ku et al. 1998; Lowenstein et al. 1999), near 1 mm/yr in one of the fastest-sinking closed
// basins on Earth; the Bonneville Salt Flats crust, about 1 m since the lake dried some 13 kyr
// ago, is ten times slower.
inline constexpr double EvaporiteRate = 1e-3;

} // namespace derivean::sim

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

} // namespace derivean::sim

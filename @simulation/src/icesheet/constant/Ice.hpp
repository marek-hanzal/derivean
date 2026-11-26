#pragma once

#include "planet/constant/Scale.hpp"

namespace derivean::sim {

// Ice as a material that flows. Lengths in metres, time in years, stress in pascals.
inline constexpr double IceDensity = 917.0;                          // kg/m³
inline constexpr double IceWeight = IceDensity * 9.81;               // Pa of pressure per metre of ice
inline constexpr double IceToWater = IceDensity / 1000.0;            // m of water one metre of ice melts to; share of its thickness a floating slab sinks
inline constexpr double IceToMantle = IceDensity / MantleDensity;    // m the crust sinks under one metre of ice once it has settled

// Glen's law with exponent 3: the depth-averaged creep of a slab is CreepFlow · H⁴ · slope³.
inline constexpr double CreepRate = 5e-17;                           // 1/(Pa³·yr), ice a few degrees below melting
inline constexpr double CreepFlow = 2.0 * CreepRate * IceWeight * IceWeight * IceWeight / 5.0;
// Sliding over a thawed bed grows with the square of the basal shear stress: 50 m/yr at one bar.
inline constexpr double SlideRate = 5e-9;                            // m/yr per Pa²
inline constexpr double SlideFlow = SlideRate * IceWeight * IceWeight;

// A front standing in water breaks off faster the deeper the water: it retreats by this many
// water depths a year. With it a front holds only where the ice arriving outruns the loss, in
// water of some tens to two hundred metres; that depth is the sill a fjord keeps at its mouth.
inline constexpr double CalvingRate = 27.0;                          // 1/yr

// Colder air carries less snow: precipitation falls by this share per °C the ice surface is colder
// than the ground the climate was solved for. Without it a dome feeds itself without end.
inline constexpr double ThinAir = 0.06;                              // 1/°C

// Heat at the base. Ice conducts worse than rock, so the gradient steepens entering the ice; snow
// carried down from the surface cools the column (Robin's steady profile).
inline constexpr double IceDiffusivity = 34.4;                       // m²/yr
inline constexpr double IceConductivity = 2.1;                       // W/(m·K)
inline constexpr double SecondsPerYear = 3.156e7;
inline constexpr double BedToIce = 1.3;                              // temperature gradient in basal ice per gradient in the rock under it
inline constexpr double MeltingFall = 8.7e-4;                        // °C the melting point drops per metre of ice above
inline constexpr double ThawBand = 2.0;                              // °C below melting over which a bed goes from frozen to thawed

inline constexpr float LeastSheet = 20.0f;                           // m of ice below which a cell counts as bare

} // namespace derivean::sim

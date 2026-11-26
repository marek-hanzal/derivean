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

// Ice yields before its bed carries more than about a bar and a half: where the thin-slab stress
// of a steep margin says more, the slope is the grid's, not the ice's.
inline constexpr float YieldStress = 1.5e5f;        // Pa

// What sliding ice does to its bed.
inline constexpr float FastestSlide = 1500.0f;      // m/yr; faster ice rides on water and no longer grips its bed
// m of granite ground off per metre the ice slides over it. Rates measured under sheets on hard
// beds are ten times higher. This one is set so that the tens of millions of years of icehouse a
// region replays take what Earth's two and a half million took, some tens to a hundred metres off
// a lowland (seed 42, a freed lowland: 1.8e-7 wore 209 m and drowned half of it; this rate about
// 115 m). A fjord is still cut below its sill: its ice slides several times faster.
inline constexpr float WearRate = 1.0e-7f;
inline constexpr float GraniteErodibility = 1.2e-6f;   // the stream-power coefficient WearRate is stated for
inline constexpr float LooseGain = 40.0f;           // times faster the ice takes loose cover than granite
// Where the bed rises along the flow by more than this many times the fall of the ice surface, the
// meltwater under the ice freezes on instead of flushing the debris out, and the ice stops cutting.
// It is what bounds a rock basin: metres under a flat sheet, a kilometre behind a fjord's sill.
inline constexpr float Overdeepening = 1.5f;
// One working of the bed takes at most this share of the ice above it: beyond that the sheet it
// was reckoned under is no longer the sheet that would be there, and must be solved again first.
inline constexpr float DeepestCut = 0.25f;
inline constexpr float TillReach = 15e3f;           // m the ice carries debris before all but 1/e of it is lodged
inline constexpr float MoraineLimit = 60.0f;        // m of debris a front keeps where it drops it; the rest goes on with its meltwater
// Ground rock by transport class (blocks and gravel, sand, fines): the mix landscape/fn/Glacial.cpp
// leaves at a glacier's front. The two must stay equal for sheet and valley ice to lay the same till.
inline constexpr float TillSplit[3] = {0.5f, 0.2f, 0.3f};

// Water under the ice.
inline constexpr float BasalMelt = 0.01f;           // m/yr of water a thawed bed yields
inline constexpr float EskerFlow = 2.0e8f;          // m³/yr above which a stream under the ice keeps a tunnel that fills with its bedload
inline constexpr float EskerReach = 12e3f;          // m of such a tunnel over which it takes all but 1/e of the passing gravel and sand
inline constexpr float PortalFlow = 1.0e9f;         // m³/yr at which a stream leaving the ice takes half of what the ice dropped there on with it
inline constexpr float OutwashReach[2] = {6e3f, 22e3f};   // m gravel and sand travel beyond the ice before all but 1/e has settled

// The last sheet at the finer levels (icesheet/fn/Local.hpp).
inline constexpr float LastSheetYears = 20e3f;      // yr the last maximum's ice stood at its full extent, sliding and taking up the cover it found
inline constexpr float BasinDepth = 50.0f;          // m a rock basin is dug below the ground around it at most (Local): the deepest lakes of a shield
inline constexpr float KnobRelief = 10.0f;          // m the same digging leaves between plucked joints and the knobs beside them (Site)
// Behind a rise of the bed the ice rides over a cavity of lower pressure, and till squeezed from
// the rest of the bed fills it: up to a line falling from the rise at this slope, the gentle lee
// of a drumlin or a crag's tail.
inline constexpr float ShelterSlope = 0.05f;
inline constexpr float LeastSlide = 1.0f;           // m/yr below which the bed counts as frozen: it keeps the cover it had
inline constexpr float TillRepose = 0.45f;          // steepest slope (24°) till and moraine are laid at against a neighbour; what would stand steeper goes on
// A tunnel's width grows with the water through it (a channel in ice melts its walls with the heat
// of its flow): half-width EskerSpread · (discharge / 1e8 m³/yr)^0.4, within these bounds.
inline constexpr float EskerSpread = 25.0f;         // m
inline constexpr float EskerNarrowest = 30.0f, EskerWidest = 250.0f;   // m of half-width

} // namespace derivean::sim

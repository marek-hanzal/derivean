#pragma once

#include "volcanic/type/Vent.hpp"

namespace derivean::sim {

// A layer that moves while its own weight along the slope exceeds its yield strength (a Bingham
// body): it flows where `slope × thickness > yield`, with the yield strength given as a length
// (τ / ρg). Lava stiffens as it cools on its way, which is what stops a flow and thickens its front.
// A body thicker than one flow unit moves no more easily than that unit: a volcano is built of
// many flows, not of one, so its flank stands at `yield / sheet`.
struct Rheology {
    float yieldHot{};      // m, as it leaves the vent
    float yieldCold{};     // m, once it has set
    float sheet{};         // m, thickness of a single flow unit
    float coolLength{};    // m of travel over which it sets
    // m, the yield strength a single flow shows where it came to rest, which sets its thickness
    // there (`flowThickness`). The emplacement laws above work on a level's cells, many flows to a
    // cell; this is one flow's, as measured on stopped flows.
    float stopped{};
};

// Stopped flows (Hulme 1974, table 3; his density 2000 kg/m³): basalts at Etna 1966 7e2 Pa, Oo-sima 1951
// 4.3e3, Mauna Loa 1942 8e3, 2.9e3 (0.15 m) in the geometric mean; andesites at Paricutin 1.3e4,
// Tristan da Cunha 1.4e4 and 5e4, Hekla 1947 2e4, 2.1e4 (1.05 m). A dome's is not measured there: its
// lava leaves the vent as stiff as it stops.
inline constexpr Rheology BasaltLava{0.15f, 1.2f, 8.0f, 26000.0f, 0.15f};      // long thin sheets: a flank of one degree at the vent, eight at its far edge
inline constexpr Rheology AndesiteLava{0.6f, 45.0f, 35.0f, 9000.0f, 1.05f};    // short thick tongues
inline constexpr Rheology DomeLava{12.0f, 60.0f, 80.0f, 1500.0f, 12.0f};       // degassed, nearly solid: piles over its vent
inline constexpr Rheology AshFlow{0.02f, 0.5f, 20.0f, 28000.0f};        // the current of ash and gas of a chamber emptying: runs far, ponds in valleys
inline constexpr Rheology BlockFlow{0.05f, 1.2f, 20.0f, 14000.0f};      // the smaller currents of ash and blocks of an ordinary eruption
inline constexpr float QuenchShare = 0.12f;   // share of its cooling length lava keeps once it runs under water
// Rise over run above which set lava does not stand where it stopped: quenched under water or
// stalled on a steep front, it breaks up and its rubble slides to rest.
inline constexpr float RubbleRest = 0.6f;
// m laid at once below which it stays loose (rubble, cinders and ash on the ground); above, it
// welds into rock with the cover it buries.
inline constexpr float WeldDepth = 2.0f;

inline constexpr const Rheology& lavaOf(Magma magma) { return magma == Magma::Basalt ? BasaltLava : AndesiteLava; }

// How an eruption divides what it brings up: lava, fragments that fall near the vent and build its
// cone, and ash the wind carries away.
struct Style {
    float lava{}, cone{}, flows{}, ash{};   // `flows`: hot currents of ash and blocks that run out over the foot of the cone
};

// The scoria of fluid melt builds small cones all over a shield, none at a region's scale: it is
// part of the small eruptions the local level builds (volcanic/fn/Minor.hpp).
inline constexpr Style BasaltStyle{0.97f, 0.0f, 0.0f, 0.03f};
inline constexpr Style AndesiteStyle{0.40f, 0.20f, 0.20f, 0.20f};
// The largest eruptions empty their chamber as currents of ash and a high column, with no lava at all.
inline constexpr Style ParoxysmStyle{0.0f, 0.05f, 0.60f, 0.35f};

inline constexpr const Style& styleOf(Magma magma) { return magma == Magma::Basalt ? BasaltStyle : AndesiteStyle; }

// Fall of fragments around a vent thins exponentially with distance; the cone it builds is as wide
// as a pile of that volume at rest: `ConeLength × volume^(1/3)` is the e-folding distance.
inline constexpr double ConeLength = 0.38;
inline constexpr double VentShare = 0.4;         // radius of the jet over the vent, where nothing falls, against that distance
inline constexpr double AshLength = 9000.0;      // m, e-folding distance of the ash of a cubic kilometre
inline constexpr double WindStretch = 8.0;       // m/s of wind that stretches the ash blanket to twice its length downwind

// A single eruption that takes a large share of its chamber (volcanic/fn/Chamber.cpp), and at least
// this much, empties it faster than the roof can follow: the roof goes down as one block by
// `CalderaDepth`, over as wide a ring as the volume that left requires. Smaller chambers sag
// instead (Pinatubo 1991, 5 km³ of dense rock, left a caldera 2.5 km wide; St Helens 1980, 0.25,
// none). The walls it leaves are the business of slope failure afterwards.
inline constexpr double CalderaVolume[2] = {1e9, 2e9};       // m³, by Magma
inline constexpr double CalderaDepth[2] = {500.0, 1200.0};   // m

} // namespace derivean::sim

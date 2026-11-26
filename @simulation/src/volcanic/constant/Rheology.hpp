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
};

inline constexpr Rheology BasaltLava{0.15f, 1.2f, 8.0f, 26000.0f};      // long thin sheets: a flank of one degree at the vent, eight at its far edge
inline constexpr Rheology AndesiteLava{0.6f, 45.0f, 35.0f, 9000.0f};    // short thick tongues
inline constexpr Rheology DomeLava{12.0f, 60.0f, 80.0f, 1500.0f};       // degassed, nearly solid: piles over its vent
inline constexpr Rheology AshFlow{0.02f, 0.5f, 20.0f, 28000.0f};        // the current of ash and gas of a chamber emptying: runs far, ponds in valleys
inline constexpr Rheology BlockFlow{0.05f, 1.2f, 20.0f, 14000.0f};      // the smaller currents of ash and blocks of an ordinary eruption
inline constexpr float QuenchShare = 0.12f;   // share of its cooling length lava keeps once it runs under water
// Rise over run above which set lava does not stand where it stopped: quenched under water or
// stalled on a steep front, it breaks up and its rubble slides to rest.
inline constexpr float RubbleRest = 0.6f;

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

// A single eruption that brings up more than this empties its chamber faster than the roof can
// follow: the roof goes down as one block by `CalderaDepth`, over as wide a ring as the volume that
// left requires. The walls it leaves are the business of slope failure afterwards.
inline constexpr double CalderaVolume[2] = {4e9, 12e9};      // m³, by Magma
inline constexpr double CalderaDepth[2] = {500.0, 1200.0};   // m

} // namespace derivean::sim

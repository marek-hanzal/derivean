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

inline constexpr Rheology BasaltLava{0.03f, 4.0f, 8.0f, 26000.0f};      // long thin sheets: flanks of 1 to 5 degrees
inline constexpr Rheology AndesiteLava{0.6f, 45.0f, 35.0f, 9000.0f};    // short thick tongues
inline constexpr Rheology DomeLava{12.0f, 60.0f, 80.0f, 1500.0f};       // degassed, nearly solid: piles over its vent
inline constexpr Rheology AshFlow{0.02f, 0.5f, 20.0f, 28000.0f};        // a hot current of ash and gas: runs far, ponds in valleys
inline constexpr float QuenchShare = 0.12f;   // share of its cooling length lava keeps once it runs under water

inline constexpr const Rheology& lavaOf(Magma magma) { return magma == Magma::Basalt ? BasaltLava : AndesiteLava; }

// How an eruption divides what it brings up: lava, fragments that fall near the vent and build its
// cone, and ash the wind carries away.
struct Style {
    float lava{}, cone{}, ash{};
};

inline constexpr Style BasaltStyle{0.93f, 0.05f, 0.02f};
inline constexpr Style AndesiteStyle{0.45f, 0.27f, 0.28f};
// The largest eruptions empty their chamber as currents of ash and a high column, with no lava at all.
inline constexpr Style ParoxysmStyle{0.0f, 0.05f, 0.35f};   // the remaining 0.6 runs out as ash flows

inline constexpr const Style& styleOf(Magma magma) { return magma == Magma::Basalt ? BasaltStyle : AndesiteStyle; }

// Fall of fragments around a vent thins exponentially with distance; the cone it builds is as wide
// as a pile of that volume at rest: `ConeLength × volume^(1/3)` is the e-folding distance.
inline constexpr double ConeLength = 0.38;
inline constexpr double VentShare = 0.4;         // radius of the jet over the vent, where nothing falls, against that distance
inline constexpr double AshLength = 9000.0;      // m, e-folding distance of the ash of a cubic kilometre
inline constexpr double WindStretch = 8.0;       // m/s of wind that stretches the ash blanket to twice its length downwind

// A single eruption that brings up more than this empties its chamber faster than the roof can
// follow: the summit founders. The pit is as wide as the chamber was: radius = CalderaRadius ×
// (km³)^0.4 kilometres, and as deep as the volume that left requires.
inline constexpr double CalderaVolume[2] = {40e9, 12e9};   // m³, by Magma
inline constexpr double CalderaRadius = 1000.0;            // m for one cubic kilometre

} // namespace derivean::sim

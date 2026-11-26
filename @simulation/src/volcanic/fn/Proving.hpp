#pragma once

#include "measure/fn/Plane.hpp"
#include "volcanic/type/Vent.hpp"

namespace derivean::sim {

// A plane built by hand with one vent on it: the ground on which the emplacement laws are judged
// before a world is asked for a volcano.
struct ProvingSpec {
    PlaneSpec ground;
    Magma magma{Magma::Basalt};
    Vec2 site;               // m from the middle of the domain to the vent
    double volume{};         // m³ of each episode
    int episodes{};          // region steps with an eruption, from the first on (local level: every fourth step)
    int quiet{};             // steps of weather alone after the last
    double paroxysm{};       // m³ of the largest single eruption of the last episode
};

// Proving.cpp
Vent provingVent(const ProvingSpec& spec, const Domain& domain);
// The plane and its vent replayed to the end; `workers` spreads what the replay may spread.
Domain replayProving(const ProvingSpec& spec, Workers* workers = nullptr);

// The form of what stands around a vent, read off a domain's surface along rings around it.
struct EdificeShape {
    double base{};        // m, ground at the foot
    double summit{};      // m, highest ground within the edifice
    double height{};      // m between them
    double radius{};      // m to where the flank has come down to a twentieth of the height
    double topSlope{};    // degrees, steepest ring of the upper third
    double midSlope{};    // degrees at half the height
    double footSlope{};   // degrees at a tenth of the height
    double pit{};         // m the middle lies below the highest ring around it: a crater or a caldera
    double pitRadius{};   // m to that ring
    double pile{};        // m³ of volcanic rock within `reach`
};

// Survey.cpp
EdificeShape measureEdifice(const Domain& domain, Vec2 site, double reach);

} // namespace derivean::sim

#pragma once

#include "volcanic/type/Vent.hpp"

namespace derivean::sim {

// One eruption of a vent as play meets it: the same eruption of the vent's sequence that the
// replay counts (volcanic/fn/Source.hpp), given a calendar, a rate and a column. It has two
// phases: the explosive one throws its fragments (fall, hot currents, ash) at once, the lava
// follows at the slower rate a conduit drains at.
struct Outbreak {
    uint64_t vent{};        // Vent::id
    int64_t number{};       // in the vent's sequence of eruptions
    Magma magma{Magma::Basalt};
    Vec2 site;              // m, the conduit it comes up through
    double time{};          // yr of the era it starts
    double volume{};        // m³ of rock brought up
    double fall{}, flows{}, ash{}, lava{};   // m³ of it by the way it travels (volcanic/constant/Rheology.hpp)
    bool empties{};         // it empties the chamber: the roof founders as it ends
    bool small{};           // of the size the local level builds (volcanic/fn/Minor.hpp): its lava sets near the vent
    double blast{};         // s the explosive phase lasts
    double effusion{};      // s the lava phase lasts after it
    double blastRate{};     // m³/s of rock through the conduit while it explodes
    double lavaRate{};      // m³/s while lava flows
    double column{};        // m the explosive column rises above the vent
    double plume{};         // m the plume over the flowing lava rises above the vent
};

// What an eruption leaves on the ground around its vent. Per cell of the grid it was laid on, m.
struct NearField {
    std::vector<float> fragments;   // fall around the vent and the deposits of hot currents: loose
    std::vector<float> lava;        // set lava: rock
    std::vector<float> drop;        // m the ground founders where the eruption emptied its chamber
    std::vector<float> sink;        // m the layering under it goes down: the roof's block (fn/Collapse.cpp)
    double left{};                  // m³ that left the grid or came to rest on cells it may not change
};

} // namespace derivean::sim

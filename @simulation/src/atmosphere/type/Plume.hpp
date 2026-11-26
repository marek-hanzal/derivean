#pragma once

#include "atmosphere/constant/Air.hpp"
#include "core/type/Field.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace derivean::sim {

// Grain classes of what a source throws into the air, by how fast they fall: lapilli, sand-size
// ash and fine dust.
inline constexpr int AshClasses = 3;
enum AshClass { Lapilli, Grit, Dust };   // Grit is the sand-size ash
inline constexpr std::array<double, AshClasses> AshSettling = {8.0, 1.0, 0.02};   // m/s

// Levels the lasting material is kept at: the three bands of the weather, and the air above the
// tropopause, where nothing rains it out.
inline constexpr int PlumeLevels = 4;

// A source of material in the air: a vent in eruption, or wind lifting dust off dry ground. It
// emits at constant rates between two calendar times; whoever owns it adds one source per phase of
// its activity. Only what lies after the atmosphere's current step when it is added gets emitted,
// so add a source before advancing past its start.
struct AirSource {
    uint64_t id{};                           // the owner's name for it; the inspector reports it back
    Vec2 position;                           // world metres
    double height{};                         // m above the sea at which the material spreads out
    double start{}, end{};                   // calendar seconds
    std::array<double, AshClasses> ash{};    // kg/s per grain class
    double sulphur{};                        // kg/s of sulphur dioxide
    double vapour{};                         // kg/s of water vapour
    double heat{};                           // W
};

// Lapilli and sand-size ash of one source that have landed around one place. They fall within a
// few steps and far closer to the source than a weather cell is wide, so they are not carried on
// the grid: each step's share lands where the wind at the source's height drops it.
struct AshPatch {
    uint64_t source{};
    int grain{};
    Vec2 centre;       // world metres
    double spread{};   // m, standard deviation of the blanket around the centre
    double mass{};     // kg
};

// kg in every store a source's material passes through. Emitted ash is aloft or fallen, emitted
// sulphur is gas, veil or fallen; nothing else happens to either.
struct PlumeLedger {
    double ashEmitted{}, ashAloft{}, ashFallen{};
    double sulphurEmitted{}, sulphurGas{}, sulphurVeil{}, sulphurFallen{};   // all as sulphur dioxide
    double vapourEmitted{};   // water the sources added to the air; the water ledger's total grew by this
};

struct Plume {
    std::vector<AirSource> sources;
    std::array<Field<double>, PlumeLevels> dust, gas, veil;   // kg/m²: fine ash, sulphur dioxide, and sulphate counted as the dioxide it formed from
    Field<double> fallen;       // kg/m² of ash of all classes that landed in the cell, as a cell mean
    Field<double> onSnow;       // kg/m² of it lying on top of the cell's snow: fresh snow buries it, and it goes with the pack
    bool sooted{};              // whether any cell has ash on its snow
    Field<double> dustFallen;   // kg/m²: the part of it that came down from the dust on the grid
    Field<double> fallRate;     // kg/m²/s of ash landing in the cell during the last step
    Field<double> depth;        // visible optical depth of ash and veil over the cell
    Field<double> dimming;      // share of sunlight they keep from the surface
    std::vector<AshPatch> patches;
    std::vector<Field<double>> work;   // transport work space
    PlumeLedger totals;                // the emitted and fallen entries; what is aloft is summed from the fields
    bool aloft{};                      // whether any level holds material; while false the plume costs nothing
};

// Ash on the ground at a place.
struct AshFall {
    double mass{};       // kg/m² fallen so far
    double rate{};       // kg/m²/s now
    uint64_t source{};   // the source most of it came from; 0 where none fell
    double since{};      // calendar seconds at which that source began
};

} // namespace derivean::sim

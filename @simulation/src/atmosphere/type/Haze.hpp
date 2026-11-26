#pragma once

#include "core/type/Field.hpp"

#include <array>
#include <cmath>
#include <vector>

namespace derivean::sim {

// The haze of the low band: the natural aerosol the weather raises, carries, swells and washes out.
// Volcanic ash and the sulphate veil of eruptions are the plume's (Plume.hpp).
inline constexpr int HazeKinds = 4;
enum HazeKind { MineralDust, SeaSalt, Sulphate, Organic };   // sulphate from the sea's DMS, organics from plants

// kg of dry aerosol of each kind that entered the air, that rain took down and that settled out.
// What is aloft is summed from the fields; emitted = aloft + washed + settled.
struct HazeLedger {
    std::array<double, HazeKinds> emitted{}, washed{}, settled{};
};

struct Haze {
    std::array<Field<double>, HazeKinds> mass;   // kg/m² of dry aerosol, mixed through the low band
    Field<double> plants;                        // share of the land plants cover, from the planet's climate
    Field<double> depth;                         // optical depth at 550 nm, swollen by the hour's humidity, as of the last step
    // By the picture's colours (Wavelengths): optical depth against `depth`, and the share of it scattered
    // rather than absorbed (single-scattering albedo), as of the last step.
    std::array<Field<double>, 3> spectrum, albedo;
    std::vector<Field<double>> work;             // transport work space
    HazeLedger totals;
};

// Koschmieder: the distance at which a black target fades to the 2 % contrast an eye still sees.
inline constexpr double SeenContrast = 0.02;
inline double visibilityOf(double extinction) { return -std::log(SeenContrast) / extinction; }   // m, extinction in 1/m

} // namespace derivean::sim

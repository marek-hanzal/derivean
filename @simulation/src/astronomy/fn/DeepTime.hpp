#pragma once

#include "astronomy/type/DeepTime.hpp"

namespace derivean::sim {

DeepTime deepTimeAt(double age);   // age: yr since the hot initial body

// s from noon to noon, for a sidereal day against the equinox.
double solarDay(double siderealDay);

} // namespace derivean::sim

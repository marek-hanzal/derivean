#pragma once

#include "astronomy/constant/DeepTime.hpp"
#include "astronomy/constant/Sky.hpp"

namespace derivean::sim {

// The sky's one time input (S0 review, Q8). `epoch` places second zero on today's ephemeris, which
// fixes the phases of the Earth's turn, the orbit and the Moon there; from it, `seconds` run at the
// rates of `age`. The rates are held over a session: the day lengthens by 2 ms per century, far below
// what a session sees.
struct Instant {
    double epoch{J2000};        // Julian date (TT) of second zero
    double seconds{};           // SI seconds since the epoch
    double age{PresentAge};     // yr since the hot initial body at second zero, as `Calendar::era`
};

} // namespace derivean::sim

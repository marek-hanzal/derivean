#pragma once

#include "chronology/type/Calendar.hpp"

namespace derivean::sim {

// Which solver moves a group's water. An edit wakes its group; while it is awake and time comes in
// steps a wave can be followed in, the event solver runs. Once nothing has risen or fallen faster
// than CalmRate for CalmSpan, or as soon as a step is too long to follow a wave, the quiet solver
// takes over and puts the water where it comes to rest.
inline constexpr double SurgeSpan = 4.0;          // s, longest step the event solver is asked to follow
inline constexpr double CalmRate = 1e-4;          // m/s of rise or fall of standing water
inline constexpr double CalmSpan = 30.0;          // s
inline constexpr double LongestEvent = 6 * 3600.0;   // s after an edit beyond which the quiet solver takes over regardless

// A group nothing happens in is advanced once this much calendar time has gathered.
inline constexpr double QuietQuantum = 3600.0;    // s

// Steps of the quiet solver. After a disturbance they start short and double, so the first hours
// of a filling lake are resolved and a century is not walked hour by hour. Sediment is moved at
// absolute calendar marks, which a jump and a walk to the same date both visit.
inline constexpr double FirstQuietStep = 3600.0;              // s
inline constexpr double SedimentStep = 5.0 * YearSeconds;     // s between the marks

} // namespace derivean::sim

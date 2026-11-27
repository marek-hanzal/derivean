#pragma once

#include "atmosphere/type/SphereForcing.hpp"
#include "atmosphere/type/SpherePoisson.hpp"

#include <vector>

namespace derivean::sim {

// Why a try at a window was taken back (SphereHour::refusals).
enum class HourRefusal {
    Courant,     // a step would have taken more than `limit` of a cell's content out of it
    Inversion,   // a step's inversion did not converge
    Forcing,     // a step's forcing was not admissible (admissibleForcing); no further try
    Shortest,    // the steps cannot grow: the last try took window / shortest of them, or the window
                 // is shorter than `shortest` and no try is made (steps 0)
    Damping,     // even window / shortest steps give a span 2Δt above 1 / stiffest (the damping's
                 // certified bound); no try is made
};

struct HourTry {
    HourRefusal reason{};
    int steps{};   // wind steps the try split the window into
    int at{};      // the step it stopped before, 0 to steps − 1
};

// The flow's state at a window's start, as a refused try or an exception puts it back: the fields
// SphereFlow names its state (vorticity, before, stream, flux, started, the first guess's history)
// and what a step reads besides (step, known with trialProduct, the last inversion's report). The
// rest of the flow is work space a step writes before it reads (product, the other trial vectors,
// tendency, absolute, atVertex) or fixed for the flow (rotation, coriolis, projection, tolerance,
// limit), and is neither copied nor restored (Neon DREV-NEON-059).
struct SphereFlowStart {
    std::vector<double> vorticity, before, stream, streamBack, streamBackTwo, productBack, productBackTwo, flux;
    std::vector<double> trialProduct;   // only while `known`
    int levels{1};
    bool known{};
    double step{};
    bool started{};
    PoissonReport inversion;
};

// The flow's step controller over one window (an hour); S4 plan, binding correction 5 (Codex's
// review DREV-CLAUDE-007-01, point 4). The window is split into `steps` equal wind steps, predicted
// from the windows before; each step's true outflow Courant number (Courant, Friedrichs & Lewy
// 1928) is checked before it is taken, and a window that fails anywhere is taken back as a whole and
// tried again with twice the steps, never more than window / shortest, at most `attempts` times.
// After a window the count rises at once to keep the Courant number below reserve · limit (up to
// window / shortest), and falls only after `patience` windows in a row, each taken at its first try,
// that would need at most `lower` of it, so that setStep, which restarts the leapfrog, runs only on
// a real change. A window taken only at a later try keeps the steps that worked and starts the calm
// count afresh.
struct SphereHour {
    double window{3600};   // s
    int steps{1};          // wind steps of the next window
    double limit{0.4};     // the flow's outflow Courant number (SphereTransport, outflowCourant)
    double reserve{0.8};
    double lower{0.5};
    int patience{3};
    int attempts{4};
    double shortest{30};   // s
    // ν · bound² of the damping the forcing carries (dampingStiffness, 1/s; 0 without): a try is
    // taken only while 2Δt · stiffest ≤ 1: a window starts at no fewer steps than that needs, the
    // count never falls below it, and a window that cannot meet it is refused (Damping).
    double stiffest{};
    int calm{};            // windows in a row that would need at most `lower` of the steps
    double courant{};      // the largest of the last accepted window, its end state included
    std::vector<HourTry> refusals;   // of the last window, in order; the caller logs them
    // Work space, one window at a time, kept for its capacity: the window's start and the forcing a
    // source fills (advanceSphereHour). Not part of the controller: a window neither reads it before
    // writing it nor restores it.
    SphereFlowStart start;
    SphereForcing filled;
};

} // namespace derivean::sim

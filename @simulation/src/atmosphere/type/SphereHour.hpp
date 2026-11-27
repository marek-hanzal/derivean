#pragma once

#include <vector>

namespace derivean::sim {

// Why a try at a window was taken back (SphereHour::refusals).
enum class HourRefusal {
    Courant,     // a step would have taken more than `limit` of a cell's content out of it
    Inversion,   // a step's inversion did not converge, or its forcing was refused
    Shortest,    // the doubled steps would be shorter than `shortest`
};

struct HourTry {
    HourRefusal reason{};
    int steps{};   // wind steps the try split the window into
    int at{};      // the step it stopped before, 0 to steps − 1
};

// The flow's step controller over one window (an hour), Codex DREV-CLAUDE-007-01 point 5: the window
// is split into `steps` equal wind steps, predicted from the windows before; each step's true outflow
// Courant number is checked before it is taken, and a window that fails anywhere is taken back as a
// whole and tried again with twice the steps, at most `attempts` times and never below `shortest`.
// After a window, the count rises at once to keep the Courant number below reserve · limit, and falls
// only after `patience` windows in a row that would need at most `lower` of it, so that setStep, which
// restarts the leapfrog, runs only on a real change.
struct SphereHour {
    double window{3600};   // s
    int steps{1};          // wind steps of the next window
    double limit{0.4};     // the flow's outflow Courant number (SphereTransport, outflowCourant)
    double reserve{0.8};
    double lower{0.5};
    int patience{3};
    int attempts{4};
    double shortest{30};   // s
    int calm{};            // windows in a row that would need at most `lower` of the steps
    double courant{};      // the largest of the last accepted window, its end state included
    std::vector<HourTry> refusals;   // of the last window, in order; the caller logs them
};

} // namespace derivean::sim

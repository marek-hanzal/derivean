#include "atmosphere/fn/SphereHour.hpp"

#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereTransport.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace derivean::sim {
namespace {

// The steps of the next window from the Courant number `worst` the last one reached with `steps`.
void predict(SphereHour& hour, int steps, double worst) {
    const double ceiling = hour.reserve * hour.limit;
    const double share = worst / ceiling;
    const int needed = std::max(1, int(std::ceil(steps * share)));
    if (needed > steps) {
        hour.steps = needed;
        hour.calm = 0;
        return;
    }
    hour.steps = steps;
    const double floor = hour.lower * steps;
    if (needed > floor) {
        hour.calm = 0;
        return;
    }
    if (++hour.calm < hour.patience) return;
    hour.steps = needed;
    hour.calm = 0;
}

} // namespace

bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                       SpherePoisson& poisson, const SphereForcing& forcing) {
    assert(hour.window > 0 && hour.steps >= 1 && hour.attempts >= 1);
    hour.refusals.clear();
    const SphereFlow start = flow;
    int steps = hour.steps;
    for (int attempt = 0; attempt < hour.attempts; ++attempt, steps *= 2) {
        const double step = hour.window / steps;
        if (step < hour.shortest) {
            hour.refusals.push_back({HourRefusal::Shortest, steps, 0});
            break;
        }
        setStep(flow, step);
        double worst = 0;
        int at = 0;
        bool refused = false;
        HourRefusal reason{};
        for (; at < steps; ++at) {
            const double courant = outflowCourant(grid, flow.flux, step);
            worst = std::max(worst, courant);
            if (courant > hour.limit) {
                refused = true, reason = HourRefusal::Courant;
                break;
            }
            if (!stepSphereFlow(flow, grid, stencil, poisson, forcing)) {
                refused = true, reason = HourRefusal::Inversion;
                break;
            }
        }
        if (!refused) {
            worst = std::max(worst, outflowCourant(grid, flow.flux, step));
            hour.courant = worst;
            predict(hour, steps, worst);
            return true;
        }
        hour.refusals.push_back({reason, steps, at});
        flow = start;
    }
    flow = start;
    return false;
}

} // namespace derivean::sim

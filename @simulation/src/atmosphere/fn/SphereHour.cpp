#include "atmosphere/fn/SphereHour.hpp"

#include "atmosphere/fn/SphereFlow.hpp"
#include "atmosphere/fn/SphereTransport.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <type_traits>
#include <utility>

namespace derivean::sim {
namespace {

// The most steps a window may take: none shorter than `shortest`, and few enough to double in an int;
// at least one, since advanceWindow refuses a window shorter than `shortest`.
int mostSteps(const SphereHour& hour) {
    const double most = std::floor(hour.window / hour.shortest);
    return int(std::clamp(most, 1.0, double(INT_MAX / 2)));
}

// Whether `steps` keep the leapfrog span 2Δt within the damping's certified bound,
// 2 (window / steps) · stiffest ≤ 1, as a try would step: the one test of it.
bool dampingAllows(const SphereHour& hour, int steps) {
    const double step = hour.window / steps;
    const double span = 2 * step;
    return span * hour.stiffest <= 1;
}

// The fewest steps dampingAllows, at most mostSteps: from ceil(2 window · stiffest), moved by the
// try's own test where rounding (an ulp near an integer) puts the two apart, up or down.
int fewestSteps(const SphereHour& hour) {
    const int most = mostSteps(hour);
    const double span = 2 * hour.window;
    int fewest = int(std::clamp(std::ceil(span * hour.stiffest), 1.0, double(most)));
    while (fewest < most && !dampingAllows(hour, fewest)) ++fewest;
    while (fewest > 1 && dampingAllows(hour, fewest - 1)) --fewest;
    return fewest;
}

// The steps of the next window from the Courant number `worst` the last one reached with `steps`,
// counted in double and capped at mostSteps, so a huge Courant number neither overflows nor asks for
// steps shorter than allowed. After a window taken only at a later try the count does not fall and
// the calm count starts afresh.
void predict(SphereHour& hour, int steps, double worst, bool retried) {
    const double ceiling = hour.reserve * hour.limit;
    const double share = worst / ceiling;
    const double wanted = std::ceil(steps * share);
    const int needed = int(std::clamp(wanted, double(fewestSteps(hour)), double(mostSteps(hour))));
    if (needed > steps) {
        hour.steps = needed;
        hour.calm = 0;
        return;
    }
    hour.steps = steps;
    if (retried) {
        hour.calm = 0;
        return;
    }
    const double floor = hour.lower * steps;
    if (needed > floor) {
        hour.calm = 0;
        return;
    }
    if (++hour.calm < hour.patience) return;
    hour.steps = needed;
    hour.calm = 0;
}

// The tries of one window; the forcing of each step from `source`, or `held` when there is none.
// The source's output is emptied before each call, so a term it fills on one try only (a friction
// at the coarse step, say) does not reach the next.
bool tryWindow(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
               const SphereForcingSource* source, const SphereForcing* held, const SphereFlow& start) {
    const int most = mostSteps(hour);
    SphereForcing filled;
    int steps = std::max(std::min(hour.steps, most), fewestSteps(hour));
    for (int attempt = 0; attempt < hour.attempts; ++attempt) {
        const double step = hour.window / steps;
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
            if (source) {
                filled.tendency.clear();
                filled.friction.clear();
                (*source)(flow, filled);
            }
            const SphereForcing& forcing = source ? filled : *held;
            if (!admissibleForcing(forcing, size_t(grid.cells))) {
                refused = true, reason = HourRefusal::Forcing;
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
            predict(hour, steps, worst, !hour.refusals.empty());
            return true;
        }
        hour.refusals.push_back({reason, steps, at});
        flow = start;
        if (reason == HourRefusal::Forcing) break;   // more steps would not mend it
        if (steps == most) {
            hour.refusals.push_back({HourRefusal::Shortest, steps, 0});
            break;
        }
        steps = std::min(2 * steps, most);
    }
    return false;   // the flow is `start` again since the last refusal
}

// One window as a transaction: a window shorter than the shortest step, or one whose most steps the
// damping's bound refuses, is refused before the flow or the steps change; whatever a try throws
// (the source, an allocation) puts the flow and the hour back as they came in and passes on. The
// snapshots are moved back, which allocates nothing, so the restore holds when memory has run out
// (Codex DREV-CLAUDE-007-04); a copy could fail there too.
static_assert(std::is_nothrow_move_assignable_v<SphereFlow> && std::is_nothrow_move_assignable_v<SphereHour>,
              "the hour's restore must not throw");
bool advanceWindow(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                   const SphereForcingSource* source, const SphereForcing* held) {
    assert(hour.window > 0 && hour.shortest > 0 && hour.steps >= 1 && hour.attempts >= 1);
    if (hour.window < hour.shortest) {
        // Built aside first: an allocation that fails leaves the last window's refusals in place.
        std::vector<HourTry> only{{HourRefusal::Shortest, 0, 0}};
        hour.refusals.swap(only);
        return false;
    }
    if (!dampingAllows(hour, mostSteps(hour))) {
        std::vector<HourTry> only{{HourRefusal::Damping, mostSteps(hour), 0}};
        hour.refusals.swap(only);
        return false;
    }
    SphereHour entry = hour;
    SphereFlow start = flow;
    hour.refusals.clear();
    try {
        return tryWindow(hour, flow, grid, stencil, poisson, source, held, start);
    } catch (...) {
        flow = std::move(start);
        hour = std::move(entry);
        throw;
    }
}

} // namespace

bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                       SpherePoisson& poisson, const SphereForcingSource& source) {
    return advanceWindow(hour, flow, grid, stencil, poisson, &source, nullptr);
}

bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                       SpherePoisson& poisson, const SphereForcing& forcing) {
    return advanceWindow(hour, flow, grid, stencil, poisson, nullptr, &forcing);
}

} // namespace derivean::sim

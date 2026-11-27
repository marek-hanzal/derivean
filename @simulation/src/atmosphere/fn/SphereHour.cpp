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

// The flow's state vectors with their places in the start, the one list of them (SphereFlowStart):
// each(flow's, start's) for every one, trialProduct only while `known`; then the state's scalars,
// from `from` into `to`. A field SphereFlow adds to its state joins here and in SphereFlowStart.
template <class Flow, class Start, class Each> void eachStateVector(Flow& flow, Start& start, bool known, Each&& each) {
    each(flow.vorticity, start.vorticity);
    each(flow.before, start.before);
    each(flow.stream, start.stream);
    each(flow.streamBack, start.streamBack);
    each(flow.streamBackTwo, start.streamBackTwo);
    each(flow.productBack, start.productBack);
    each(flow.productBackTwo, start.productBackTwo);
    each(flow.flux, start.flux);
    if (known) each(flow.trialProduct, start.trialProduct);
}

template <class To, class From> void copyStateScalars(To& to, const From& from) noexcept {
    to.levels = from.levels, to.known = from.known, to.step = from.step, to.started = from.started;
    to.inversion = from.inversion;
}

// The flow's state into `start`, into the capacity its vectors kept from the last window, so a window
// allocates only while they first grow (Neon DREV-NEON-059). Before the window changes anything: what
// it throws leaves the flow and the hour's controller as they were.
void saveStart(SphereFlowStart& start, const SphereFlow& flow) {
    const auto into = [](const std::vector<double>& from, std::vector<double>& to) { to.assign(from.begin(), from.end()); };
    eachStateVector(flow, start, flow.known, into);
    copyStateScalars(start, flow);
}

// A refused try taken back: the start copied into the flow, which keeps it for the next try. It
// allocates only where a vector of the flow lacks the capacity; should that fail, advanceWindow's
// restore swaps the start in, whole.
void copyStart(SphereFlow& flow, const SphereFlowStart& start) {
    const auto back = [](std::vector<double>& to, const std::vector<double>& from) { to.assign(from.begin(), from.end()); };
    eachStateVector(flow, start, start.known, back);
    copyStateScalars(flow, start);
}

// The restore when a try throws: the start's vectors swapped into the flow, which allocates nothing,
// so it holds when memory has run out (Codex DREV-CLAUDE-007-04); the start then holds the try's.
void swapStart(SphereFlow& flow, SphereFlowStart& start) noexcept {
    eachStateVector(flow, start, start.known, [](std::vector<double>& a, std::vector<double>& b) noexcept { a.swap(b); });
    copyStateScalars(flow, start);
}

// A window's forcing: a source's output each step (hour.filled), or one held through the window, and
// a friction fixed for the window or none; the parts fixed for the window are checked once.
struct WindowForcing {
    const SphereForcingSource* source{};
    const SphereForcing* held{};
    const std::vector<double>* friction{};   // fixed for the window; the source then fills no friction
};

// The tries of one window. The source's output is emptied before each call, so a term it fills on one
// try only (a friction at the coarse step, say) does not reach the next.
bool tryWindow(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
               const WindowForcing& given) {
    const size_t cells = size_t(grid.cells);
    const bool fixedAdmissible = given.source ? !given.friction || admissibleFriction(*given.friction, cells)
                                              : admissibleForcing(*given.held, cells);
    SphereForcing& filled = hour.filled;
    const int most = mostSteps(hour);
    int steps = std::max(std::min(hour.steps, most), fewestSteps(hour));
    for (int attempt = 0; attempt < hour.attempts; ++attempt) {
        const double step = hour.window / steps;
        setStep(flow, step);
        double worst = 0;
        int at = 0;
        bool refused = false;
        HourRefusal reason{};
        for (; at < steps; ++at) {
            const double courant = outflowCourant(grid, stencil, flow.flux, step);
            worst = std::max(worst, courant);
            if (courant > hour.limit) {
                refused = true, reason = HourRefusal::Courant;
                break;
            }
            bool admissible = fixedAdmissible;
            if (given.source) {
                filled.tendency.clear();
                filled.friction.clear();
                (*given.source)(flow, filled);
                admissible = admissible && admissibleTendency(filled.tendency, cells) &&
                             (given.friction ? filled.friction.empty() : admissibleFriction(filled.friction, cells));
            }
            if (!admissible) {
                refused = true, reason = HourRefusal::Forcing;
                break;
            }
            const std::vector<double>& tendency = given.source ? filled.tendency : given.held->tendency;
            const std::vector<double>& friction = given.friction ? *given.friction : given.source ? filled.friction : given.held->friction;
            if (!stepAdmittedFlow(flow, grid, stencil, poisson, tendency, friction)) {
                refused = true, reason = HourRefusal::Inversion;
                break;
            }
        }
        if (!refused) {
            worst = std::max(worst, outflowCourant(grid, stencil, flow.flux, step));
            hour.courant = worst;
            predict(hour, steps, worst, !hour.refusals.empty());
            return true;
        }
        hour.refusals.push_back({reason, steps, at});
        copyStart(flow, hour.start);
        if (reason == HourRefusal::Forcing) break;   // more steps would not mend it
        if (steps == most) {
            hour.refusals.push_back({HourRefusal::Shortest, steps, 0});
            break;
        }
        steps = std::min(2 * steps, most);
    }
    return false;   // the flow is the window's start again since the last refusal
}

// One window as a transaction: a window shorter than the shortest step, or one whose most steps the
// damping's bound refuses, is refused before the flow or the steps change; whatever a try throws
// (the source, an allocation) puts the flow and the hour back as they came in and passes on. The
// start is swapped back and the controller's refusals moved back, which allocates nothing, so the
// restore holds when memory has run out (Codex DREV-CLAUDE-007-04); a copy could fail there too.
static_assert(std::is_nothrow_swappable_v<std::vector<double>> && std::is_nothrow_move_assignable_v<std::vector<HourTry>>,
              "the hour's restore must not throw");
bool advanceWindow(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil, SpherePoisson& poisson,
                   const WindowForcing& given) {
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
    // Of the controller a try changes only the refusals before it can throw: steps, calm and courant
    // change once the window is taken (predict), after the last call that can.
    std::vector<HourTry> refusals = hour.refusals;
    saveStart(hour.start, flow);
    hour.refusals.clear();
    try {
        return tryWindow(hour, flow, grid, stencil, poisson, given);
    } catch (...) {
        swapStart(flow, hour.start);
        hour.refusals = std::move(refusals);
        throw;
    }
}

} // namespace

bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                       SpherePoisson& poisson, const SphereForcingSource& source) {
    return advanceWindow(hour, flow, grid, stencil, poisson, {&source, nullptr, nullptr});
}

bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                       SpherePoisson& poisson, const SphereForcingSource& source, const std::vector<double>& friction) {
    return advanceWindow(hour, flow, grid, stencil, poisson, {&source, nullptr, &friction});
}

bool advanceSphereHour(SphereHour& hour, SphereFlow& flow, const SphereGrid& grid, const SphereStencil& stencil,
                       SpherePoisson& poisson, const SphereForcing& forcing) {
    return advanceWindow(hour, flow, grid, stencil, poisson, {nullptr, &forcing, nullptr});
}

} // namespace derivean::sim

#include "nature/type/Weather.hpp"
#include "nature/service/Nature.hpp"

#include <cmath>
#include <thread>

namespace derivean {
namespace {

constexpr double SliceMs = 1000.0;   // a task that is behind steps this long before it publishes what it has

using Clock = std::chrono::steady_clock;

double millisecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int64_t stepAt(const sim::Calendar& calendar) { return int64_t(std::floor(calendar.seconds / sim::AirStep)); }

// What the game reads of the weather is its state: the step's work space and the Poisson solver
// stay with the working copy, which they are handed back to.
WeatherState publish(sim::Atmosphere& air) {
    sim::AirWork work = std::move(air.work);
    sim::PoissonSolver poisson = std::move(air.poisson);
    auto copy = std::make_shared<const sim::Atmosphere>(air);
    air.work = std::move(work);
    air.poisson = std::move(poisson);
    auto clouds = std::make_shared<const sim::CloudField>(sim::cloudField(*copy));
    return {std::move(copy), std::move(clouds)};
}

void collect(Weather& weather) {
    for (WeatherResult& result : weather.finished->collect()) {
        weather.working = std::move(result.working);
        weather.seen = std::move(result.state);
        weather.flying = false;
        weather.fresh = true;
        weather.steps += size_t(result.steps);
        weather.stepMs += result.milliseconds;
    }
}

} // namespace

void startWeather(Weather& weather, Nature& nature, const sim::Calendar& start) {
    weather.finished = std::make_shared<Mailbox<WeatherResult>>();
    weather.flying = true;
    nature.air->submit([box = weather.finished, &planet = nature.world.planet, &pool = *nature.air, start] {
        const auto began = Clock::now();
        auto air = std::make_shared<sim::Atmosphere>(sim::startAtmosphere(planet, start, pool));
        WeatherState state = publish(*air);
        box->post({std::move(air), std::move(state), 0, millisecondsSince(began)});
    });
}

void awaitWeather(Weather& weather) {
    while (!weather.seen.air) {
        collect(weather);
        if (!weather.seen.air) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void advanceWeather(Weather& weather, Nature& nature, const sim::Calendar& now) {
    collect(weather);
    const int64_t target = stepAt(now);
    if (weather.flying || !weather.working || weather.working->step >= target) return;
    weather.flying = true;
    nature.air->submit([box = weather.finished, air = weather.working, &pool = *nature.air, target] {
        const auto began = Clock::now();
        int steps = 0;
        // One step at a time: each lands on the simulation's absolute hourly grid, so the state reached
        // does not depend on where a slice ends.
        while (air->step < target && millisecondsSince(began) < SliceMs) {
            sim::advanceAtmosphere(*air, sim::Calendar{air->era, double(air->step + 1) * sim::AirStep}, pool);
            ++steps;
        }
        WeatherState state = publish(*air);
        box->post({air, std::move(state), steps, millisecondsSince(began)});
    });
}

double weatherLag(const Weather& weather, const sim::Calendar& now) {
    if (!weather.seen.air) return 0;
    return double(std::max<int64_t>(0, stepAt(now) - weather.seen.air->step)) * sim::AirStep;
}

} // namespace derivean

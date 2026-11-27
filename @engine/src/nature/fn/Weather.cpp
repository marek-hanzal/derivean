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

// The work spaces of a step and the Poisson solver are not part of the weather: they are set aside
// while the working state is copied and handed back to it, even when the copy fails.
struct SetAside {
    sim::Atmosphere& air;
    sim::AirWork work;
    sim::PoissonSolver poisson;
    std::vector<sim::Field<double>> haze, plume;
    explicit SetAside(sim::Atmosphere& from)
        : air(from), work(std::move(from.work)), poisson(std::move(from.poisson)), haze(std::move(from.haze.work)), plume(std::move(from.plume.work)) {}
    SetAside(const SetAside&) = delete;
    SetAside& operator=(const SetAside&) = delete;
    ~SetAside() {
        air.work = std::move(work);
        air.poisson = std::move(poisson);
        air.haze.work = std::move(haze);
        air.plume.work = std::move(plume);
    }
};

WeatherState publish(sim::Atmosphere& air) {
    std::shared_ptr<const sim::Atmosphere> copy;
    {
        const SetAside aside(air);
        copy = std::make_shared<const sim::Atmosphere>(air);
    }
    auto clouds = std::make_shared<const sim::CloudField>(sim::cloudField(*copy));
    return {std::move(copy), std::move(clouds)};
}

void collect(Weather& weather) {
    for (WeatherResult& result : weather.finished->collect()) {
        weather.start.reset();
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
    weather.start = std::make_shared<sim::Workers>(nature.startThreads);
    weather.start->submit([box = weather.finished, &planet = nature.world.planet, &pool = *weather.start, start] {
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

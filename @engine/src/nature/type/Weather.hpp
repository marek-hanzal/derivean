#pragma once

#include "nature/type/Air.hpp"
#include "nature/type/Mailbox.hpp"

#include <chrono>
#include <memory>

namespace derivean {

struct Nature;

// One finished state of the atmosphere and the cloud description made from it.
struct WeatherState {
    std::shared_ptr<const sim::Atmosphere> air;
    std::shared_ptr<const sim::CloudField> clouds;
};

struct WeatherResult {
    std::shared_ptr<sim::Atmosphere> working;
    WeatherState state;
    int steps{};
    double milliseconds{};
};

// The planet's weather as the game holds it. The simulation steps it on the engine's workers, an
// hour of calendar time at a time, and the game thread only ever reads a finished copy: it never
// waits for a step, and a calendar that runs ahead of the stepping shows up as lag, not as a stall.
// The state is a function of the start date and the calendar alone, however the calendar got there.
struct Weather {
    std::shared_ptr<sim::Atmosphere> working;      // the state being stepped; the task in flight owns it while `flying`
    std::shared_ptr<Mailbox<WeatherResult>> finished;
    WeatherState seen;                             // newest finished state; empty until the atmosphere has started
    std::shared_ptr<sim::Workers> start;           // the pool the first state spins up on, released when it arrives
    bool flying{};
    bool fresh{};                                  // `seen.clouds` has not gone out in a frame yet
    std::chrono::steady_clock::time_point cloudsSent{};
    size_t steps{};                                // steps taken so far and the background time they took
    double stepMs{};
};

// Orders the first state for the start date; returns at once. The spin-up is the whole of the
// work until the first state is published, and the game waits for it: it runs on a pool of its
// own, nature.startThreads wide, not on nature.air, which steps the weather afterwards. The
// state does not depend on the thread count. The pool reads nature's planet until it is joined,
// so the Weather must not outlive nature.
void startWeather(Weather& weather, Nature& nature, const sim::Calendar& start);

// Blocks until that first state exists.
void awaitWeather(Weather& weather);

// Takes in finished states and orders the steps the calendar asks for; never blocks.
void advanceWeather(Weather& weather, Nature& nature, const sim::Calendar& now);

// Calendar seconds the newest finished state is behind the hour the calendar has reached.
double weatherLag(const Weather& weather, const sim::Calendar& now);

} // namespace derivean

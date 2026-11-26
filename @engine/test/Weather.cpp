#include "nature/service/Nature.hpp"
#include "nature/type/Weather.hpp"

#include "Expect.hpp"

#include <chrono>
#include <thread>

using namespace derivean;

namespace {

void settle(Weather& weather, Nature& nature, const sim::Calendar& now) {
    const auto start = std::chrono::steady_clock::now();
    do {
        advanceWeather(weather, nature, now);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        expect(std::chrono::steady_clock::now() - start < std::chrono::seconds(120), "The atmosphere never caught up with the calendar");
    } while (weatherLag(weather, now) > 0 || weather.flying);
}

} // namespace

// The weather of a date is the same whether the calendar crept there frame by frame or leapt
// there in a few uneven strides, and the water it moves around is neither made nor lost.
void testWeather() {
    Nature nature(NatureOptions{42, 2e9, 0});
    const sim::Calendar start{2e9, 0};
    Weather paced, leaped;
    startWeather(paced, nature, start);
    startWeather(leaped, nature, start);
    awaitWeather(paced);
    awaitWeather(leaped);
    const std::shared_ptr<const sim::Atmosphere> first = paced.seen.air;
    expect(first->step == 0 && paced.seen.clouds && paced.seen.clouds->size == sim::AirCells, "The atmosphere must start at the start date");

    sim::Calendar slow = start, fast = start;
    double worst = 0;
    for (int frame = 0; frame < 24 * 60; ++frame) {
        slow.seconds += 60;
        const auto began = std::chrono::steady_clock::now();
        advanceWeather(paced, nature, slow);
        worst = std::max(worst, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count());
        if (frame % 40 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    for (double stride : {5000.0, 70000.0, 11400.0}) {
        fast.seconds += stride;
        advanceWeather(leaped, nature, fast);
    }
    expect(weatherLag(leaped, fast) > 0, "A leap of the calendar must show as lag, not as a wait");
    settle(paced, nature, slow);
    settle(leaped, nature, fast);
    expect(worst < 250, "Asking the weather to follow the calendar must return at once");

    // The simulation taken to the same date in one call is the measure: slicing must not show.
    sim::Atmosphere whole = sim::startAtmosphere(nature.world.planet, start, *nature.air);
    sim::advanceAtmosphere(whole, slow, *nature.air);
    const sim::Atmosphere& a = *paced.seen.air;
    const sim::Atmosphere& b = *leaped.seen.air;
    expect(slow.seconds == fast.seconds && a.step == 24 && b.step == 24 && whole.step == 24, "Every calendar must have reached the same hour");
    for (const sim::Atmosphere* other : {&b, static_cast<const sim::Atmosphere*>(&whole)}) {
        expect(a.airTemperature.cells == other->airTemperature.cells && a.windX.cells == other->windX.cells && a.snow.cells == other->snow.cells &&
               a.rainRate.cells == other->rainRate.cells, "Weather depends on how the calendar was paced");
        for (size_t band = 0; band < sim::AirBands; ++band)
            expect(a.vapour[band].cells == other->vapour[band].cells && a.liquid[band].cells == other->liquid[band].cells &&
                   a.ice[band].cells == other->ice[band].cells, "Clouds depend on how the calendar was paced");
    }
    expect(paced.seen.clouds->band[0].cover == leaped.seen.clouds->band[0].cover, "The cloud snapshot depends on how the calendar was paced");
    expect(a.airTemperature.cells != first->airTemperature.cells, "A day must change the weather");

    const double before = sim::total(sim::waterLedger(*first)), after = sim::total(sim::waterLedger(a));
    expect(std::abs(after - before) <= 1e-9 * before, "The atmosphere made or lost water");
    std::cout << "weather: " << paced.steps << " hourly steps, " << paced.stepMs / double(std::max<size_t>(1, paced.steps))
              << " ms each in the background; slowest call from the game thread " << worst << " ms\n";
}

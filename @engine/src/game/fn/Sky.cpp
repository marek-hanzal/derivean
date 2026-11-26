#include "game/type/WorldProjection.hpp"

namespace derivean {
namespace {

constexpr double CloudInterval = 1.0;   // s of real time between cloud snapshots handed to the renderer

} // namespace

void projectSky(Game& game, Frame& frame, std::optional<double> standing) {
    const sim::Calendar& calendar = game.calendar;
    frame.calendar = calendar;
    frame.dayOfYear = sim::dayOfYear(calendar);
    frame.hour = sim::hourOfDay(calendar);
    const sim::Vec2 place{frame.eye[0], frame.eye[2]};
    const sim::SunDirection sun = sim::sunDirectionAt(place, calendar);
    frame.sun = {sun.x, sun.up, sun.z};

    Weather& weather = game.weather;
    const sim::Atmosphere& air = *weather.seen.air;
    const double ground = standing.value_or(frame.eye[1] - PlayerEyeHeight);
    const sim::WeatherSample sample = sim::weatherAt(air, place, ground, calendar);
    frame.weather = {sample.temperature, sample.rain, sample.snow, sample.wind.x, sample.wind.z, sample.cloudCover, sample.humidity,
                     sim::pressureAt(std::max(0.0, ground - air.seaLevel)), sample.snowpack, weatherLag(weather, calendar)};

    const auto now = std::chrono::steady_clock::now();
    if (weather.fresh && std::chrono::duration<double>(now - weather.cloudsSent).count() >= CloudInterval) {
        frame.clouds = weather.seen.clouds;
        frame.cloudsAir = weather.seen.air;
        frame.cloudsTime = double(air.step) * sim::AirStep;
        weather.fresh = false;
        weather.cloudsSent = now;
    }
}

} // namespace derivean

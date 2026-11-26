#include "atmosphere/fn/Cover.hpp"
#include "atmosphere/fn/Kernels.hpp"

#include "climate/fn/Moisture.hpp"
#include "climate/type/Climate.hpp"

#include <cstdio>

namespace derivean::sim {
namespace {

double read(const AirField& field, Vec2 position) {
    return sample(field, position.x / AirCell - 0.5, position.z / AirCell - 0.5, Kernel::Linear, Edge::Wrap);
}

// °C at which air of this temperature and relative humidity would be saturated.
double dewPointOf(double celsius, double humidity) { return celsius - 20.0 * (1.0 - saturate(humidity)); }

} // namespace

WeatherSample weatherAt(const Atmosphere& atmosphere, Vec2 position, double groundElevation, const Calendar& calendar) {
    WeatherSample weather;
    double clear = 1.0;
    for (size_t band = 0; band < AirBands; ++band) {
        const double path = std::max(0.0, read(atmosphere.liquid[band], position) + read(atmosphere.ice[band], position));
        weather.cloudWater += path;
    }
    const int x = wrapAir(floorToInt(position.x / AirCell)), z = wrapAir(floorToInt(position.z / AirCell));
    const AirColumn column = columnAt(atmosphere, x, z);
    const ColumnProfile profile = profileOf(column);
    for (const CloudLayer& layer : describeClouds(column, profile, 0.0).layer) clear *= 1.0 - layer.cover;
    weather.cloudCover = 1.0 - clear;
    weather.wind = {read(atmosphere.windX, position), read(atmosphere.windZ, position)};

    // The sun swings the temperature of the ground it reaches; cloud and open water damp the swing.
    const double water = read(atmosphere.openWater, position);
    const double swing = dailySwing(latitudeAt(position.z), solarDeclination(calendar), hourOfDay(calendar));
    const double range = mix(LandDaily, SeaDaily, water) * (1.0 - CloudDamping * read(atmosphere.shade, position));
    const double height = std::max(0.0, groundElevation - atmosphere.seaLevel);
    weather.temperature = read(atmosphere.airTemperature, position) - LapseRate * height + range * swing;

    const double falling = (read(atmosphere.rainRate, position) + read(atmosphere.snowRate, position)) * 3600.0;
    weather.snow = falling * snowShare(weather.temperature);
    weather.rain = falling - weather.snow;
    weather.humidity = saturate(read(atmosphere.vapour[Low], position) * profile.perCapacity[Low]);
    weather.snowpack = snowAt(atmosphere, position, groundElevation);
    return weather;
}

WeatherReport inspectWeather(const Atmosphere& atmosphere, Vec2 position, double groundElevation, const Calendar& calendar) {
    WeatherReport report;
    report.weather = weatherAt(atmosphere, position, groundElevation, calendar);
    const int x = wrapAir(floorToInt(position.x / AirCell)), z = wrapAir(floorToInt(position.z / AirCell));
    const size_t cell = atmosphere.ground.index(x, z);
    report.column = columnAt(atmosphere, x, z);
    report.profile = profileOf(report.column);
    report.clouds = describeClouds(report.column, report.profile, atmosphere.rainRate.cells[cell] + atmosphere.snowRate.cells[cell]);
    for (int band = 0; band < AirBands; ++band) {
        report.wind[size_t(band)] = bandWind(atmosphere, band, x, z);
        report.humidity[size_t(band)] = report.column.vapour[size_t(band)] * report.profile.perCapacity[size_t(band)];
    }
    report.pressure = pressureAt(std::max(0.0, groundElevation - atmosphere.seaLevel));
    report.dewPoint = dewPointOf(report.weather.temperature, report.weather.humidity);
    report.verticalSpeed = (report.column.lift + report.column.glide) / LiftTime;
    report.snowLine = snowLineOf(atmosphere, x, z);
    report.ledger = waterLedger(atmosphere);
    report.cause = explainClouds(report.column, report.profile, report.clouds);
    report.ash = ashFallAt(atmosphere, position);
    report.aerosolDepth = atmosphere.plume.depth.cells[cell];
    report.hazeDepth = hazeDepths(atmosphere, x, z);
    report.visibility = viewDistance(atmosphere, position, groundElevation - atmosphere.seaLevel);
    if (report.ash.mass > 0) {
        char line[160];
        std::snprintf(line, sizeof line, "; ash from source %llu, in eruption since day %.1f: %.3g kg/m² fallen here", static_cast<unsigned long long>(report.ash.source),
                      report.ash.since / DaySeconds, report.ash.mass);
        report.cause += line;
    }
    return report;
}

} // namespace derivean::sim

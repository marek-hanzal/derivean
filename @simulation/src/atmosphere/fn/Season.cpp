#include "atmosphere/fn/Kernels.hpp"

#include "climate/constant/Moisture.hpp"
#include "climate/type/Climate.hpp"

namespace derivean::sim {
namespace {

constexpr double Radians = 3.141592653589793 / 180.0;
constexpr double BeltShift = 0.35;      // degrees the belts follow per degree of solar declination
constexpr double BeltLag = 30.0;        // days the belts trail the sun
constexpr double JetDrive = 1.6;        // target over climatological belt speed; eddies erode the difference
constexpr double EddyGrowth = 1.0 / (1.5 * DaySeconds);   // 1/s where the temperature contrast is strong
constexpr double RowDegrees = 360.0 / AirCells;
constexpr double ShearPerGradient = 12.0;   // m/s of westerly shear per band for 1 °C per degree of latitude
constexpr double ShearLimit = 12.0;         // m/s

// Latitude the belts of a row correspond to once they have followed the sun.
double beltLatitude(double latitude, double shift) { return latitude - shift * std::cos(latitude * Radians); }

} // namespace

AirSeason seasonOf(const Atmosphere& atmosphere) {
    AirSeason season;
    const double day = double(atmosphere.step) * AirStep / DaySeconds;
    const double declination = season.declination = declinationOn(day);
    const double shift = BeltShift * declinationOn(day - BeltLag);

    // The belts of each half blow as hard as its simulated subtropics are warmer than its subpolar
    // rows, measured against the same contrast of the annual-mean climate.
    double contrast[2]{}, reference[2]{};
    for (int z = 0; z < AirCells; ++z) {
        const double band = std::abs(atmosphere.latitude[size_t(z)]);
        const double side = band >= 15 && band < 35 ? 1.0 : band >= 55 && band < 75 ? -1.0 : 0.0;
        const int half = atmosphere.latitude[size_t(z)] >= 0 ? 0 : 1;
        contrast[half] += side * atmosphere.zonalTemperature[size_t(z)];
        reference[half] += side * atmosphere.baseTemperature[size_t(z)];
    }
    const double strength[2] = {std::clamp(contrast[0] / reference[0], 0.5, 1.8), std::clamp(contrast[1] / reference[1], 0.5, 1.8)};

    AirRows zonal{};
    for (int z = 0; z < AirCells; ++z) {
        const double latitude = atmosphere.latitude[size_t(z)];
        const double belt = beltLatitude(latitude, shift);
        const double drive = mix(strength[1], strength[0], smoothstep(-10.0, 10.0, belt));
        zonal[size_t(z)] = JetDrive * drive * beltWind(belt).east;
        season.insolation[size_t(z)] = dailyInsolation(latitude, declination);

        const double corner = beltLatitude(atmosphere.vertexLatitude[size_t(z)], shift);
        season.drift[size_t(z)] = beltWind(corner).poleward * (corner >= 0 ? 1.0 : -1.0) * atmosphere.vertexNorth[size_t(z)];
    }
    for (int z = 0; z < AirCells; ++z) {
        // Temperature gradient across the corner row, from the two rows on either side of it.
        const double gradient = std::abs(atmosphere.zonalTemperature[size_t(wrapAir(z + 1))] + atmosphere.zonalTemperature[size_t(z)] -
                                         atmosphere.zonalTemperature[size_t(wrapAir(z - 1))] - atmosphere.zonalTemperature[size_t(wrapAir(z - 2))]) /
                                (4 * RowDegrees);
        season.growth[size_t(z)] = EddyGrowth * storminess(gradient, atmosphere.vertexLatitude[size_t(z)]);
        season.jet[size_t(z)] = -(zonal[size_t(z)] - zonal[size_t(wrapAir(z - 1))]) / AirCell;

        // Thermal wind: where the air gets colder towards the pole, the westerlies grow with height.
        const double latitude = atmosphere.latitude[size_t(z)];
        const double poleward = (latitude >= 0 ? 1.0 : -1.0) * atmosphere.vertexNorth[size_t(z)];
        const double cooling = -poleward * (atmosphere.zonalTemperature[size_t(wrapAir(z + 1))] - atmosphere.zonalTemperature[size_t(wrapAir(z - 1))]) / (2 * RowDegrees);
        season.shear[size_t(z)] = std::clamp(ShearPerGradient * cooling * std::sin(2 * std::abs(latitude) * Radians), -ShearLimit, ShearLimit);
        const double spreading = (season.drift[size_t(wrapAir(z + 1))] - season.drift[size_t(z)]) / AirCell;
        season.overturning[size_t(z)] = -MoistDepth * spreading * LiftTime;
    }
    return season;
}

} // namespace derivean::sim

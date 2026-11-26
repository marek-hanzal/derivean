#include "atmosphere/fn/Kernels.hpp"

#include "climate/type/Climate.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;

} // namespace

Vec2 bandWind(const Atmosphere& atmosphere, int band, int x, int z) {
    const double drift = 0.5 * (atmosphere.season.drift[size_t(z)] + atmosphere.season.drift[size_t(wrapAir(z + 1))]);
    const double faster = BandWind[band];   // the wind the band's water is carried with, see fillBand
    return {faster * atmosphere.windX.at(x, z) + band * atmosphere.season.shear[size_t(z)],
            faster * (atmosphere.windZ.at(x, z) - drift) + (band == Low ? drift : 0.0)};
}

CloudField cloudField(const Atmosphere& atmosphere) {
    const size_t cells = size_t(AirCells) * AirCells;
    CloudField field;
    field.size = AirCells;
    field.cell = AirCell;
    field.span = PlanetSpan;
    for (CloudField::BandChannels& channels : field.band) {
        for (auto* channel : {&channels.cover, &channels.base, &channels.top, &channels.opticalDepth, &channels.iceShare, &channels.windX, &channels.windZ})
            channel->resize(cells);
        channels.genus.resize(cells);
    }
    field.heaping.resize(cells);
    field.rain.resize(cells);
    field.snow.resize(cells);
    field.seaLevel = atmosphere.seaLevel;
    field.aerosolDepth.assign(atmosphere.plume.depth.cells.begin(), atmosphere.plume.depth.cells.end());
    field.ashAloft.resize(cells);
    field.ashLevel.resize(cells);
    field.snowLine.resize(cells);
    field.snowLadder.reserve(cells * SnowRungs);
    for (const SnowColumn& rungs : atmosphere.snowLadder.cells)
        for (double pack : rungs) field.snowLadder.push_back(float(pack));
    for (int z = 0; z < AirCells; ++z) {
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = atmosphere.ground.index(x, z);
            const AirColumn column = columnAt(atmosphere, x, z);
            const ColumnProfile profile = profileOf(column);
            const CloudReport clouds = describeClouds(column, profile, atmosphere.rainRate.cells[cell] + atmosphere.snowRate.cells[cell]);
            for (int band = 0; band < AirBands; ++band) {
                CloudField::BandChannels& channels = field.band[size_t(band)];
                const CloudLayer& layer = clouds.layer[size_t(band)];
                const double path = layer.liquidPath + layer.icePath;
                const Vec2 wind = bandWind(atmosphere, band, x, z);
                channels.cover[cell] = float(layer.cover);
                channels.base[cell] = float(layer.base);
                channels.top[cell] = float(layer.top);
                channels.opticalDepth[cell] = float(layer.opticalDepth);
                channels.iceShare[cell] = float(path > 0 ? layer.icePath / path : 0.0);
                channels.windX[cell] = float(wind.x);
                channels.windZ[cell] = float(wind.z);
                channels.genus[cell] = uint8_t(layer.genus);
            }
            field.heaping[cell] = float(profile.convection);
            field.snowLine[cell] = float(snowLineOf(atmosphere, x, z));
            // Fine ash over the cell and the height its mass is centred at, the last level counted 4 km over the tropopause.
            const double levels[PlumeLevels] = {0.5 * profile.height[1], 0.5 * (profile.height[1] + profile.height[2]),
                                                0.5 * (profile.height[2] + profile.height[3]), profile.height[3] + 4000.0};
            double ash = 0, lifted = 0;
            for (size_t level = 0; level < PlumeLevels; ++level) {
                ash += atmosphere.plume.dust[level].cells[cell];
                lifted += atmosphere.plume.dust[level].cells[cell] * levels[level];
            }
            field.ashAloft[cell] = float(ash);
            field.ashLevel[cell] = float(ash > 0 ? column.ground + lifted / ash : 0.0);
            field.rain[cell] = float(atmosphere.rainRate.cells[cell]);
            field.snow[cell] = float(atmosphere.snowRate.cells[cell]);
        }
    }
    return field;
}

SunDirection sunDirectionAt(Vec2 position, const Calendar& calendar) {
    const double latitude = latitudeAt(position.z) * Pi / 180.0, declination = solarDeclination(calendar) * Pi / 180.0;
    const double hourAngle = (hourOfDay(calendar) - 12.0) * Pi / 12.0;
    const double north = std::cos(latitude) * std::sin(declination) - std::sin(latitude) * std::cos(declination) * std::cos(hourAngle);
    SunDirection sun;
    sun.x = -std::cos(declination) * std::sin(hourAngle);
    sun.up = std::sin(latitude) * std::sin(declination) + std::cos(latitude) * std::cos(declination) * std::cos(hourAngle);
    sun.z = north * northwardZ(position.z);
    return sun;
}

} // namespace derivean::sim

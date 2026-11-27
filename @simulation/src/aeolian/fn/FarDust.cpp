#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"
#include "climate/type/Climate.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// Deserts raise dust in proportion to how seldom rain reaches their ground and how hard the wind
// blows over it (DustRise and DustDrift, constant/Wind.hpp).
constexpr float DustWindCap = 2.0f;     // most a windier place multiplies that by
// The air is followed back along the annual mean wind until the coarser dust it could carry is gone.
constexpr double TraceStep = 30e3;      // m
constexpr double TraceReach = 2400e3;   // m, four fall lengths of the fine dust
constexpr double Calm = 0.3;            // m/s of mean wind below which the air is taken to stand
// Tile origins step by TileCells, a multiple of the stride, so all tiles read one world lattice of points.
constexpr int FarStride = 64;           // cells between the points the far dust is read at

// Dust comes from the driest ground almost alone.
float dustSource(float rain, float celsius) {
    const float square = aridityOf(rain, celsius) * aridityOf(rain, celsius);
    return square * square;
}

// m/yr of fines the land at a planet point gives up to the wind, from its rain (m/yr) and the
// potential sand drift of its regime.
float emissionAt(const Planet& planet, Vec2 point, float rain, double potential) {
    const float warmth = float(samplePlanet(planet.temperature, point, Kernel::Linear));
    return DustRise * dustSource(rain, warmth) * std::min(DustWindCap, float(potential) / DustDrift);
}

// m/yr of fines settling at `position` in dry air out of dust raised anywhere upwind on the planet grid.
// It is a function of the world position alone, so neighbouring tiles agree along their shared edge;
// leaving out the domain's own square would step the field at every tile border. The dust the
// domain's own floor raises is then counted again by the routed dust, which on seed 42 is some six
// hundred times smaller than this field, so the double count is negligible.
float farDustAt(const Setting& setting, Vec2 position) {
    const Planet& planet = *setting.planet;
    double flux[DustSizes] = {}, depth[DustSizes] = {};   // m²/yr arriving, and optical depth of the way back
    Vec2 at = position;
    for (double travelled = 0; travelled < TraceReach; travelled += TraceStep) {
        const Vec2 point = planetPoint(setting.seed, at);
        const float rainfall = float(samplePlanet(planet.precipitation, point, Kernel::Linear));
        // The sea raises nothing, so there the rose of the regime is not needed, only its mean wind.
        float raised = 0;
        Vec2 mean;
        if (samplePlanet(planet.elevation, point, Kernel::Linear) <= planet.seaLevel) {
            mean = meanWindAtLatitude(latitudeAt(point.z), northwardZ(point.z));
        } else {
            const WindRegime regime = regimeAtPoint(setting.seed, point, setting.age);
            raised = emissionAt(planet, point, rainfall, regime.potential);
            mean = regime.mean;
        }
        const float rain = std::max(0.0f, rainfall);
        // Each stretch of the way gives what it raises less what settles before it has crossed the rest.
        for (int s = 0; s < DustSizes; ++s) {
            const double fall = (1.0 + rain / DustWashRain) / DustFall[s];
            if (raised != 0) flux[s] += double(raised * DustShare[s]) * std::exp(-depth[s]) * (1.0 - std::exp(-fall * TraceStep)) / fall;
            depth[s] += fall * TraceStep;
        }
        // Where the mean wind is calm the air goes nowhere in particular: it brings nothing from further off.
        const double speed = length(mean);
        if (speed < Calm) break;
        at = at - mean * (TraceStep / speed);
    }
    double settling = 0;
    for (int s = 0; s < DustSizes; ++s) settling += flux[s] / DustFall[s];
    return float(settling);
}

} // namespace

void readFarDust(Domain& domain, const Setting& setting) {
    WindField& wind = domain.aeolian;
    wind.farDust.assign(domain.cells(), 0.0f);
    const Planet* planet = setting.planet;
    if (!planet || planet->precipitation.cells.empty() || planet->temperature.cells.empty() || planet->elevation.cells.empty()) return;
    const int nodes = (domain.size - 1) / FarStride + 2;
    std::vector<float> node(size_t(nodes) * size_t(nodes));
    for (int nz = 0; nz < nodes; ++nz)
        for (int nx = 0; nx < nodes; ++nx)
            node[size_t(nz) * size_t(nodes) + size_t(nx)] = farDustAt(setting, {domain.origin.x + nx * FarStride * domain.cell, domain.origin.z + nz * FarStride * domain.cell});
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const double fx = (x + 0.5) / FarStride, fz = (z + 0.5) / FarStride;
            const int nx = int(fx), nz = int(fz);
            const double tx = fx - nx, tz = fz - nz;
            const auto at = [&](int ox, int oz) { return double(node[size_t(nz + oz) * size_t(nodes) + size_t(nx + ox)]); };
            wind.farDust[domain.index(x, z)] = float(mix(mix(at(0, 0), at(1, 0), tx), mix(at(0, 1), at(1, 1), tx), tz));
        }
}

} // namespace derivean::sim

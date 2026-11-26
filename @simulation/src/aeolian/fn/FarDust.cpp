#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"
#include "hydrology/constant/Water.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// Deserts raise dust in proportion to how seldom rain reaches their ground and how hard the wind
// blows over it: the Sahara gives up some 0.5–1 Gt a year from 9 million km², 0.04–0.07 mm of loess.
constexpr float DustRise = 5e-5f;       // m/yr of fines a hyper-arid floor gives up under DustDrift
constexpr float DustDrift = 30.0f;      // m²/yr of drift potential, that of a trade-wind desert
constexpr float DustWindCap = 2.0f;     // most a windier place multiplies that by
// The air is followed back along the annual mean wind until the coarser dust it could carry is gone.
constexpr double TraceStep = 30e3;      // m
constexpr double TraceReach = 2400e3;   // m, four fall lengths of the fine dust
constexpr double Calm = 0.3;            // m/s of mean wind below which the air is taken to stand
constexpr int FarStride = 64;           // cells between the points the far dust is read at

// m/yr of fines the ground at a world position gives up to the wind, read from the planet grid.
float emissionAt(const Planet& planet, uint64_t seed, Vec2 position, const WindRegime& regime) {
    const Vec2 point = planetPoint(seed, position);
    if (samplePlanet(planet.elevation, point, Kernel::Linear) <= planet.seaLevel) return 0;
    const float rain = float(samplePlanet(planet.precipitation, point, Kernel::Linear));
    const float warmth = float(samplePlanet(planet.temperature, point, Kernel::Linear));
    return DustRise * dustSource(rain, warmth) * std::min(DustWindCap, float(regime.potential) / DustDrift);
}

// m/yr of fines settling at `position` in dry air out of dust raised outside the domain's square.
float farDustAt(const Domain& domain, const Setting& setting, Vec2 position) {
    const Planet& planet = *setting.planet;
    const double low[2] = {domain.origin.x, domain.origin.z};
    const double high[2] = {low[0] + domain.size * domain.cell, low[1] + domain.size * domain.cell};
    double flux[DustSizes] = {}, depth[DustSizes] = {};   // m²/yr arriving, and optical depth of the way back
    Vec2 at = position;
    for (double travelled = 0; travelled < TraceReach; travelled += TraceStep) {
        const WindRegime regime = regimeAt(setting.seed, at, setting.age);
        const double speed = length(regime.mean);
        if (speed < Calm) break;
        const bool inside = at.x >= low[0] && at.x < high[0] && at.z >= low[1] && at.z < high[1];
        const float raised = inside ? 0.0f : emissionAt(planet, setting.seed, at, regime);
        const float rain = std::max(0.0f, float(samplePlanet(planet.precipitation, planetPoint(setting.seed, at), Kernel::Linear)));
        for (int s = 0; s < DustSizes; ++s) {
            flux[s] += double(raised * DustShare[s]) * TraceStep * std::exp(-depth[s]);
            depth[s] += TraceStep / DustFall[s] * (1.0 + rain / DustWashRain);
        }
        at = at - regime.mean * (TraceStep / speed);
    }
    double settling = 0;
    for (int s = 0; s < DustSizes; ++s) settling += flux[s] / DustFall[s];
    return float(settling);
}

} // namespace

float aridityOf(float rain, float celsius) { return std::clamp(1.0f - std::max(0.0f, rain) / evaporationDemand(celsius), 0.0f, 1.0f); }

float dustSource(float rain, float celsius) {
    // Dust comes from the driest ground almost alone.
    const float square = aridityOf(rain, celsius) * aridityOf(rain, celsius);
    return square * square;
}

void readFarDust(Domain& domain, const Setting& setting) {
    WindField& wind = domain.aeolian;
    wind.farDust.assign(domain.cells(), 0.0f);
    if (!setting.planet || setting.planet->precipitation.cells.empty()) return;
    const int nodes = domain.size / FarStride + 2;
    std::vector<float> node(size_t(nodes) * size_t(nodes));
    for (int nz = 0; nz < nodes; ++nz)
        for (int nx = 0; nx < nodes; ++nx)
            node[size_t(nz) * size_t(nodes) + size_t(nx)] = farDustAt(domain, setting, {domain.origin.x + nx * FarStride * domain.cell, domain.origin.z + nz * FarStride * domain.cell});
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

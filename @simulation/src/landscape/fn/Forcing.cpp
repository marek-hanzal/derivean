#include "landscape/fn/Forcing.hpp"

#include "landscape/fn/Drainage.hpp"

#include "climate/type/Climate.hpp"
#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "landscape/type/Level.hpp"
#include "landscape/fn/Process.hpp"
#include "landscape/type/Terrain.hpp"
#include "planet/fn/Sample.hpp"
#include "tectonics/fn/Structure.hpp"

#include <queue>

namespace derivean::sim {
namespace {

constexpr uint64_t WarpStream = 0x57415250;
constexpr uint64_t RoughStream = 0x524f5547;
constexpr double RangeContrast = 0.9;      // how unevenly the rise is shared between ranges and the basins between them
constexpr double BlockRate = 5.5e-5;       // m/yr of fault-block tilting next to a fast plate contact
constexpr double StressReach = 650e3;      // m over which a contact loads the plate interior

// Rivers of the planet grid: where each cell drains and how much water passes, so a region knows
// what enters across its edge from catchments it does not contain.
void routePlanet(const Planet& planet, Backdrop& backdrop) {
    const int n = PlanetCells;
    const size_t cells = planet.elevation.size();
    backdrop.discharge = Field<float>(n, n);
    backdrop.load = Field<float>(n, n);
    backdrop.graded = Field<float>(n, n, float(planet.seaLevel));
    backdrop.flowTo = Field<uint8_t>(n, n, uint8_t(255));
    std::vector<float> level(cells);
    std::vector<int32_t> order;
    std::vector<uint8_t> seen(cells, 0);
    using Entry = std::pair<float, int32_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    for (size_t i = 0; i < cells; ++i) {
        if (planet.elevation.cells[i] > planet.seaLevel) continue;
        level[i] = float(planet.seaLevel);
        seen[i] = 1;
        open.push({level[i], int32_t(i)});
    }
    while (!open.empty()) {
        const int32_t cell = open.top().second;
        open.pop();
        order.push_back(cell);
        for (int k = 0; k < 8; ++k) {
            const int nx = wrapIndex(cell % n + NeighborX[k], n), nz = wrapIndex(cell / n + NeighborZ[k], n);
            const size_t other = size_t(nz) * size_t(n) + size_t(nx);
            if (seen[other]) continue;
            seen[other] = 1;
            level[other] = std::max(planet.elevation.cells[other], std::nextafter(level[size_t(cell)], 1e9f));
            backdrop.flowTo.cells[other] = uint8_t((k + 4) % 8);
            open.push({level[other], int32_t(other)});
        }
    }
    for (size_t i = 0; i < cells; ++i) {
        backdrop.discharge.cells[i] = runoffOf(planet.precipitation.cells[i], planet.temperature.cells[i]) * float(PlanetCellArea);
        backdrop.load.cells[i] = planet.erosion.cells[i] * float(PlanetCellArea);
    }
    for (size_t step = order.size(); step-- > 0;) {
        const int32_t cell = order[step];
        const uint8_t to = backdrop.flowTo.cells[size_t(cell)];
        if (to == 255) continue;
        const int nx = wrapIndex(cell % n + NeighborX[to], n), nz = wrapIndex(cell / n + NeighborZ[to], n);
        backdrop.discharge.at(nx, nz) += backdrop.discharge.cells[size_t(cell)];
        // Half of what a planet cell sheds is taken to settle on the way through the next one.
        backdrop.load.at(nx, nz) += 0.5f * backdrop.load.cells[size_t(cell)];
    }
    // Walking back up from the sea: the slope stream power needs to keep pace with the erosion
    // the planet records, for the water that passes. No river bed can lie below this profile.
    for (const int32_t cell : order) {
        const uint8_t to = backdrop.flowTo.cells[size_t(cell)];
        if (to == 255) continue;
        const int nx = wrapIndex(cell % n + NeighborX[to], n), nz = wrapIndex(cell / n + NeighborZ[to], n);
        const double pace = std::max(2e-6, double(planet.erosion.cells[size_t(cell)]));
        const double slope = std::min(0.02, pace / (4e-6 * std::pow(std::max(1.0, double(backdrop.discharge.cells[size_t(cell)])), StreamExponent)));
        backdrop.graded.cells[size_t(cell)] = std::min(planet.elevation.cells[size_t(cell)],
                                                       float(backdrop.graded.at(nx, nz) + slope * PlanetCell * NeighborReach[to]));
    }
}

} // namespace

Backdrop readBackdrop(const Planet& planet, double age, Workers* workers) {
    Backdrop backdrop;
    const int n = PlanetCells;
    backdrop.regionStart = std::min(windowAt(Level::Region, age).start, planet.time);
    backdrop.pastElevation = Field<float>(n, n);
    backdrop.thickening = Field<float>(n, n);
    backdrop.crust = Field<float>(n, n);
    backdrop.residual = Field<float>(n, n);
    const size_t mark = size_t(pastMarkIndex(backdrop.regionStart));
    const double years = planet.time - backdrop.regionStart;
    for (size_t i = 0; i < planet.columns.size(); ++i) {
        const CrustColumn& column = planet.columns.cells[i];
        const PastMark& past = planet.records.cells[i].past[mark];
        backdrop.pastElevation.cells[i] = years > 0 ? past.elevation : planet.elevation.cells[i];
        backdrop.crust.cells[i] = column.crust;
        if (years <= 0) continue;
        const double gain = column.tectonicGain - past.gain;
        const double removed = (column.eroded - past.eroded) - (column.deposited - past.deposited);
        backdrop.thickening.cells[i] = float(gain / years);
        backdrop.residual.cells[i] = float((planet.elevation.cells[i] - past.elevation - IsostaticRise * (gain - removed)) / years);
    }
    routePlanet(planet, backdrop);
    backdrop.drainage = drainPlanet(planet, backdrop, workers);
    return backdrop;
}

Vec2 planetPoint(uint64_t seed, Vec2 position) {
    // Two scales of offset, each gentle enough (gradient below one) that the mapping never folds.
    const uint64_t base = stream(seed, WarpStream);
    const double u = position.x / 1500e3, v = position.z / 1500e3, s = position.x / 520e3, t = position.z / 520e3;
    return {position.x + 120e3 * fractalNoise(base + 3, u, v, 2) + 40e3 * fractalNoise(base + 1, s, t, 3),
            position.z + 120e3 * fractalNoise(base + 4, u, v, 2) + 40e3 * fractalNoise(base + 2, s, t, 3)};
}

Vec2 worldPoint(uint64_t seed, Vec2 planetPosition) {
    Vec2 position = planetPosition;
    for (int round = 0; round < 6; ++round) position = position - (planetPoint(seed, position) - planetPosition);
    return position;
}

float fractureAt(uint64_t seed, Vec2 material, double strike, double spacing) {
    const double c = std::cos(strike), s = std::sin(strike);
    const double along = (material.x * c + material.z * s) / (spacing * 1.7), across = (-material.x * s + material.z * c) / spacing;
    const CellularSite site = cellularNoise(stream(seed, RoughStream) + uint64_t(spacing), along, across, 0.95);
    return float(1.0 - smoothstep(0.0, 0.14, site.second - site.nearest));
}

float roughnessAt(uint64_t seed, Vec2 material, double wavelength) {
    return float(fractalNoise(stream(seed, RoughStream) + 3, material.x / wavelength, material.z / wavelength, 4));
}

float qualityAt(uint64_t seed, Vec2 material, double wavelength) {
    const uint64_t base = stream(seed, RoughStream) + 17;
    const double broad = fractalNoise(base, material.x / wavelength, material.z / wavelength, 3, 0.6);
    const double fine = fractalNoise(base + 1, material.x / (wavelength * 0.22), material.z / (wavelength * 0.22), 2);
    return float(std::exp(0.75 * broad + 0.35 * fine));
}

double seaLevelAt(const Setting& setting, double time) {
    const Planet& planet = *setting.planet;
    if (planet.seaLevels.empty() || time >= planet.time) return planet.seaLevel;
    const double at = std::max(0.0, (time - TectonicsStart) / PlanetStep - 1.0);
    const size_t low = std::min(planet.seaLevels.size() - 1, size_t(at));
    const size_t high = std::min(planet.seaLevels.size() - 1, low + 1);
    return mix(planet.seaLevels[low], planet.seaLevels[high], at - double(low));
}

Forcing forcingAt(const Setting& setting, Vec2 position) {
    const Planet& planet = *setting.planet;
    const Backdrop& backdrop = *setting.backdrop;
    const Vec2 point = planetPoint(setting.seed, position);
    Forcing forcing;
    forcing.strata = strataAt(planet, point);
    const Structure structure = structureAt(setting.seed, forcing.strata.material, forcing.strata.fabric);
    // Thickened crust floats higher as a whole; the upper crust takes that rise up unevenly, range
    // by range and graben by graben. Thin oceanic lids have no such structure.
    const double thick = samplePlanet(backdrop.thickening, point);
    const double continental = smoothstep(OceanicLimit, ContinentalLimit, samplePlanet(backdrop.crust, point, Kernel::Linear));
    // Stretching is shared less unevenly than stacking: a graben sinks at most 1.6 times the mean.
    const double pattern = thick >= 0 ? RangeContrast * structure.ranges : 0.5 * std::min(1.2, structure.rifts);
    forcing.tectonic = float(IsostaticRise * thick * (1.0 + continental * pattern));

    // Plate interiors within reach of a working contact break into blocks that tilt against each other.
    const ContactSample contact = contactAt(planet, point);
    const double stress = std::min(1.0, (std::abs(contact.convergence) + contact.shear) / 0.02) *
                          std::exp(-contact.distance / StressReach) * epochActivity(planet.epoch, planet.time);
    forcing.residual = float(samplePlanet(backdrop.residual, point) + BlockRate * stress * continental * structure.blocks);
    forcing.weakness = float(structure.weakness);

    // The steady climate leaves the driest ground with no rain at all; every real desert still
    // sees the odd storm, and over millions of years those storms are what shapes it.
    const double trace = planet.surfaceWater > 0 ? 0.04 : 0.0;
    forcing.rain = float(std::max(trace, samplePlanet(planet.precipitation, point, Kernel::Linear)));
    const double ground = std::max(0.0, samplePlanet(planet.elevation, point, Kernel::Linear) - planet.seaLevel);
    forcing.warmth = float(samplePlanet(planet.temperature, point, Kernel::Linear) + LapseRate * ground);
    forcing.windX = float(samplePlanet(planet.windX, point, Kernel::Linear));
    forcing.windZ = float(samplePlanet(planet.windZ, point, Kernel::Linear));
    return forcing;
}

} // namespace derivean::sim

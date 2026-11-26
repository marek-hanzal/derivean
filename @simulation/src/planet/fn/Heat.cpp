#include "planet/fn/Heat.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t CentreStream = 0x48454154;
constexpr double CentreSpacing = 46e3;        // m between magmatic centres along an active belt
constexpr double MeltCooling = 3 * Myr;       // e-folding time of a crustal magma body's heat
constexpr float ContinentalGradient = 0.022f; // K/m through old felsic crust
constexpr float RidgeGradient = 0.17f;        // K/m through a lid one Myr old; falls with the root of its age
constexpr float MagmaticGradient = 0.26f;     // K/m added directly above a body that is still molten
constexpr float BetweenCentres = 0.10f;       // share of the magmatic heat that reaches ground far from a centre

constexpr float MeltForHeat = 600.0f;         // m of added crust that makes a full-strength heat source

// Crust-attached coordinates of a position: where the rock under it was when its columns formed.
Vec2 crustFrame(const Planet& planet, Vec2 position) {
    const ColumnBlend around = blendColumns(planet, position);
    double driftX = 0, driftZ = 0;
    for (int corner = 0; corner < 4; ++corner) driftX += around.weight[corner] * around.column[corner]->driftX, driftZ += around.weight[corner] * around.column[corner]->driftZ;
    return position - Vec2{driftX, driftZ} * PlanetCell;
}

CellularSite centreSite(const Planet& planet, Vec2 material) {
    return cellularNoise(stream(planet.seed, CentreStream), material.x / CentreSpacing, material.z / CentreSpacing, 0.85);
}

float centreVigor(const Planet& planet, const CellularSite& site) {
    return float(ranged(hash(stream(planet.seed, CentreStream) + 1, site.cellX, site.cellZ), 0.35, 1.0));
}

} // namespace

MagmaticCentre centreNear(const Planet& planet, Vec2 position) {
    const Vec2 material = crustFrame(planet, position);
    const CellularSite site = centreSite(planet, material);
    return {position + (site.site * CentreSpacing - material), site.cellX, site.cellZ, centreVigor(planet, site)};
}

// Crust is handed between cells whole, so neighbouring columns can differ by a whole step of melt.
// The heat of a belt is that of its columns spread over the belt's width, not of one column.
void refreshMeltHeat(Planet& planet) {
    const int n = PlanetCells;
    Field<float> raw(n, n);
    for (size_t cell = 0; cell < raw.size(); ++cell) {
        const CrustColumn& column = planet.columns.cells[cell];
        if (column.meltTime < 0) continue;
        // Melt that rose during the planet's last step is the present-day activity.
        const double age = std::max(0.0, planet.time - double(column.meltTime) - PlanetStep);
        raw.cells[cell] = float(std::exp(-age / MeltCooling)) * std::min(1.0f, column.melt / MeltForHeat);
    }
    planet.meltHeat = Field<float>(n, n);
    constexpr float Kernel[3] = {0.25f, 0.5f, 0.25f};
    Field<float> rows(n, n);
    for (int z = 0; z < n; ++z)
        for (int x = 0; x < n; ++x)
            for (int k = -1; k <= 1; ++k) rows.at(x, z) += Kernel[k + 1] * raw.at(wrapIndex(x + k, n), z);
    for (int z = 0; z < n; ++z)
        for (int x = 0; x < n; ++x)
            for (int k = -1; k <= 1; ++k) planet.meltHeat.at(x, z) += Kernel[k + 1] * rows.at(x, wrapIndex(z + k, n));
}

Geotherm geothermAt(const Planet& planet, Vec2 position) {
    Geotherm geotherm;
    if (planet.time < CrustStable) {
        geotherm.gradient = 1.0f;
        geotherm.magmatic = geotherm.centre = 1.0f;
        return geotherm;
    }
    const ColumnBlend around = blendColumns(planet, position);
    // A planet built by hand for a test carries no melt record.
    const double heat = planet.meltHeat.size() == 0 ? 0.0 : std::max(0.0, samplePlanet(planet.meltHeat, position));
    double conductive = 0, since = 0;
    for (int corner = 0; corner < 4; ++corner) {
        const CrustColumn& column = *around.column[corner];
        const double weight = around.weight[corner];
        const double lidMyr = std::max(1.0, double(column.oceanAge) / Myr);
        conductive += weight * (column.crust < float(OceanicLimit) ? std::max(double(ContinentalGradient), RidgeGradient / std::sqrt(lidMyr))
                                                                    : double(ContinentalGradient));
        since += weight * (column.meltTime < 0 ? planet.time : std::max(0.0, planet.time - double(column.meltTime) - PlanetStep));
    }
    // Centres sit on a jittered lattice in crust-attached coordinates.
    const CellularSite site = centreSite(planet, crustFrame(planet, position));
    const double closeness = 1.0 - smoothstep(0.06, 0.42, site.nearest);
    const double vigor = centreVigor(planet, site);
    const double focus = BetweenCentres + (1.0 - BetweenCentres) * closeness * closeness * vigor;
    const double magmatic = MagmaticGradient * heat * focus;
    geotherm.gradient = float(conductive + magmatic);
    geotherm.magmatic = float(magmatic / (conductive + magmatic));
    geotherm.sinceMelt = float(since);
    geotherm.centre = float(closeness * vigor);
    return geotherm;
}

} // namespace derivean::sim

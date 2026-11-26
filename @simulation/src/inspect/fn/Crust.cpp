#include "inspect/fn/Inspect.hpp"

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {
namespace {

constexpr double QuietRate = 0.0005;   // m/yr of relative plate motion below which a contact does nothing visible

} // namespace

InspectCrust inspectCrust(const Planet& planet, uint64_t seed, Vec2 position) {
    InspectCrust crust;
    // Crust and plates are read where the landscape reads the planet.
    const Vec2 point = planetPoint(seed, position);
    const CrustColumn& column = planetColumn(planet, point);
    const size_t cell = size_t(&column - planet.columns.cells.data());
    crust.elevation = float(samplePlanet(planet.elevation, point));
    crust.seaLevel = planet.seaLevel;
    crust.basement = column.basement;
    crust.thickness = column.crust;
    crust.stock = column.crust - float(CrustAtDatum);
    crust.cover = coverThickness(planet.records.cells[cell].cover);
    crust.lidAge = column.oceanAge;
    crust.sinceDeformation = planet.time - column.quietSince;
    crust.sinceOrogeny = column.orogenTime < 0 ? -1.0 : planet.time - column.orogenTime;
    crust.exhumed = column.exhumed;
    crust.erodedTotal = column.eroded;
    crust.depositedTotal = column.deposited;
    crust.erosionRate = planet.erosion.cells[cell];
    crust.depositionRate = planet.deposition.cells[cell];
    crust.drifted = Vec2{double(column.driftX), double(column.driftZ)} * PlanetCell;

    // Plates are the planet's live outlines: each cell rides on its owner, and its contact record
    // points at the nearest cell of another plate.
    const int cellX = wrapIndex(floorToInt(point.x / PlanetCell), PlanetCells), cellZ = wrapIndex(floorToInt(point.z / PlanetCell), PlanetCells);
    const ContactCell& nearest = planet.contact.at(cellX, cellZ);
    const ContactSample contact = contactAt(planet, point);
    crust.activity = epochActivity(planet.epoch, planet.time);
    crust.plate = planet.owner.at(cellX, cellZ);
    crust.neighbor = crust.plate;
    if (nearest.distance < 2 * ContactReach) {
        const double reach = nearest.distance / PlanetCell + 1.0;
        crust.neighbor = planet.owner.at(wrapIndex(cellX + int(std::lround(nearest.normalX * reach)), PlanetCells),
                                         wrapIndex(cellZ + int(std::lround(nearest.normalZ * reach)), PlanetCells));
    }
    crust.velocity = planet.epoch.plates[size_t(crust.plate)].velocity * crust.activity;
    crust.contactDistance = contact.distance;
    const double closing = contact.convergence * crust.activity;
    crust.compression = float(std::max(0.0, closing));
    crust.extension = float(std::max(0.0, -closing));
    crust.shear = float(contact.shear * crust.activity);
    const float strongest = std::max({crust.compression, crust.extension, crust.shear});
    crust.regime = strongest < QuietRate ? Regime::Quiet
                 : strongest == crust.compression ? Regime::Compression
                 : strongest == crust.extension ? Regime::Extension : Regime::Shear;
    return crust;
}

} // namespace derivean::sim

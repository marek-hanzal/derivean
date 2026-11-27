#include "planet/fn/FlatView.hpp"

#include "climate/type/Climate.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Frame.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Sample.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double DegreesToRadians = Pi / 180;
// Zonal.cpp: z = 0 sits at 48° north and north is towards negative z there.
constexpr double PoleZ = -(90.0 - 48.0) / 360.0 * PlanetSpan;

double wrapped(double value) { return value - std::floor(value / PlanetSpan) * PlanetSpan; }

int cellUnder(const Globe& globe, Vec2 flat) { return indexOf(*globe.grid, cellOf(globeDirectionOf(flat), globe.grid->level)); }

} // namespace

Vec3 globeDirectionOf(Vec2 flat) {
    const double phi = latitudeAt(flat.z) * DegreesToRadians;
    const double lambda = 2 * Pi * flat.x / PlanetSpan;
    return {std::cos(phi) * std::cos(lambda), std::cos(phi) * std::sin(lambda), std::sin(phi)};
}

Vec2 flatPositionOf(Vec3 direction) {
    const Vec3 u = normalized(direction);
    const double latitude = std::asin(std::clamp(u.z, -1.0, 1.0)) / DegreesToRadians;
    const double longitude = std::atan2(u.y, u.x);
    return {wrapped(longitude / (2 * Pi) * PlanetSpan), wrapped(PoleZ + (90.0 - latitude) / 360.0 * PlanetSpan)};
}

BridgeJacobian bridgeJacobianAt(double z) {
    return {BridgeScale * std::cos(latitudeAt(z) * DegreesToRadians), BridgeScale * northwardZ(z)};
}

Vec2 flatVector(Vec2 at, Vec3 v) {
    const SurfaceFrame frame = geographicFrame(globeDirectionOf(at));
    const BridgeJacobian j = bridgeJacobianAt(at.z);
    return {dot(v, frame.first) / j.east, dot(v, frame.second) / j.north};
}

Vec2 flatNormal(Vec2 at, Vec3 n) {
    const SurfaceFrame frame = geographicFrame(globeDirectionOf(at));
    const BridgeJacobian j = bridgeJacobianAt(at.z);
    return normalized(Vec2{dot(n, frame.first) * j.east, dot(n, frame.second) * j.north});
}

float flatStrikeOf(Vec3 direction, Vec3 n) {
    const Vec2 flat = flatNormal(flatPositionOf(direction), n);
    return float(std::atan2(flat.x, -flat.z));
}

Planet flatViewOf(const Globe& globe) {
    const int n = PlanetCells;
    Planet planet;
    planet.seed = globe.seed;
    planet.time = globe.time;
    planet.columns = Field<CrustColumn>(n, n);
    planet.records = Field<ColumnRecord>(n, n);
    for (Field<float>* field : {&planet.elevation, &planet.swell, &planet.meltHeat, &planet.erosion, &planet.deposition, &planet.temperature,
                                &planet.precipitation, &planet.windX, &planet.windZ})
        *field = Field<float>(n, n);
    planet.owner = Field<uint16_t>(n, n);
    planet.contact = Field<ContactCell>(n, n);
    planet.epoch = epochAt(globe.seed, globe.time);
    const size_t plates = globe.epoch.plates.size();
    planet.epoch.plates.resize(plates);
    planet.motion.assign(plates, PlateMotion{});
    for (size_t p = 0; p < plates && p < globe.motion.size(); ++p) planet.motion[p].resistance = globe.motion[p].resistance / PlanetCellArea;

    std::vector<Vec2> velocity(plates);
    std::vector<double> count(plates, 0.0);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const Vec2 at = planetCellCenter(x, z);
            const int cell = cellUnder(globe, at);
            const int piece = dominantPiece(globe.coupling, cell);
            const size_t i = planet.columns.index(x, z);
            if (piece >= 0) {
                const CrustPiece& crust = globe.pieces[size_t(piece)];
                planet.columns.cells[i] = crust.column;
                planet.records.cells[i] = crust.record;
                planet.owner.cells[i] = crust.plate;
                if (crust.plate < plates && crust.plate < globe.applied.size()) {
                    // By the sphere's area each flat cell stands for (cos φ), so the rows near the poles do not
                    // outweigh the plate's middle.
                    const double weight = bridgeJacobianAt(at.z).east / BridgeScale;
                    velocity[crust.plate] = velocity[crust.plate] + flatVector(at, velocityAt(globe.applied[crust.plate], globeDirectionOf(at))) * weight;
                    count[crust.plate] += weight;
                }
            }
            planet.swell.cells[i] = globe.swell.value[size_t(cell)];
            // The cell's view, on which the sea was solved, so the view's shore is the globe's.
            planet.elevation.cells[i] = globe.elevation.value[size_t(cell)];
            planet.meltHeat.cells[i] = globe.meltHeat.value[size_t(cell)];
            planet.erosion.cells[i] = globe.erosion.value[size_t(cell)];
            planet.deposition.cells[i] = globe.deposition.value[size_t(cell)];
            planet.temperature.cells[i] = globe.temperature.value[size_t(cell)];
            planet.precipitation.cells[i] = globe.precipitation.value[size_t(cell)];
            const GlobeContact& contact = globe.contact.value[size_t(cell)];
            if (contact.across < 0) continue;
            ContactCell& flat = planet.contact.cells[i];
            // The way to the contact, along its normal, is a displacement: J⁻¹ takes it to flat metres.
            flat.distance = float(length(flatVector(at, contact.normal * double(contact.distance))));
            flat.convergence = contact.convergence;
            flat.shear = contact.shear;
            const Vec2 normal = flatNormal(at, contact.normal);
            flat.normalX = float(normal.x), flat.normalZ = float(normal.z);
            if (flat.distance < ContactReach) planet.contactZone.push_back(uint32_t(i));
        }
    }
    for (size_t p = 0; p < plates; ++p) planet.epoch.plates[p].velocity = count[p] > 0 ? velocity[p] * (1.0 / count[p]) : Vec2{};

    planet.seaLevels = globe.seaLevels;
    planet.seaLevel = globe.seaLevel;
    planet.surfaceWater = globe.surfaceWater;
    planet.surfaceKelvin = globe.surfaceKelvin;
    const GlobeLedger& ledger = globe.ledger;
    planet.ledger.eroded = ledger.eroded, planet.ledger.deposited = ledger.deposited, planet.ledger.subducted = ledger.subducted;
    planet.ledger.accreted = ledger.accreted, planet.ledger.erupted = ledger.erupted, planet.ledger.precipitated = ledger.precipitated;
    planet.ledger.waterInventory = ledger.waterInventory;
    return planet;
}

void readClimateBack(Globe& globe, const Planet& view) {
    for (size_t cell = 0; cell < globe.temperature.value.size(); ++cell) {
        const Vec2 at = flatPositionOf(globe.grid->centre[cell]);
        globe.temperature.value[cell] = float(samplePlanet(view.temperature, at, Kernel::Linear));
        globe.precipitation.value[cell] = float(std::max(0.0, samplePlanet(view.precipitation, at, Kernel::Linear)));
    }
}

} // namespace derivean::sim

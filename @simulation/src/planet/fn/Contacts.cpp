#include "planet/type/Planet.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr double ArcYield = 1660.0;            // m² of new crust per m of plate consumed, per m of contact
constexpr double ArcOffset = 130e3;            // m behind the contact where the melt rises
constexpr double ArcWidth = 70e3;
constexpr double ArcNorm = 124e3;               // integral of the arc profile across the belt
constexpr float ArcSurfaceShare = 0.35f;       // share of an arc's melt that reaches the surface as lava and ash
constexpr float ArcCeiling = 44000.0f;         // m of crust above which rising melt stalls in the root
constexpr double HotspotEra = 150 * Myr;
constexpr int HotspotCount = 12;
constexpr float HotspotGain = 1800.0f;         // m of crust per planet step directly above a plume
constexpr uint64_t HotspotStream = 0x484f5453;
constexpr double SpreadStart = 50000.0;        // m of crust above which the pile flows under its own weight
constexpr double SpreadRange = 14000.0;

bool oceanic(const CrustColumn& column) { return column.crust < float(OceanicLimit); }

const CrustColumn& across(const Planet& planet, int x, int z, const ContactCell& contact) {
    const double reach = (2.0 * contact.distance + PlanetCell) / PlanetCell;
    const int ox = wrapIndex(x + int(std::lround(contact.normalX * reach)), PlanetCells);
    const int oz = wrapIndex(z + int(std::lround(contact.normalZ * reach)), PlanetCells);
    return planet.columns.at(ox, oz);
}

void erupt(Planet& planet, size_t cell, float gain, Facies facies, float layer, MeltSource source) {
    CrustColumn& column = planet.columns.cells[cell];
    column.crust += gain;
    column.tectonicGain += gain;
    column.quietSince = column.meltTime = float(planet.time);
    column.melt += gain;
    column.erupted += layer;
    column.meltSource = source;
    addDeposit(planet.records.cells[cell].cover, planet.time, layer, facies);
    planet.ledger.erupted += double(gain) * PlanetCellArea;
}

} // namespace

void actOnContacts(Planet& planet, double dt) {
    const double activity = epochActivity(planet.epoch, planet.time);
    for (const uint32_t cell : planet.contactZone) {
        const ContactCell& contact = planet.contact.cells[cell];
        CrustColumn& column = planet.columns.cells[cell];
        const double closing = contact.convergence * activity;
        const int x = int(cell % PlanetCells), z = int(cell / PlanetCells);
        if (closing > 0.001) {
            if (column.crust > ArcCeiling) continue;
            const CrustColumn& other = across(planet, x, z, contact);
            // The denser side sinks: ocean under continent, old ocean under young ocean.
            const bool overrides = oceanic(other) && (!oceanic(column) || column.oceanAge < other.oceanAge);
            if (!overrides) continue;
            const double offset = (contact.distance - ArcOffset) / ArcWidth;
            const float gain = float(ArcYield * closing * dt * std::exp(-offset * offset) / ArcNorm);
            if (gain < 1.0f) continue;
            erupt(planet, cell, gain, Facies{0, 25, 30, 0, 200, 0}, ArcSurfaceShare * gain, MeltSource::Arc);
            if (column.basement != Basement::Craton && column.basement != Basement::Orogen) column.basement = Basement::Arc;
            column.fabric = std::atan2(contact.normalX, -contact.normalZ);
        } else if (closing < -0.002 && contact.distance < PlanetCell && !oceanic(column)) {
            // Stretched continent leaks basalt along the opening contact.
            erupt(planet, cell, 40.0f, Facies{0, 0, 15, 0, 240, 0}, 40.0f, MeltSource::Rift);
            column.fabric = std::atan2(contact.normalX, -contact.normalZ);
        }
    }

    // Plumes stay put in the mantle frame for an era while the plates carry their products away.
    const int64_t era = int64_t(planet.time / HotspotEra);
    const uint64_t seed = stream(planet.seed, HotspotStream);
    for (int spot = 0; spot < HotspotCount; ++spot) {
        const int x = int(hash(seed, era, spot, 1) % PlanetCells);
        const int z = int(hash(seed, era, spot, 2) % PlanetCells);
        const float vigor = float(ranged(hash(seed, era, spot, 3), 0.4, 1.0));
        erupt(planet, planet.columns.index(x, z), HotspotGain * vigor, Facies{0, 0, 10, 0, 245, 0}, 300.0f * vigor, MeltSource::Plume);
    }
}

void relaxThickCrust(Planet& planet, double dt) {
    // Explicit conservative diffusion whose mobility rises with the excess thickness, so a narrow
    // stack widens into a plateau instead of growing without bound. Stable for a number <= 0.24.
    const double base = 0.24 * dt / PlanetStep;
    const int n = PlanetCells;
    std::vector<float> mobility(planet.columns.size());
    bool any = false;
    for (size_t cell = 0; cell < mobility.size(); ++cell) {
        const double excess = std::clamp((double(planet.columns.cells[cell].crust) - SpreadStart) / SpreadRange, 0.0, 1.0);
        mobility[cell] = float(excess * excess);
        any = any || excess > 0;
    }
    if (!any) return;
    std::vector<float> change(planet.columns.size(), 0.0f);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const size_t cell = size_t(z) * n + size_t(x);
            const size_t east = size_t(z) * n + size_t(x + 1 == n ? 0 : x + 1);
            const size_t south = size_t(z + 1 == n ? 0 : z + 1) * n + size_t(x);
            for (const size_t other : {east, south}) {
                const float rate = std::max(mobility[cell], mobility[other]);
                if (rate <= 0) continue;
                const float flow = float(base) * rate * (planet.columns.cells[cell].crust - planet.columns.cells[other].crust);
                change[cell] -= flow;
                change[other] += flow;
            }
        }
    }
    for (size_t cell = 0; cell < change.size(); ++cell) {
        if (change[cell] == 0) continue;
        planet.columns.cells[cell].crust += change[cell];
        planet.columns.cells[cell].tectonicGain += change[cell];
    }
}

} // namespace derivean::sim

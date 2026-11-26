#include "measure/fn/Seek.hpp"

#include "landscape/fn/Forcing.hpp"
#include "measure/fn/Place.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Spawn.hpp"

namespace derivean::sim {
namespace {

constexpr double Lattice = 32768.0;   // m between the samples of the planet-wide search
constexpr double Unfit = -1e30;
constexpr int Patch = TileCells / Refinement;   // region cells under one Local tile, local cells under one Site tile

struct Spot {
    Vec2 at, point;    // world position and where it reads the planet
    double height{};   // m above the sea
    double relief{};   // m between the highest and lowest of the spot and four points 32 km away
    double rain{}, warmth{}, flow{};
};

bool cliffFormer(Material rock) { return rock == Material::Sandstone || rock == Material::Limestone || rock == Material::Conglomerate; }

// How well a spot shows a kind of country; Unfit where it does not show it at all.
double fitness(const World& world, Country kind, const Spot& spot, std::string& why) {
    const Planet& planet = world.planet;
    const auto rocks = [&](double least, auto&& accept) {
        if (spot.height < least) return false;
        const StrataColumn column = strataAt(planet, spot.point);
        return accept(column, unitAt(column, 5.0f, 0.0f));
    };
    char text[160];
    switch (kind) {
        case Country::HumidRange: return spot.rain >= 0.9 && spot.warmth >= 2 ? spot.height : Unfit;
        case Country::ColdRange: return spot.warmth <= -3 && spot.rain >= 0.3 ? spot.height : Unfit;
        case Country::DryPlateau: return spot.rain <= 0.3 && spot.height >= 700 && spot.relief <= 500 && spot.flow >= 2e9 ? 300 * std::log10(spot.flow) + spot.height : Unfit;
        case Country::Lowland:
            return spot.height >= 60 && spot.height <= 300 && spot.relief <= 120 && spot.rain >= 0.5 && spot.rain <= 1.3 && spot.warmth >= 4 && spot.warmth <= 20
                       ? -std::abs(spot.height - 150) - spot.relief : Unfit;
        case Country::OldRange: {
            double age = 0;
            if (spot.height > 1800 || spot.relief < 150 || spot.rain < 0.5 || !rocks(300, [&](const StrataColumn& column, const StrataUnit&) {
                    age = column.orogenTime > 0 ? column.now - column.orogenTime : 0;
                    return age >= 250 * Myr;
                })) return Unfit;
            std::snprintf(text, sizeof text, "last stacked %.0f Myr ago", age / Myr);
            why = text;
            return std::min(age, 800 * Myr) / Myr + spot.relief / 4;
        }
        case Country::FlatBeds: {
            double thickness = 0;
            if (spot.rain < 0.3 || !rocks(400, [&](const StrataColumn& column, const StrataUnit& unit) {
                    int strong = 0;
                    for (int bed = 0; bed < 10; ++bed) strong += cliffFormer(lithified(unit.facies, (bed + 0.5) / 10));
                    thickness = unit.bottom - unit.top;
                    return unit.slot >= 0 && unit.laidDown > column.deformed && thickness >= 250 && strong >= 6;
                })) return Unfit;
            std::snprintf(text, sizeof text, "%.0f m of unfolded cliff-forming cover at the top", thickness);
            why = text;
            return std::min(thickness, 800.0) + 0.5 * spot.height + spot.relief;
        }
        case Country::FoldBelt:
            if (spot.rain < 0.4 || !rocks(500, [&](const StrataColumn& column, const StrataUnit& unit) {
                    return column.basement == Basement::Orogen && column.foldSpan >= 300 && unit.slot != BasementUnit && unit.laidDown < column.deformed && unit.bottom - unit.top >= 300;
                })) return Unfit;
            return spot.height + spot.relief;
        case Country::Granite:
            if (spot.rain < 0.4 || !rocks(600, [&](const StrataColumn& column, const StrataUnit& unit) {
                    return bedAt(planet.seed, column, unit, 5.0f, column.material).rock == Material::Granite;
                })) return Unfit;
            return spot.height + spot.relief;
        default: return Unfit;
    }
}

// What a 16 km window of a Region tile is worth for a kind, from its cells.
double windowWorth(Country kind, const Tile& tile, int wx, int wz) {
    float low = 1e30f, high = -1e30f, flow = 0;
    int dry = 0, granite = 0, strong = 0;
    for (int z = wz * Patch; z < (wz + 1) * Patch; ++z)
        for (int x = wx * Patch; x < (wx + 1) * Patch; ++x) {
            const size_t i = size_t(z) * TileCells + size_t(x);
            const float ground = tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i];
            if (tile.water[i] > ground || ground < tile.seaLevel) continue;
            ++dry, low = std::min(low, ground), high = std::max(high, ground), flow = std::max(flow, tile.discharge[i]);
            granite += Material(tile.rock[i]) == Material::Granite, strong += cliffFormer(Material(tile.rock[i]));
        }
    if (dry < Patch * Patch * 9 / 10) return Unfit;
    const double relief = high - low;
    switch (kind) {
        case Country::DryPlateau: return flow >= 1e9 ? 200 * std::log10(flow) + relief : Unfit;
        case Country::Granite: return granite * 2 >= dry ? relief : Unfit;
        case Country::FlatBeds: return strong * 2 >= dry ? relief : Unfit;
        case Country::Lowland: return -relief;
        default: return relief;
    }
}

} // namespace

Sought seekCountry(World& world, Country kind) {
    const Planet& planet = world.planet;
    Sought sought;
    if (kind == Country::Coast) {
        sought.planet = findShore(planet, {0, 0});
        sought.local = coastNear(world, sought.planet);
        sought.found = true, sought.why = "the shore nearest the origin with country behind it";
        return sought;
    }
    const int side = int(PlanetSpan / Lattice);
    const auto heightAt = [&](Vec2 at) { return samplePlanet(planet.elevation, planetPoint(planet.seed, at)) - planet.seaLevel; };
    double best = Unfit;
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            Spot spot;
            spot.at = {(x + 0.5) * Lattice, (z + 0.5) * Lattice};
            spot.point = planetPoint(planet.seed, spot.at);
            spot.height = samplePlanet(planet.elevation, spot.point) - planet.seaLevel;
            if (spot.height < 20) continue;
            double low = spot.height, high = spot.height;
            for (int k = 0; k < 8; k += 2) {
                const double other = heightAt(spot.at + Vec2{double(NeighborX[k]), double(NeighborZ[k])} * Lattice);
                low = std::min(low, other), high = std::max(high, other);
            }
            if (low < 0) continue;   // not on a coast: the sea would take half the tiles
            spot.relief = high - low;
            spot.rain = samplePlanet(planet.precipitation, spot.point, Kernel::Linear);
            spot.warmth = samplePlanet(planet.temperature, spot.point, Kernel::Linear);
            spot.flow = world.backdrop.discharge.at(wrapIndex(floorToInt(spot.point.x / PlanetCell), PlanetCells), wrapIndex(floorToInt(spot.point.z / PlanetCell), PlanetCells));
            std::string why;
            const double fit = fitness(world, kind, spot, why);
            if (fit <= best) continue;
            best = fit, sought.found = true, sought.planet = spot.at;
            char text[200];
            std::snprintf(text, sizeof text, "%.0f m above the sea, %.0f m of relief over 64 km, rain %.2f m/yr, %.1f C, %.3g m3/yr through the planet cell%s%s", spot.height,
                          spot.relief, spot.rain, spot.warmth, spot.flow, why.empty() ? "" : ", ", why.c_str());
            sought.why = text;
        }
    }
    if (!sought.found) return sought;

    // The 16 km of the Region tile around the sample that show the kind best, within 60 km of it.
    const TileKey key = tileAt(Level::Region, sought.planet);
    const auto tile = world.atlas.require(key);
    const double cell = specOf(Level::Region).cell;
    const int centreX = int(sought.planet.x / cell) - key.x * TileCells, centreZ = int(sought.planet.z / cell) - key.z * TileCells;
    const int reach = int(60e3 / cell / Patch);
    double worth = Unfit;
    sought.local = sought.planet;
    for (int wz = std::max(1, centreZ / Patch - reach); wz <= std::min(TileCells / Patch - 2, centreZ / Patch + reach); ++wz)
        for (int wx = std::max(1, centreX / Patch - reach); wx <= std::min(TileCells / Patch - 2, centreX / Patch + reach); ++wx) {
            const double here = windowWorth(kind, *tile, wx, wz);
            if (here <= worth) continue;
            worth = here;
            sought.local = {(double(key.x) * TileCells + (wx + 0.5) * Patch) * cell, (double(key.z) * TileCells + (wz + 0.5) * Patch) * cell};
        }
    return sought;
}

Vec2 steepestSite(World& world, Vec2 local) {
    const TileKey key = tileAt(Level::Local, local);
    const auto tile = world.atlas.require(key);
    const double cell = specOf(Level::Local).cell;
    Vec2 found = local;
    float most = -1;
    // Inner windows only, so the Site tiles around the chosen one have this Local tile as their parent.
    for (int wz = 2; wz < TileCells / Patch - 2; ++wz)
        for (int wx = 2; wx < TileCells / Patch - 2; ++wx) {
            float low = 1e30f, high = -1e30f;
            int dry = 0;
            for (int z = wz * Patch; z < (wz + 1) * Patch; ++z)
                for (int x = wx * Patch; x < (wx + 1) * Patch; ++x) {
                    const size_t i = size_t(z) * TileCells + size_t(x);
                    const float ground = tile->now.rock[i] + tile->now.sediment[0][i] + tile->now.sediment[1][i] + tile->now.sediment[2][i];
                    if (tile->water[i] > ground) continue;
                    ++dry, low = std::min(low, ground), high = std::max(high, ground);
                }
            if (dry < Patch * Patch * 9 / 10 || high - low <= most) continue;
            most = high - low;
            found = {(double(key.x) * TileCells + (wx + 0.5) * Patch) * cell, (double(key.z) * TileCells + (wz + 0.5) * Patch) * cell};
        }
    return found;
}

} // namespace derivean::sim

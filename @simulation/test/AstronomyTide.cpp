// The equilibrium tide: its planet-wide shape against the bodies' bulges, and over a partial ocean,
// where only the mask's own level correction keeps the water's volume.
#include "AstronomyFixture.hpp"

#include "astronomy/fn/Tide.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace derivean::sim;
using namespace derivean::sim::astronomytest;

namespace {

// Over the whole sphere the tide's mean is zero at every instant. A body raises a bulge
// A = GM R⁴ / (GM⊕ d³) under itself and lowers its rim by A/2, so the planet-wide range is 1.5 (A☾ + A☉)
// with the bodies in line and 1.5 A☾ with them at right angles; the degree-3 term and the bodies being
// a few degrees off line or square allow 3 %. The Moon alone raises 0.357 m at its mean distance.
void testTide() {
    const auto range = [](const Sky& sky) {
        constexpr int Points = 4000;
        double sum = 0, low = 1e9, high = -1e9;
        for (int i = 0; i < Points; ++i) {
            // Fibonacci points cover the sphere evenly (González 2010).
            const double z = 1 - (2 * i + 1.0) / Points, around = i * std::numbers::pi * (3 - std::sqrt(5.0));
            const Vec3 at = Vec3{std::sqrt(1 - z * z) * std::cos(around), std::sqrt(1 - z * z) * std::sin(around), z} * PlanetRadius;
            const double height = equilibriumTide(sky, at);
            sum += height, low = std::min(low, height), high = std::max(high, height);
        }
        expect(std::abs(sum / Points) < 1e-4, "the tide has a net height (m)", sum / Points);
        return high - low;
    };
    const auto bulge = [](double gm, Vec3 body) { return gm / EarthGM * std::pow(PlanetRadius, 4) / std::pow(length(body), 3); };
    const Sky spring = skyOn(2460409.26), neap = skyOn(2460416.5);   // new moon 8 Apr 2024, first quarter 15 Apr
    const double springRange = range(spring), neapRange = range(neap);
    const double springExpected = 1.5 * (bulge(MoonGM, spring.moon) + bulge(SunGM, spring.sun)), neapExpected = 1.5 * bulge(MoonGM, neap.moon);
    expect(std::abs(springRange / springExpected - 1) < 0.03, "the spring tide's range is off its bulges", springRange / springExpected);
    expect(std::abs(neapRange / neapExpected - 1) < 0.03, "the neap tide's range is off its bulges", neapRange / neapExpected);
    expect(std::abs(bulge(MoonGM, Vec3{MoonOrbitAxis, 0, 0}) - 0.357) < 0.002, "the Moon's equilibrium bulge is not 0.357 m");
}

// A sea on part of the planet: cells of 2° in latitude and longitude with their true areas. The cap
// z ≥ 0.5 under both bodies at their mean distances holds +0.1955 m of the planet-wide shape on
// average (Codex, DREV-CLAUDE-002-03), so adding it would make water. `wetTide` gives every mask a
// zero volume and keeps the shape: the cap, and two uneven masks at two instants, the second larger.
// Water carried between masks (wetting and drying) is the sea owner's solve and S4's test. A mask is
// one connected body: two cut-off lakes called together trade water, called apart keep it.
void testWetTide() {
    struct Cell {
        Vec3 at;
        double area, latitude, longitude;
    };
    std::vector<Cell> cells;
    for (int row = 0; row < 90; ++row) {
        const double south = (-90 + 2.0 * row) * Degree, north = south + 2 * Degree;
        for (int column = 0; column < 180; ++column) {
            const double latitude = (-89 + 2.0 * row), longitude = (-179 + 2.0 * column);
            cells.push_back({observerAt(latitude, longitude), PlanetRadius * PlanetRadius * 2 * Degree * (std::sin(north) - std::sin(south)),
                             latitude, longitude});
        }
    }
    const auto sea = [&](const Sky& sky, auto wet, double& mean, double& volume, double& shape) {
        std::vector<Vec3> at;
        std::vector<double> areas, raw;
        for (const Cell& cell : cells) {
            if (!wet(cell)) continue;
            at.push_back(cell.at), areas.push_back(cell.area), raw.push_back(equilibriumTide(sky, cell.at));
        }
        std::vector<double> heights(at.size());
        wetTide(sky, at, areas, heights);
        double area = 0, rawVolume = 0, scale = 0;
        volume = 0, shape = 0;
        for (std::size_t i = 0; i < at.size(); ++i) {
            area += areas[i], rawVolume += raw[i] * areas[i], volume += heights[i] * areas[i], scale += std::abs(heights[i]) * areas[i];
            shape = std::max(shape, std::abs((heights[i] - heights[0]) - (raw[i] - raw[0])));
        }
        mean = rawVolume / area;
        volume /= scale;
    };
    Sky overhead;
    overhead.moon = {0, 0, MoonOrbitAxis};
    overhead.sun = {0, 0, AstronomicalUnit};
    double mean, volume, shape;
    sea(overhead, [](const Cell& cell) { return cell.at.z >= 0.5 * PlanetRadius; }, mean, volume, shape);
    expect(std::abs(mean - 0.1955) < 0.002, "the cap's mean of the planet-wide tide is off Codex's (m)", mean);
    expect(std::abs(volume) < 1e-12 && shape < 1e-12, "the cap's tide changes the water's volume or its shape", volume);

    // Three basins and a polar sea, then a larger mask of the same shape at a later instant.
    const auto ocean = [](double shore) {
        return [shore](const Cell& cell) {
            const bool basins = std::sin(3 * cell.longitude * Degree) + 0.6 * std::cos(2 * cell.latitude * Degree) > shore;
            return basins || cell.latitude > 72 + 10 * shore;
        };
    };
    sea(skyOn(2460409.26), ocean(0.3), mean, volume, shape);
    expect(std::abs(mean) > 1e-3 && std::abs(volume) < 1e-12 && shape < 1e-12, "the uneven sea's tide changes its volume", volume);
    sea(skyOn(2460409.26 + 0.25), ocean(0.2), mean, volume, shape);
    expect(std::abs(mean) > 1e-3 && std::abs(volume) < 1e-12 && shape < 1e-12, "the larger sea's tide changes its volume", volume);

    // Two lakes not joined by water, of 2 and 5 km², one at the pole and one on the equator (Codex,
    // DREV-CLAUDE-002-04). Called together, the common correction moves about 1.1 × 10⁶ m³ from one to
    // the other; called once per lake, each keeps its water.
    const std::vector<Vec3> lakes{observerAt(90, 0), observerAt(0, 0)};
    const std::vector<double> lakeAreas{2e6, 5e6};
    std::vector<double> together(2), pole(1), equator(1);
    wetTide(overhead, lakes, lakeAreas, together);
    wetTide(overhead, std::span(lakes).first(1), std::span(lakeAreas).first(1), pole);
    wetTide(overhead, std::span(lakes).last(1), std::span(lakeAreas).last(1), equator);
    const double moved = together[0] * lakeAreas[0];
    expect(std::abs(std::abs(moved) / 1.124e6 - 1) < 0.01 && std::abs(moved + together[1] * lakeAreas[1]) < 1e-6 * std::abs(moved),
           "one call for two cut-off lakes does not move Codex's water between them (m³)", moved);
    expect(std::abs(pole[0]) < 1e-15 && std::abs(equator[0]) < 1e-15, "a lake called on its own changes its volume (m)", pole[0]);

    std::vector<double> one(1);
    bool threw = false;
    try {
        wetTide(overhead, std::vector<Vec3>{cells[0].at}, std::vector<double>{}, one);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "wetTide accepts mismatched spans");
}

} // namespace

void testAstronomyTide() {
    testTide();
    testWetTide();
}

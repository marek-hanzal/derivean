#include "core/fn/Noise.hpp"
#include "core/service/Workers.hpp"
#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Flow.hpp"
#include "icesheet/fn/Region.hpp"
#include "icesheet/fn/Sheet.hpp"
#include "landscape/fn/Process.hpp"
#include "measure/fn/Plane.hpp"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>

using namespace derivean::sim;

// Continental ice on hand-built ground: the sheet itself, then what its sliding does to a region
// domain. The worlds' own sheets are looked at with `terra icesheet`.

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "IceSheet: " << message << " (" << value << ")\n";
    std::exit(1);
}

float groundOf(const Domain& domain, int x, int z) {
    const size_t i = domain.index(x, z);
    return domain.ground.rock[i] + domain.sedimentAt(i);
}

Domain regionDomain(const std::function<float(double, double)>& ground, float warmth, float rain) {
    Domain domain;
    domain.key = {Level::Region, 0, 0};
    domain.cell = specOf(Level::Region).cell;
    domain.step = specOf(Level::Region).step;
    domain.origin = {-Halo * domain.cell, -Halo * domain.cell};
    domain.seaLevel = 0;
    allocateBare(domain);
    domain.unit.assign(domain.cells(), UnitCache{-1e30f, 1e30f, traitsOf(Material::Granite)});
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.rock[i] = domain.ground.datum[i] = ground(x * domain.cell, z * domain.cell);
            domain.warmth[i] = warmth, domain.rain[i] = domain.rainBase[i] = rain;
            if (x == 0 || z == 0 || x == last || z == last) domain.hold[i] = Rim;
        }
    return domain;
}

SheetSource plainSource(std::vector<double> cooling, std::function<float(int, Vec2)> thickness, float heat, float halfRange) {
    SheetSource source;
    source.cooling = std::move(cooling);
    source.thickness = std::move(thickness);
    source.sink = [](int, Vec2) { return 0.0f; };
    source.heat = [heat](Vec2) { return heat; };
    source.halfRange = [halfRange](Vec2) { return halfRange; };
    return source;
}

WearTotals add(WearTotals sum, const WearTotals& more) {
    sum.worn += more.worn, sum.entered += more.entered, sum.lodged += more.lodged, sum.dropped += more.dropped, sum.left += more.left;
    return sum;
}

double closure(const WearTotals& sum) { return std::abs(sum.worn + sum.entered - sum.lodged - sum.dropped - sum.left) / std::max(1.0, sum.worn); }

// A plateau 600 m high in the middle of a sea, on cells of 20 km.
IceGrid plateau(float warmth) {
    constexpr int Side = 100;
    constexpr double Cell = 20000;
    IceGrid grid;
    shapeIce(grid, Side, Side, Cell, false);
    for (int z = 0; z < Side; ++z)
        for (int x = 0; x < Side; ++x) {
            const size_t i = grid.index(x, z);
            const double r = std::hypot(x - Side / 2.0, z - Side / 2.0) * Cell;
            grid.bed[i] = float(600.0 - 900.0 * std::max(0.0, r / (0.4 * Side * Cell) - 0.6) / 0.4);
            grid.warmth[i] = warmth, grid.halfRange[i] = 14, grid.rain[i] = 0.5f, grid.heat[i] = 0.025f;
            if (x == 0 || z == 0 || x == Side - 1 || z == Side - 1) grid.held[i] = 1;
        }
    return grid;
}

// A cold plateau grows a dome: thick and flat inside, steep at its margin. A warm one stays bare.
// The pool changes nothing.
void testDome() {
    IceGrid cold = plateau(-12), alone = plateau(-12), warm = plateau(8);
    Workers pool(4);
    relaxIce(cold, &pool, 4000, 2e-4f, true);
    relaxIce(alone, nullptr, 4000, 2e-4f, true);
    relaxIce(warm, &pool, 400, 2e-4f, true);
    const int middle = cold.width / 2;
    const float centre = cold.thickness[cold.index(middle, middle)];
    expect(centre > 1500 && centre < 4500, "the dome on a cold plateau is not some kilometres thick", centre);
    // Along a radius: the outer half of the ice loses more thickness than the inner half.
    int margin = middle;
    while (margin + 1 < cold.width && cold.thickness[cold.index(margin + 1, middle)] > LeastSheet) ++margin;
    const float halfway = cold.thickness[cold.index((middle + margin) / 2, middle)];
    expect(margin > middle + 5, "the dome does not spread over the plateau", margin - middle);
    expect(centre - halfway < halfway, "the dome is not flat inside and steep at its margin", centre - halfway);
    for (size_t i = 0; i < cold.cells(); ++i) {
        if (cold.thickness[i] != alone.thickness[i]) expect(false, "the dome depends on the pool", double(i));
        expect(warm.thickness[i] < LeastSheet, "a warm plateau holds ice", warm.thickness[i]);
    }
}

// A coastal range with a river valley through it; a sheet comes from the plateau behind.
Domain coastalRange() {
    return regionDomain(
        [](double x, double z) {
            const double km = x / 1000, across = z / 1000;
            double h = km < 70 ? -150 - 6 * (70 - km) : km < 110 ? 1500 * (km - 70) / 40 : km < 150 ? 1500 - 700 * (km - 110) / 40 : 800;
            if (km >= 70 && km <= 200) {
                const double floor = std::max(5.0, 700 * (km - 70) / 130.0), wide = std::max(0.0, 1.0 - std::abs(across - 160) / 7);
                h -= wide * std::max(0.0, h - floor);
            }
            return float(h + 8 * std::sin(km * 0.9) * std::cos(across * 0.7));
        },
        0.0f, 0.8f);
}

SheetSource rangeSource() {
    return plainSource({0.0, -8.0}, [](int stage, Vec2 p) { return stage == 0 ? 0.0f : float(1700 * std::sqrt(std::clamp((p.x / 1000 - 60) / 230, 0.0, 1.0))); }, 0.025f, 12.0f);
}

WearTotals workRange(Domain& domain, SheetWork& work, Workers* workers) {
    constexpr int Workings = 15;
    WearTotals sum;
    for (int k = 0; k < Workings; ++k) sum = add(sum, workSheets(domain, work, {0.4f, 0.6f}, 0.0, 16 * 100e3, 1.6e6, k + 1 == Workings, workers));
    return sum;
}

// The valley through the range is cut into a trough below the sea, deeper than its mouth, while the
// ridge beside it and the frozen plateau keep their height. The pool changes nothing.
void testFjord() {
    Domain domain = coastalRange(), alone = coastalRange();
    SheetWork work, aloneWork;
    work.source = rangeSource(), aloneWork.source = rangeSource();
    Workers pool(4);
    const WearTotals sum = workRange(domain, work, &pool);
    workRange(alone, aloneWork, nullptr);
    expect(closure(sum) < 1e-6, "what the ice wore off is not where it went", closure(sum));
    for (size_t i = 0; i < domain.cells(); ++i)
        if (domain.ground.rock[i] != alone.ground.rock[i]) expect(false, "the worn ground depends on the pool", double(i));

    const auto floorAt = [&](int x) {
        float low = 1e9f;
        for (int z = 153; z <= 167; ++z) low = std::min(low, groundOf(domain, x, z));
        return low;
    };
    float deepest = 1e9f;
    for (int x = 72; x <= 100; ++x) deepest = std::min(deepest, floorAt(x));
    const float mouth = floorAt(70);
    expect(deepest < -80, "the valley through the range is not cut below the sea", deepest);
    expect(deepest < mouth - 40, "the trough has no sill at its mouth", mouth - deepest);
    const float ridge = groundOf(domain, 94, 125);
    expect(ridge - deepest > 800, "the ridge beside the trough does not stand high above it", ridge - deepest);
    // Where the plateau's ice is frozen to its bed it does not slide and wears nothing.
    const SheetField field = readStage(domain, work, IceLastMaximum, &pool);
    expect(field.slide[domain.index(280, 125)] < 1, "the cold plateau under thick slow ice slides", field.slide[domain.index(280, 125)]);
    expect(std::abs(groundOf(domain, 280, 125) - 800) < 15, "the frozen plateau was worn", groundOf(domain, 280, 125) - 800);
}

// A low shield cut by fracture zones, ice from the north ending on it: the zones become lakes in
// rock, a moraine stands where the ice ended, gravel and sand lie beyond, an esker under the ice.
void testShield() {
    Domain domain = regionDomain([](double x, double z) { return float(260 - 0.0006 * z + 12 * fractalNoise(7, x / 9000, z / 9000, 4)); }, 0.0f, 0.6f);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const double a = x * 0.94 + z * 0.34, b = -x * 0.34 + z * 0.94;
            const double da = std::abs(std::fmod(a + 1000, 23.0) - 11.5), db = std::abs(std::fmod(b + 1000, 31.0) - 15.5);
            domain.weakness[i] = float(std::max(smoothstep(10.2, 11.2, da), smoothstep(14.4, 15.2, db)));
            domain.quality[i] = float(std::exp(0.6 * fractalNoise(11, x / 14.0, z / 14.0, 3)));
            domain.warmth[i] = float(-3.0 + 9.0 * z / domain.size);
        }
    SheetWork work;
    work.source = plainSource({0.0, -6.0, -9.0},
                              [](int stage, Vec2 p) { return stage == 0 ? 0.0f : float(1500 * std::sqrt(std::clamp(((stage == 1 ? 200e3 : 290e3) - p.z) / 230e3, 0.0, 1.0))); }, 0.03f,
                              13.0f);
    Workers pool(4);
    constexpr int Workings = 8;
    WearTotals sum;
    for (int k = 0; k < Workings; ++k) {
        const bool last = k + 1 == Workings;
        sum = add(sum, workSheets(domain, work, {0.5f, 0.5f, 0.0f}, last ? 0.0 : (k % 4) / 4.0, last ? 100e3 : 16 * 100e3, last ? 0 : 1.6e6, last, &pool));
    }
    expect(closure(sum) < 1e-6, "what the shield's ice wore off is not where it went", closure(sum));

    FlowScratch scratch;
    for (size_t i = 0; i < domain.cells(); ++i) domain.runoff[i] = 0.3f, domain.warmthNow[i] = 2;
    for (int x = 0; x < domain.size; ++x) domain.outlet[domain.index(x, domain.size - 1)] = 1;
    floodLevels(domain, scratch);
    routeWater(domain, scratch);
    double iced = 0, lakes = 0, inRock = 0, weakIced = 0, weakLakes = 0, moraine = 0, outwash = 0, esker = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) {
            const size_t i = domain.index(x, z);
            moraine += work.left.moraine[i] > 0.5f, outwash += work.left.outwash[i] > 0.5f, esker += work.left.esker[i] > 0.5f;
            if (work.lastMaximum[i] < LeastSheet) continue;
            const float depth = domain.level[i] - domain.surface[i];
            ++iced, weakIced += domain.weakness[i];
            if (depth <= 0.5f) continue;
            ++lakes, weakLakes += domain.weakness[i];
            inRock += domain.level[i] - domain.ground.rock[i] > 0.5f && domain.sedimentAt(i) < 0.5f * depth;
        }
    expect(lakes / iced > 0.05, "the ground the sheet left holds few lakes", lakes / iced);
    expect(inRock / lakes > 0.8, "the shield's lakes do not lie in rock basins", inRock / lakes);
    expect(weakLakes / lakes > 2 * weakIced / iced, "the shield's lakes do not follow its fracture zones", (weakLakes / lakes) / (weakIced / iced));
    expect(moraine > 10, "no end moraine where the sheet ended", moraine);
    expect(outwash > 0, "no outwash beyond the sheet", outwash);
    expect(esker > 0, "no esker under the sheet", esker);
}

} // namespace

void testIceSheet() {
    testDome();
    testFjord();
    testShield();
}

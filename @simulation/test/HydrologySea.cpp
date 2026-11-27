#include "chronology/type/Calendar.hpp"
#include "SeaFixture.hpp"

using namespace derivean::sim;
using namespace derivean::sim::sea;

void testStraitBodies();   // HydrologyStrait.cpp

namespace {

constexpr double ThroatArea = 1000.0;   // m²
const double Pull = OrificeCoefficient * ThroatArea * std::sqrt(2 * Gravity);

// A sea of `seaSide`² cells with its floor at −1000 m and its surface at −20 m, and behind a ridge
// at +100 m a dry bowl of `bowlSide`² cells with its floor at −300 m.
Ocean seaAndBowl(int seaSide, int bowlSide) {
    return boxes(40, 16, 100.0, {{2, 2, 2 + seaSide, 2 + seaSide, -1000.0, -20.0}, {20, 2, 20 + bowlSide, 2 + bowlSide, -300.0}});
}

// Free outflow until the bowl rises over the bed, then submerged outflow until the surfaces stand
// level, against the closed form: √H falls linearly at C·A·√(2g)·S/2 with S the sum of the
// inverse wet areas, 1/A_u alone while the outflow is free, and v = ΔH/S.
void testVerticalWalls() {
    Ocean sea = seaAndBowl(12, 8);
    const double up = 144 * Cell * Cell, down = 64 * Cell * Cell, sill = -250.0;
    const int32_t seaCell = cellAt(sea, 6, 6), bowlCell = cellAt(sea, 24, 6);
    expect(seaBodies(sea).size() == 1 && sea.anchor >= 0 && surfaceAt(sea, bowlCell) == -300.0, "the fixture is not one sea beside a dry bowl");
    expect(openStrait(sea, channel(sea, 6, 6, 24, 6, ThroatArea, sill)), "the strait was refused");
    const double head = -20.0 - sill, free = Pull / (2 * up);
    const auto freeVolume = [&](double t) { return up * (head - std::pow(std::sqrt(head) - free * t, 2)); };

    drain(sea, 10 * 3600.0);
    const double v = freeVolume(10 * 3600.0);
    expect(near(sea.moved[0].water, v, 1e-9), "free outflow does not follow the orifice law", sea.moved[0].water / v - 1);
    expect(near(surfaceAt(sea, seaCell), -20.0 - v / up, 1e-9) && near(surfaceAt(sea, bowlCell), -300.0 + v / down, 1e-9), "the sea did not fall by ΔV/A, nor the bowl rise by it");
    expect(near(sea.moved[0].salt, Salinity * v, 1e-9), "the water left without its salt", sea.moved[0].salt / (Salinity * v) - 1);
    expectClosed(sea, "free outflow");

    // The bowl reaches the bed at v₁; from then on both surfaces move.
    const double v1 = down * (sill + 300.0), h1 = head - v1 / up, t1 = (std::sqrt(head) - std::sqrt(h1)) / free;
    const double spread = 1 / up + 1 / down, submerged = 0.5 * Pull * spread;
    drain(sea, t1 + DaySeconds - 10 * 3600.0);
    const double h = std::pow(std::sqrt(h1) - submerged * DaySeconds, 2), v2 = v1 + (h1 - h) / spread;
    expect(near(sea.moved[0].water, v2, 1e-9), "submerged outflow does not follow the orifice law", sea.moved[0].water / v2 - 1);
    expect(near(surfaceAt(sea, seaCell) - surfaceAt(sea, bowlCell), h, 1e-8), "the head is not continuous across the bed", surfaceAt(sea, seaCell) - surfaceAt(sea, bowlCell) - h);

    // Level at √H₁/pace, where the volume-weighted mean of the two surfaces stands.
    const double level = (up * (sill + h1) + down * sill) / (up + down);
    drain(sea, std::sqrt(h1) / submerged - DaySeconds - 100.0);
    expect(surfaceAt(sea, seaCell) - surfaceAt(sea, bowlCell) > 0, "the surfaces met before √H₁/pace");
    drain(sea, 200.0);
    expect(near(surfaceAt(sea, seaCell), level, 1e-9) && near(surfaceAt(sea, bowlCell), level, 1e-9), "the surfaces did not meet at the mean level", surfaceAt(sea, seaCell) - level);
    expect(near(sea.moved[0].water, v1 + h1 / spread, 1e-9), "the bowl did not take what the sea gave");
    const Tally moved = sea.moved[0];
    const std::vector<double> depth = sea.grid.depth;
    drain(sea, 30 * DaySeconds);
    expect(sea.moved[0].water == moved.water && sea.grid.depth == depth, "water still moved after the surfaces met", sea.moved[0].water - moved.water);
    expect(std::abs(strengthAt(sea, bowlCell) - Salinity) < 1e-9 && std::abs(strengthAt(sea, seaCell) - Salinity) < 1e-9, "the salinities parted");
    expectClosed(sea, "after the surfaces met");
}

// A small sea falls to the bed of its strait and the link breaks; the bowl keeps what it received.
void testSillStop() {
    Ocean sea = seaAndBowl(4, 12);
    const double up = 16 * Cell * Cell, sill = -100.0;
    expect(openStrait(sea, channel(sea, 3, 3, 24, 6, ThroatArea, sill)), "the strait was refused");
    drain(sea, 400 * DaySeconds);
    const int32_t seaCell = cellAt(sea, 3, 3), bowlCell = cellAt(sea, 24, 6);
    expect(near(surfaceAt(sea, seaCell), sill, 1e-12) && near(sea.moved[0].water, up * (-20.0 - sill), 1e-9), "the sea did not fall to the bed", surfaceAt(sea, seaCell) - sill);
    expect(std::abs(strengthAt(sea, bowlCell) - Salinity) < 1e-9 && std::abs(strengthAt(sea, seaCell) - Salinity) < 1e-9, "the bowl did not get the sea's salinity");
    const std::vector<double> depth = sea.grid.depth, salt = sea.grid.salt;
    drain(sea, 400 * DaySeconds);
    expect(sea.grid.depth == depth && sea.grid.salt == salt, "water moved over a sill the sea no longer reaches");
    expect(seaBodies(sea).size() == 2, "the parted sea and bowl are not two bodies", double(seaBodies(sea).size()));
    expectClosed(sea, "sill stop");
}

// Water does not run uphill: a channel from the sea to a dry floor above it moves nothing, and a
// strait between cells that coincide is refused.
void testDryEndAbove() {
    Ocean sea = boxes(40, 16, 100.0, {{2, 2, 14, 14, -1000.0, -20.0}, {20, 2, 28, 10, 10.0}});
    const Ocean untouched = sea;
    expect(!openStrait(sea, channel(sea, 6, 6, 6, 6, ThroatArea, -100.0)), "a strait between one cell and itself was cut");
    expect(openStrait(sea, channel(sea, 6, 6, 24, 6, ThroatArea, -100.0)) && openStrait(sea, channel(sea, 6, 7, 16, 8, ThroatArea, -100.0)), "the strait was refused");
    drain(sea, 30 * DaySeconds);
    expect(!seaMoved(sea) && sea.grid.depth == untouched.grid.depth && sea.grid.salt == untouched.grid.salt, "water climbed to a dry end above the sea");
}

// Sloping bowls: the levels come from the flooding tree, and where they meet is the level that
// holds the water over the real cells, found here by bisection without the tree.
void testBathymetry() {
    Ocean sea;
    shapeGrid(sea.grid, 32, 16, Cell);
    sea.grid.periodic = true;
    for (int z = 0; z < 16; ++z) {
        for (int x = 0; x < 32; ++x) {
            const size_t cell = size_t(z) * 32 + size_t(x);
            const double west = std::hypot(x - 8.0, z - 8.0), east = std::hypot(x - 24.0, z - 8.0);
            sea.grid.ground[cell] = float(std::min({-400.0 + 60.0 * west, -300.0 + 60.0 * east, 50.0}));
            if (x < 16) sea.grid.depth[cell] = std::max(0.0, -20.0 - double(sea.grid.ground[cell]));
            sea.grid.salt[cell] = Salinity * sea.grid.depth[cell];
        }
    }
    sea = settled(std::move(sea));
    expect(openStrait(sea, channel(sea, 8, 8, 24, 8, 5000.0, -295.0)), "the strait was refused");
    drain(sea, 3 * YearSeconds);
    const double west = surfaceAt(sea, cellAt(sea, 8, 8)), east = surfaceAt(sea, cellAt(sea, 24, 8));
    double low = -300.0, high = -20.0;
    for (int step = 0; step < 200; ++step) {
        const double mid = 0.5 * (low + high);
        double held = 0;
        for (const float ground : sea.grid.ground) held += std::max(0.0, mid - double(ground)) * Cell * Cell;
        (held < sea.initial.water ? low : high) = mid;
    }
    expect(std::abs(west - east) < 1e-6 && std::abs(west - low) < 1e-6, "the bowls did not meet at the level that holds their water", west - low);
    expect(std::abs(strengthAt(sea, cellAt(sea, 24, 8)) - Salinity) < 1e-9, "the eastern bowl did not get the sea's salinity");
    expectClosed(sea, "bathymetry");
}

// However time is fed in, one strait moves the same water: ticks as an hour, steps across the
// change from free to submerged outflow as one jump.
void testFeeding() {
    Ocean ticked = seaAndBowl(12, 8), hour = seaAndBowl(12, 8);
    for (Ocean* sea : {&ticked, &hour}) openStrait(*sea, channel(*sea, 6, 6, 24, 6, ThroatArea, -250.0));
    for (int tick = 0; tick < 3600 * 20; ++tick) drainOcean(ticked, 0.05);
    settleOcean(ticked);
    drain(hour, 3600.0);
    expect(near(ticked.moved[0].water, hour.moved[0].water, 1e-9), "ticks and an hour move different water", ticked.moved[0].water / hour.moved[0].water - 1);

    Ocean stepped = seaAndBowl(12, 8), once = seaAndBowl(12, 8);
    for (Ocean* sea : {&stepped, &once}) openStrait(*sea, channel(*sea, 6, 6, 24, 6, ThroatArea, -250.0));
    for (int step = 0; step < 60; ++step) drain(stepped, 1800.0);
    drain(once, 60 * 1800.0);
    expect(surfaceAt(once, cellAt(once, 24, 6)) > -250.0, "the jump did not reach submerged outflow");
    expect(near(stepped.moved[0].water, once.moved[0].water, 1e-9), "steps and one jump move different water", stepped.moved[0].water / once.moved[0].water - 1);
    expectClosed(stepped, "stepped");
    expectClosed(once, "one jump");
}

} // namespace

void testSmallSea() {
    testVerticalWalls();
    testSillStop();
    testDryEndAbove();
    testBathymetry();
    testFeeding();
    testStraitBodies();
}

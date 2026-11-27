#include "chronology/type/Calendar.hpp"
#include "hydrology/fn/Strait.hpp"
#include "SeaFixture.hpp"

#include <array>
#include <cmath>

using namespace derivean::sim;
using namespace derivean::sim::sea;

namespace {

constexpr int Width = 40, Height = 16;
constexpr double Ridge = 100.0;
const Box Sea{2, 2, 14, 14, -1000.0, -20.0};   // 144 cells

// One sea feeds two bowls through two throats in different places. They take turns in rounds
// fixed in time, so however the days are fed in the same water moves; both bowls end at the sea's
// salinity and nothing is made or lost.
void testSharedDonor() {
    const auto make = [] {
        Ocean sea = boxes(Width, Height, Ridge, {Sea, {20, 2, 26, 8, -300.0}, {30, 2, 36, 8, -400.0}});
        expect(openStrait(sea, channel(sea, 5, 5, 22, 4, 800.0, -280.0)) && openStrait(sea, channel(sea, 10, 10, 32, 4, 800.0, -380.0)), "a strait was refused");
        return sea;
    };
    Ocean both = make(), walked = make();
    drain(both, 20 * DaySeconds);
    for (int step = 0; step < 500; ++step) drainOcean(walked, 20 * DaySeconds / 500);
    settleOcean(walked);
    for (size_t strait = 0; strait < 2; ++strait)
        expect(both.moved[strait].water > 1e9 && near(walked.moved[strait].water, both.moved[strait].water, 1e-9), "two throats depend on how time is fed in", walked.moved[strait].water / both.moved[strait].water - 1);
    for (const int32_t bowl : {cellAt(both, 22, 4), cellAt(both, 32, 4)})
        expect(std::abs(strengthAt(both, bowl) - Salinity) < 1e-9, "a bowl did not get the sea's salinity", strengthAt(both, bowl));
    expect(seaBodies(both)[0].straits == 2, "the sea does not show both throats", double(seaBodies(both)[0].straits));
    expectClosed(both, "shared donor");
    expectClosed(walked, "shared donor, walked");
}

// The strait fills a bowl to its sill; the bowl then passes on what it receives into its sibling,
// and once that is full too the two stand as one lake over their saddle. That lake rises until it
// stands with the sea, and then the strait is still.
void testMerging() {
    Ocean sea = boxes(Width, Height, Ridge, {Sea, {20, 2, 25, 8, -300.0}, {25, 2, 26, 8, -150.0}, {26, 2, 31, 8, -300.0}});
    expect(openStrait(sea, channel(sea, 6, 6, 22, 4, 2000.0, -290.0)), "the strait was refused");
    const int32_t seaCell = cellAt(sea, 6, 6), west = cellAt(sea, 22, 4), east = cellAt(sea, 28, 4);
    size_t most = 0;
    bool passedOn = false;
    for (int day = 0; day < 60; ++day) {
        drain(sea, DaySeconds);
        most = std::max(most, seaBodies(sea).size());
        passedOn = passedOn || (sea.grid.depth[size_t(east)] > 0 && surfaceAt(sea, east) < -150.0 && std::abs(surfaceAt(sea, west) + 150.0) < 1e-9);
    }
    expect(passedOn, "the full bowl did not pass the water on into its sibling");
    expect(most == 3 && seaBodies(sea).size() == 2, "the bowls did not become one lake", double(seaBodies(sea).size()));
    expect(surfaceAt(sea, west) == surfaceAt(sea, east) && surfaceAt(sea, west) > -150.0, "the merged bowls do not stand level over their saddle", surfaceAt(sea, west) - surfaceAt(sea, east));
    // 144 (−20 − L) = 66 (L + 150) + 60·150: the sea's loss fills the two floors and the saddle strip.
    const double level = (-2880.0 - 66.0 * 150.0 - 60.0 * 150.0) / 210.0;
    expect(near(surfaceAt(sea, seaCell), level, 1e-9) && near(surfaceAt(sea, west), level, 1e-9), "the lake and the sea did not meet", surfaceAt(sea, seaCell) - level);
    const Tally moved = sea.moved[0];
    drain(sea, 30 * DaySeconds);
    expect(sea.moved[0].water == moved.water, "the strait still runs between bodies at one level");
    expectClosed(sea, "merging");
}

// A sea and a bowl stand level through one strait. A second strait lets a higher lake into the
// bowl, which rises over the sea: the first strait runs back, and what it moved falls.
void testReversal() {
    Ocean sea = boxes(Width, Height, Ridge, {Sea, {20, 2, 28, 10, -300.0}, {30, 2, 38, 10, 0.0, 60.0}});
    expect(openStrait(sea, channel(sea, 6, 6, 24, 6, 1000.0, -290.0)), "the first strait was refused");
    drain(sea, 60 * DaySeconds);
    const double level = (144.0 * -20.0 - 64.0 * 300.0) / 208.0;
    expect(near(surfaceAt(sea, cellAt(sea, 24, 6)), level, 1e-9), "the sea and the bowl did not meet", surfaceAt(sea, cellAt(sea, 24, 6)) - level);
    const double forth = sea.moved[0].water;
    expect(openStrait(sea, channel(sea, 34, 6, 26, 8, 1000.0, -290.0)), "the second strait was refused");
    drain(sea, 60 * DaySeconds);
    const double lake = 64.0 * 60.0 * Cell * Cell, back = lake * 144.0 / 208.0;
    expect(near(sea.moved[1].water, lake, 1e-9) && sea.grid.depth[size_t(cellAt(sea, 34, 6))] == 0, "the high lake did not run empty into the bowl", sea.moved[1].water / lake);
    expect(near(sea.moved[0].water, forth - back, 1e-9), "the first strait did not run back by the sea's share", (forth - sea.moved[0].water) / back);
    expect(near(surfaceAt(sea, cellAt(sea, 6, 6)), level + lake / (208.0 * Cell * Cell), 1e-9), "the sea and the bowl do not stand level again");
    expectClosed(sea, "reversal");
}

// Salt goes at the strength the donor has just before each segment. A salt sea runs into a fresh
// lake, which passes its mixed water on into a dry bowl: what the bowl holds is what its strait
// carried, neither fresh nor at the sea's strength. And Codex's case in figures: 10 m³ of fresh
// water take 10 m³ with 0.35 kg of salt, then give 5 m³, which carry 0.0875 kg.
void testCarriedStrength() {
    Ocean sea = boxes(Width, Height, Ridge, {Sea, {20, 2, 28, 10, -400.0, -200.0, 0.0}, {30, 2, 36, 8, -300.0}});
    expect(openStrait(sea, channel(sea, 6, 6, 24, 6, 1000.0, -250.0)) && openStrait(sea, channel(sea, 26, 8, 32, 4, 1000.0, -290.0)), "a strait was refused");
    for (int day = 0; day < 30; ++day) drain(sea, DaySeconds);
    const int32_t bowl = cellAt(sea, 32, 4);
    double held = 0;
    for (int z = 2; z < 8; ++z)
        for (int x = 30; x < 36; ++x) held += sea.grid.salt[size_t(cellAt(sea, x, z))] * Cell * Cell;
    expect(sea.moved[1].water > 1e9 && near(held, sea.moved[1].salt, 1e-12), "the bowl does not hold the salt its strait carried", held / sea.moved[1].salt - 1);
    expect(strengthAt(sea, bowl) > 0.5 && strengthAt(sea, bowl) < Salinity - 0.5, "the bowl's water is not the lake's mixture", strengthAt(sea, bowl));
    expectClosed(sea, "carried strength");

    Ocean small = boxes(Width, Height, Ridge, {{2, 2, 3, 3, 0.0, 1e-3, 0.035}, {6, 2, 7, 3, 0.0, 1e-5, 0.0}, {10, 2, 11, 3, 0.0}});
    const auto basin = [&](int x) { return small.basins.leafOf[size_t(cellAt(small, x, 2))]; };
    Exchange exchange = openExchange(small);
    const Tally first = book(small, exchange, basin(2), basin(6), basin(6), 10.0);
    const Tally second = book(small, exchange, basin(6), basin(10), basin(10), 5.0);
    expect(near(first.salt, 0.35, 1e-12) && near(second.salt, 0.0875, 1e-12), "the mixed lake did not pass on its mixture", second.salt);
    closeExchange(small, exchange);
    expect(near(small.grid.salt[size_t(cellAt(small, 10, 2))] * Cell * Cell, 0.0875, 1e-9), "the bowl did not receive what was booked");
    expect(small.unbooked.water <= 1e-12 && small.unbooked.salt <= 1e-12, "the small bodies do not close", small.unbooked.salt);
}

// A dry bowl floored with salt crust: the water the strait brings dissolves it, so the bowl holds
// the crust and what was carried. A crusted pit no strait reaches keeps its crust to the bit.
void testCrust() {
    constexpr double Crust = 50.0;   // kg/m²
    Ocean sea = boxes(Width, Height, Ridge, {Sea, {20, 2, 26, 8, -300.0}, {30, 2, 34, 6, -200.0}});
    for (const auto& [x0, z0, x1, z1] : {std::array{20, 2, 26, 8}, std::array{30, 2, 34, 6}})
        for (int z = z0; z < z1; ++z)
            for (int x = x0; x < x1; ++x) sea.grid.salt[size_t(cellAt(sea, x, z))] = Crust;
    sea = settled(std::move(sea));
    const std::vector<double> before = sea.grid.salt;
    expect(openStrait(sea, channel(sea, 6, 6, 22, 4, 1000.0, -290.0)), "the strait was refused");
    drain(sea, DaySeconds);
    double held = 0;
    for (int z = 2; z < 8; ++z) {
        for (int x = 20; x < 26; ++x) {
            const size_t cell = size_t(cellAt(sea, x, z));
            expect(sea.grid.depth[cell] > 0, "a bowl cell stayed dry", double(cell));
            held += sea.grid.salt[cell] * Cell * Cell;
        }
    }
    const double crust = 36.0 * Crust * Cell * Cell;
    expect(sea.moved[0].water > 0 && near(held, crust + sea.moved[0].salt, 1e-12), "the bowl does not hold its crust and what was carried", held / (crust + sea.moved[0].salt) - 1);
    for (int z = 2; z < 6; ++z) {
        for (int x = 30; x < 34; ++x) {
            const size_t cell = size_t(cellAt(sea, x, z));
            expect(sea.grid.depth[cell] == 0 && sea.grid.salt[cell] == before[cell], "a crust no strait reached changed", double(cell));
        }
    }
    expectClosed(sea, "crust");
}

// A bowl beside a full fresh sibling: one strait fills the bowl until the two stand as one lake,
// a second drains it below their saddle again within the same hour. The sibling keeps the strength
// it had while they were one, and no salt is lost.
void testMergeAndPart() {
    Ocean sea = boxes(Width, Height, Ridge, {Sea, {20, 2, 25, 8, -300.0}, {25, 2, 26, 8, -150.0}, {26, 2, 31, 8, -300.0, -150.0, 0.0}, {33, 2, 39, 8, -400.0}});
    expect(openStrait(sea, channel(sea, 6, 6, 22, 4, 1e5, -290.0)) && openStrait(sea, channel(sea, 23, 5, 35, 4, 1e6, -390.0)), "a strait was refused");
    drain(sea, 3600.0);
    const int32_t sibling = cellAt(sea, 28, 4);
    expect(sea.moved[1].water > 0 && strengthAt(sea, sibling) > 0, "the sibling did not take the mixed strength", strengthAt(sea, sibling));
    expectClosed(sea, "merge and part");
}

// Three seas in a row joined by two straits of 1 m², cut at `opened`. Its rounds end where
// `opened` plus their offset rounds to, a little short of the whole offset for most fractions.
Ocean chain(double opened) {
    Ocean sea = boxes(17, 7, 30, {{1, 2, 4, 5, 0, 10}, {6, 2, 9, 5, 0, 4}, {11, 2, 14, 5, 0, 1}});
    drainOcean(sea, opened);
    expect(openStrait(sea, channel(sea, 2, 3, 7, 3, 1, 0)) && openStrait(sea, channel(sea, 7, 3, 12, 3, 1, 0)), "a strait was refused");
    return sea;
}

// Every round ends past the time it starts at, also on the stored end of the round before: the
// sea cut at a fraction of a second reaches any time asked for. Fed in, settled or integrated a
// day at a time, it moves what the sea cut on the whole second before it moves in the same span.
void testFractionalCut() {
    for (const double opened : {496.07, 992.05, 1984.01}) {
        Ocean walked = chain(opened);
        for (int round = 0; walked.seconds < opened + DaySeconds; ++round) {
            const double end = roundEnd(walked);
            expect(end > walked.seconds && round < 64, "a round does not end past the time it starts at", walked.seconds - walked.opened);
            walked.seconds = end;
        }
        // The last whole-second round before 2^53 s still ends past now; from there the turns end.
        walked.opened = 0;
        walked.seconds = 0x1p53 - 1.0;
        expect(roundEnd(walked) > walked.seconds, "the last round does not end past now", walked.seconds);
        walked.seconds = 0x1p53 - 86400.0;   // the year's round would end past the cutoff (DREV-011-07)
        expect(roundEnd(walked) == 0x1p53, "the last round does not end on the cutoff", roundEnd(walked) - 0x1p53);
        for (const double far : {0x1p53, 10062568091.0 * 31557600.0, 1070162113858.0 * 31557600.0, 1e24}) {
            walked.seconds = far;
            expect(std::isinf(roundEnd(walked)), "rounds are followed past 2^53 s", far);
        }
        walked.opened = walked.seconds = 0x1p126;   // cut where 2^53 s no longer moves the clock
        expect(std::isinf(roundEnd(walked)), "rounds are followed where the cutoff does not move the clock", walked.seconds);
        for (const double span : {7200.0, 14400.0, 28800.0, DaySeconds + 1.0}) {
            const bool settled = span < DaySeconds;   // a day and a second is integrated on its own
            Ocean cut = chain(opened), whole = chain(std::floor(opened));
            for (Ocean* sea : {&cut, &whole}) {
                drainOcean(*sea, span);
                if (settled) settleOcean(*sea);
                expectClosed(*sea, "fractional cut");
            }
            expect(near(cut.seconds - cut.opened, whole.seconds - whole.opened, 1e-12) && near(cut.pending, whole.pending, 1e-12), "the fractional cut integrated another span", cut.seconds - cut.opened);
            for (size_t strait = 0; strait < 2; ++strait)
                expect(cut.moved[strait].water > 0 && near(cut.moved[strait].water, whole.moved[strait].water, 1e-12) && near(cut.moved[strait].salt, whole.moved[strait].salt, 1e-12),
                       "the fractional cut moved other water or salt", cut.moved[strait].water / whole.moved[strait].water - 1);
        }
    }
}

} // namespace

void testStraitLedger();   // HydrologyLedger.cpp

void testStraitBodies() {
    testCarriedStrength();
    testCrust();
    testSharedDonor();
    testMerging();
    testMergeAndPart();
    testReversal();
    testFractionalCut();
    testStraitLedger();
}

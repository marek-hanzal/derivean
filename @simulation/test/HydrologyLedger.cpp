#include "chronology/type/Calendar.hpp"
#include "hydrology/fn/Strait.hpp"
#include "SeaFixture.hpp"

#include <tuple>

using namespace derivean::sim;
using namespace derivean::sim::sea;

namespace {

constexpr int Width = 40, Height = 16;
constexpr double Ridge = 100.0;
constexpr double Crust = 2.0;   // kg/m² on the floor of a dry bowl

Ocean crusted(Ocean ocean, int x0, int z0, int x1, int z1) {
    for (int z = z0; z < z1; ++z)
        for (int x = x0; x < x1; ++x) ocean.grid.salt[size_t(cellAt(ocean, x, z))] = Crust;
    return settled(std::move(ocean));
}

int32_t leafAt(const Ocean& ocean, int x, int z) { return ocean.basins.leafOf[size_t(cellAt(ocean, x, z))]; }

// Three one-cell bowls: A salt, B dry with crust, C dry. A gives to B, and the crust dissolves the
// moment B's floor is wet, so what B gives to C in the same round already carries it.
void testCrustCarried() {
    Ocean bowls = crusted(boxes(Width, Height, Ridge, {{2, 2, 3, 3, 0.0, 1.0}, {6, 2, 7, 3, 0.0}, {10, 2, 11, 3, 0.0}}), 6, 2, 7, 3);
    const int32_t a = leafAt(bowls, 2, 2), b = leafAt(bowls, 6, 2), c = leafAt(bowls, 10, 2);
    Exchange exchange = openExchange(bowls);
    const double given = 0.5 * bowls.lakes.own[size_t(a)], passed = 0.4 * given, crust = Crust * Cell * Cell;
    book(bowls, exchange, a, b, b, given);
    expect(bowls.grid.salt[size_t(cellAt(bowls, 6, 2))] == 0, "the crust did not dissolve when the floor was wetted");
    const Tally carried = book(bowls, exchange, b, c, c, passed);
    const double expected = passed * (given * Salinity + crust) / given;
    expect(near(carried.salt, expected, 1e-12), "what B passed on does not carry the dissolved crust", carried.salt / expected - 1);
    closeExchange(bowls, exchange);
    expect(near(bowls.grid.salt[size_t(cellAt(bowls, 10, 2))] * Cell * Cell, expected, 1e-12), "C does not hold what B passed on");
    expectClosed(bowls, "crust carried");
}

// B's floor is wetted and runs dry again in one integration, then is wetted once more: its crust
// dissolves once, the floor ends with none, and its salt is in what B passed on.
void testTransientWet() {
    Ocean bowls = crusted(boxes(Width, Height, Ridge, {{2, 2, 3, 3, 0.0, 1.0}, {6, 2, 7, 3, 0.0}, {10, 2, 11, 3, 0.0}}), 6, 2, 7, 3);
    const int32_t a = leafAt(bowls, 2, 2), b = leafAt(bowls, 6, 2), c = leafAt(bowls, 10, 2);
    const int32_t floor = cellAt(bowls, 6, 2);
    const double given = 0.25 * bowls.lakes.own[size_t(a)], crust = Crust * Cell * Cell;
    Exchange exchange = openExchange(bowls);
    double toC = 0;
    for (int pass = 0; pass < 2; ++pass) {
        book(bowls, exchange, a, b, b, given);
        toC += book(bowls, exchange, b, c, c, bowls.lakes.own[size_t(b)]).salt;
        relake(bowls, exchange);
    }
    expect(near(toC, 2 * given * Salinity + crust, 1e-12), "the crust did not dissolve exactly once", toC / (2 * given * Salinity + crust) - 1);
    closeExchange(bowls, exchange);
    expect(bowls.grid.depth[size_t(floor)] == 0 && bowls.grid.salt[size_t(floor)] == 0, "a floor wetted and dried again kept salt", bowls.grid.salt[size_t(floor)]);
    expectClosed(bowls, "transient wet");
}

void expectSame(const Ocean& a, const Ocean& b, const char* what) {
    for (size_t cell = 0; cell < a.grid.cells(); ++cell) {
        expect(std::abs(a.grid.depth[cell] - b.grid.depth[cell]) <= 1e-9 * std::max(1.0, std::abs(b.grid.depth[cell])), what, double(cell));
        expect(std::abs(a.grid.salt[cell] - b.grid.salt[cell]) <= 1e-9 * std::max(1.0, std::abs(b.grid.salt[cell])), what, double(cell));
    }
}

// The first second, to one, at which `reached` holds for the sea `make` builds once it has run.
template <typename Make, typename Reached> double whenOf(Make make, Reached reached, double window) {
    double before = 0, after = window;
    while (after - before > 1.0) {
        Ocean probe = make();
        const double mid = 0.5 * (before + after);
        drain(probe, mid);
        (reached(probe) ? after : before) = mid;
    }
    return after;
}

// A sea feeds a bowl whose deep floor and shelf carry crust, and with two straits the bowl feeds a
// lower one at once. The shelf is wetted inside a round.
Ocean shelved(bool onwards) {
    Ocean sea = crusted(boxes(Width, Height, Ridge, {{2, 2, 14, 14, -1000.0, -20.0}, {20, 2, 23, 8, -300.0}, {23, 2, 26, 8, -280.0}, {30, 2, 36, 8, -400.0}}), 20, 2, 26, 8);
    expect(openStrait(sea, channel(sea, 6, 6, 21, 4, 300.0, -295.0)) && (!onwards || openStrait(sea, channel(sea, 22, 6, 32, 4, 300.0, -395.0))), "a strait was refused");
    return sea;
}

// Two throats sharing the bowl take turns in rounds: fed in one piece, in ticks, or settled at the
// end of a round, the same window ends in the same bodies and crust. Settled inside a round, they
// would differ to first order in the round, which is what taking turns costs.
void testFeedingAcrossRounds() {
    const double window = 3 * DaySeconds;
    const auto make = [] { return shelved(true); };
    const int32_t shelf = cellAt(make(), 24, 4);
    const double wet = whenOf(make, [&](const Ocean& sea) { return sea.grid.depth[size_t(shelf)] > 0; }, window);
    expect(wet > 4 * 3600.0 && wet < window - 3600.0, "the shelf is not wetted inside the window after the third round", wet);
    Ocean piece = make(), ticked = make(), rounded = make();
    drainOcean(piece, window);
    settleOcean(piece);
    for (int tick = 0; tick < 2000; ++tick) drainOcean(ticked, window / 2000);
    settleOcean(ticked);
    drain(rounded, 4 * 3600.0);   // the end of the third round after the straits were cut
    drain(rounded, window - 4 * 3600.0);
    expect(piece.moved[1].salt > 0, "nothing went on into the lower bowl");
    expectSame(ticked, piece, "ticks and one piece end in different bodies");
    expectSame(rounded, piece, "settling at the end of a round changes the bodies");
    for (const Ocean* sea : {&piece, &ticked, &rounded}) expectClosed(*sea, "feeding across rounds");
}

// A lake over two bowls joined above a saddle; `high` adds a high sea that feeds the dry bowl the
// lake runs into, through a second strait.
Ocean parting(bool high) {
    Ocean sea = boxes(Width, Height, Ridge, {{2, 2, 14, 14, -1000.0, high ? 10.0 : -1e30}, {20, 2, 24, 8, -300.0, -100.0}, {24, 2, 25, 8, -150.0, -100.0}, {25, 2, 29, 8, -300.0, -100.0}, {32, 2, 38, 10, -400.0}});
    expect(openStrait(sea, channel(sea, 27, 4, 34, 5, 3000.0, -390.0)) && (!high || openStrait(sea, channel(sea, 6, 6, 35, 8, 300.0, -390.0))), "a strait was refused");
    return sea;
}

bool parted(const Ocean& sea) { return sea.lakes.lakeOf[size_t(leafAt(sea, 22, 4))] != sea.lakes.lakeOf[size_t(leafAt(sea, 27, 4))]; }

// One throat: settling exactly when the shelf is wetted, or exactly when the lake parts, ends in
// the same bodies and crust as the window in one piece.
void testSplitAtEvents() {
    const double window = 3 * DaySeconds;
    const auto wetting = [] { return shelved(false); };
    const int32_t shelf = cellAt(wetting(), 24, 4);
    const auto parts = [] { return parting(false); };
    const double wet = whenOf(wetting, [&](const Ocean& sea) { return sea.grid.depth[size_t(shelf)] > 0; }, window);
    const double apart = whenOf(parts, parted, window);
    expect(wet > 3600.0 && wet < window - 3600.0 && apart > 3600.0 && apart < window - 3600.0, "the shelf is not wetted, or the lake does not part, inside the window", wet);
    for (const auto& [make, at, what] : {std::tuple{+wetting, wet, "settling when the shelf is wetted changes the bodies"}, std::tuple{+parts, apart, "settling when the lake parts changes the bodies"}}) {
        Ocean piece = make(), split = make();
        drain(piece, window);
        drain(split, at);
        drain(split, window - at);
        expectSame(split, piece, what);
        expectClosed(split, what);
    }
}

// The lake runs into the bowl until it parts; the high sea fills the bowl until it runs back into
// the part the first strait ends in. What the first strait moved passes through zero while the
// sea has changed.
void testPartingAndBack() {
    const double window = 2 * YearSeconds, step = 600.0;
    // The lake parts first and joins again once the bowl has run back into it.
    double whole = 0;
    for (Ocean probe = parting(true); !parted(probe) && whole < 2 * DaySeconds; drain(probe, step)) whole += step;
    double apart = whole;
    whole -= step;
    expect(apart < 2 * DaySeconds, "the lake did not part");
    while (apart - whole > 1.0) {
        Ocean probe = parting(true);
        const double mid = 0.5 * (whole + apart);
        drain(probe, mid);
        (parted(probe) ? apart : whole) = mid;
    }
    Ocean split = parting(true), end = parting(true);
    drain(split, apart);
    drain(end, window);
    expect(split.moved[0].water > 0 && end.moved[0].water < 0, "the bowl did not run back into the lake", end.moved[0].water);
    double forth = apart, back = window;
    while (back - forth > 1.0) {
        Ocean probe = parting(true);
        const double mid = 0.5 * (forth + back);
        drain(probe, mid);
        (probe.moved[0].water > 0 ? forth : back) = mid;
    }
    Ocean even = parting(true);
    drain(even, forth);
    const Ocean start = parting(true);
    const double bowl = surfaceAt(even, cellAt(even, 34, 5)) - surfaceAt(start, cellAt(start, 34, 5));
    expect(std::abs(even.moved[0].water) < 1e-3 * split.moved[0].water && bowl > 1.0 && even.changes > 0, "the sea did not change while the first strait's account stood at zero", bowl);
    expectClosed(even, "parting, at zero");
}

// The same in one integration, booked by hand: the lake gives what stands over its saddle, parts,
// one part gives more, and the bowl gives all of it back into that part. The bodies' accounts know
// the parts as bodies of their own; the sea ends as it began, and closes. And where a level
// changed the sea counts a change though no strait has booked anything.
void testPartingAccounts() {
    Ocean sea = boxes(Width, Height, Ridge, {{20, 2, 24, 8, -300.0, -100.0}, {24, 2, 25, 8, -150.0, -100.0}, {25, 2, 29, 8, -300.0, -100.0}, {32, 2, 38, 10, -400.0}});
    const int32_t west = leafAt(sea, 22, 4), east = leafAt(sea, 27, 4), bowl = leafAt(sea, 34, 5);
    const int32_t lake = sea.basins.basins[size_t(west)].parent;
    expect(lake == sea.basins.basins[size_t(east)].parent && sea.lakes.lakeOf[size_t(west)] == lake, "the fixture is not one lake over two bowls");
    const Ocean start = sea;
    Ocean seen = sea;
    expect(openStrait(seen, channel(seen, 27, 4, 34, 5, 1000.0, -390.0)), "the strait was refused");
    Exchange once = openExchange(seen);
    book(seen, once, lake, bowl, bowl, seen.lakes.own[size_t(lake)]);
    closeExchange(seen, once);
    expect(seen.changes == 1 && seen.moved[0].water == 0 && seen.grid.depth[size_t(cellAt(seen, 34, 5))] > 0, "a change of the sea booked by no strait was not counted", double(seen.changes));
    Exchange exchange = openExchange(sea);
    const double over = sea.lakes.own[size_t(lake)], more = 0.5 * sea.lakes.own[size_t(east)];
    book(sea, exchange, lake, bowl, bowl, over);
    relake(sea, exchange);
    book(sea, exchange, east, bowl, bowl, more);
    relake(sea, exchange);
    expect(exchange.account[size_t(west)].water > 0 && near(exchange.account[size_t(east)].water, sea.basins.basins[size_t(east)].capacity - more, 1e-12),
           "the parts are not booked as bodies of their own", exchange.account[size_t(west)].water);
    book(sea, exchange, bowl, east, east, more);
    relake(sea, exchange);
    book(sea, exchange, bowl, pourTarget(sea.basins, sea.lakes.own, east), east, over);
    closeExchange(sea, exchange);
    expectSame(sea, start, "giving everything back did not restore the lake");
    expectClosed(sea, "parting accounts");
}

} // namespace

void testStraitLedger() {
    testCrustCarried();
    testTransientWet();
    testFeedingAcrossRounds();
    testSplitAtEvents();
    testPartingAndBack();
    testPartingAccounts();
}

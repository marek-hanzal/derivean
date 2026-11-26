#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Local.hpp"
#include "measure/fn/Plane.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace derivean::sim;

// The last sheet over two neighbouring Local tiles, the west one finished first: a plain under ice,
// a stream under the ice along a row, its fill spread by the parent. With the ice moving east the
// stretch that straddles the seam starts on the west tile, moving west on the east tile. Where the
// cover beside the stream is thin, a tile alone could not find the fill of the stretches there.

namespace {

void expect(bool passed, const std::string& message, double value = 0) {
    if (passed) return;
    std::cerr << "IceSheetSeam: " << message << " (" << value << ")\n";
    std::exit(1);
}

constexpr int Stream = 140;       // row the stream under the ice follows

struct Case {
    int flow = 1;                 // +1 the ice and its streams move east, -1 west
    float slide = 60;             // m/yr; zero holds the bed frozen, so only the streams move cover
    double supply = 0.5;          // share of the fill the parent's cover still holds
    int thinFrom = 0, thinTo = 0; // world columns where the cover beside the stream is thin
    int first = -1000, last = 1000;   // parent cells the stretches start in
};

// World column of domain column x in tile tx.
int column(int tx, int x) { return x - Halo + tx * TileCells; }

Domain plainAt(int tx, const Case& at) {
    Domain domain;
    domain.key = {Level::Local, tx, 0};
    domain.cell = specOf(Level::Local).cell;
    domain.origin = {(tx * TileCells - Halo) * domain.cell, -Halo * domain.cell};
    allocateBare(domain);
    domain.unit.assign(domain.cells(), UnitCache{-1e30f, 1e30f, traitsOf(Material::Granite)});
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            const int g = column(tx, x);
            domain.ground.rock[i] = domain.ground.datum[i] = float(200 - 0.002 * g * domain.cell);
            const bool thin = g >= at.thinFrom && g < at.thinTo && std::abs(z - Stream) < 8;
            domain.ground.sediment[Coarse][i] = domain.ground.sediment[Sand][i] = thin ? 0.05f : 2.0f;
            domain.ground.sediment[Fines][i] = 2;
            if (x == 0 || z == 0 || x == last || z == last) domain.hold[i] = Rim;
        }
    return domain;
}

ParentSheet sheetAt(const Domain& domain, int tx, const Case& at) {
    ParentSheet parent;
    parent.any = true;
    const size_t cells = domain.cells();
    parent.iceNow.assign(cells, 0.0f), parent.iceThen.assign(cells, 800.0f), parent.slideX.assign(cells, float(at.flow) * at.slide), parent.slideZ.assign(cells, 0.0f);
    parent.worn.assign(cells, at.slide > 0 ? 80.0f : 0.0f), parent.esker.assign(cells, 0.0f), parent.topNow.assign(cells, 0.0f), parent.topThen.assign(cells, 0.0f);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const int g = column(tx, x);
            parent.topThen[i] = float(1000 - at.flow * 0.001 * g * domain.cell);
            parent.topNow[i] = domain.ground.rock[i] + domain.sedimentAt(i);
            const int cell = int(std::floor(g / double(Refinement)));
            if (std::abs(z - Stream) < 8 && cell >= at.first && cell <= at.last) parent.esker[i] = 10.0f;
        }
    return parent;
}

// The parent's stretches whose cells lie over the domain, as parentTunnels lists them.
std::vector<Tunnel> tunnelsOver(const Domain& domain, const Case& at) {
    const double stretch = Refinement * domain.cell, z = (Stream + 0.5) * domain.cell + domain.origin.z;
    const double extent = domain.size * domain.cell;
    std::vector<Tunnel> tunnels;
    for (int64_t g = int64_t(std::floor(domain.origin.x / stretch)); g <= int64_t(std::floor((domain.origin.x + extent) / stretch)); ++g) {
        if (g < at.first || g > at.last) continue;
        const double x = (double(g) + 0.5) * stretch, fill = 10.0 * stretch * stretch;
        tunnels.push_back({{x, z}, {x + at.flow * stretch, z}, fill, at.supply * fill, 1e9f});
    }
    return tunnels;
}

// Highest esker cell across the stream in domain column x.
float ridgeAt(const Domain& domain, const SheetWork& work, int x) {
    float most = 0;
    for (int z = Stream - 4; z <= Stream + 4; ++z) most = std::max(most, work.left.esker[domain.index(x, z)]);
    return most;
}

// The two tiles, the west one finished first: the east tile holds the cells the west tile owns at
// what its sheet left.
struct Pair {
    Domain west, east;
    SheetWork westWork, eastWork;
    Tile finished;
};

void leave(Pair& pair, const Case& at) {
    pair.west = plainAt(0, at);
    const ParentSheet westParent = sheetAt(pair.west, 0, at);
    expect(leaveFiner(pair.west, pair.westWork, westParent, tunnelsOver(pair.west, at)), "the last sheet did not pass over the west tile");
    pair.finished.sheet = presentFiner(pair.west, pair.westWork, westParent);
    pair.east = plainAt(1, at);
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = 1; x < Halo; ++x) {
            const size_t i = pair.east.index(x, z), from = pair.west.index(x + TileCells, z);
            pair.east.hold[i] = Settled;
            pair.east.ground.rock[i] = pair.west.ground.rock[from];
            for (int c = 0; c < SedimentClasses; ++c) pair.east.ground.sediment[c][i] = pair.west.ground.sediment[c][from];
        }
    pair.east.finished[3] = &pair.finished;
    leaveFiner(pair.east, pair.eastWork, sheetAt(pair.east, 1, at), tunnelsOver(pair.east, at));
}

// The ridge carries on across the seam at the height the west tile left it.
void expectSeamless(const Pair& pair, const std::string& way) {
    const float before = ridgeAt(pair.west, pair.westWork, Halo + TileCells - 1), after = ridgeAt(pair.east, pair.eastWork, Halo);
    expect(before > 1, "no ridge reaches the seam" + way, before);
    expect(std::abs(after - before) < 0.1f * before, "the ridge steps at the seam" + way, after - before);
}

// With the bed frozen only the streams move cover, and a stream that starts and ends on the two
// tiles: what one tile books as leaving the other books as arriving, and the two together lay no
// more than they took.
void expectBooksMatch(const Pair& pair, const std::string& way) {
    const TileLedger &west = pair.west.ledger, &east = pair.east.ledger;
    const double laid = west.deposited + east.deposited, tolerance = 1e-3 * laid;
    expect(laid > 0, "no ridge was laid" + way);
    expect(west.sedimentIn + west.sedimentOut > 1e-4 * laid, "no ridge crossed the seam" + way, west.sedimentIn + west.sedimentOut);
    expect(std::abs(west.sedimentIn - east.sedimentOut) < tolerance, "what the east tile sent across the seam is not what the west tile received" + way, west.sedimentIn - east.sedimentOut);
    expect(std::abs(west.sedimentOut - east.sedimentIn) < tolerance, "what the west tile sent across the seam is not what the east tile received" + way, west.sedimentOut - east.sedimentIn);
    expect(std::abs(west.eroded + east.eroded - laid) < tolerance, "the two tiles laid more ridge than they took" + way, laid - west.eroded - east.eroded);
}

} // namespace

void testIceSheetSeam() {
    // The west tile alone, with all of the fill still in the parent's cover and with half of it.
    Case whole{.supply = 1.0};
    Domain full = plainAt(0, whole);
    SheetWork fullWork;
    leaveFiner(full, fullWork, sheetAt(full, 0, whole), tunnelsOver(full, whole));
    Case eastward{.thinFrom = TileCells, .thinTo = TileCells + 8};
    Pair along;
    leave(along, eastward);
    double fullRidge = 0, halfRidge = 0;
    for (int x = Halo + 40; x < Halo + 200; ++x) fullRidge += ridgeAt(full, fullWork, x), halfRidge += ridgeAt(along.west, along.westWork, x);
    expect(fullRidge > 0 && std::abs(halfRidge / fullRidge - 0.5) < 0.1, "the ridge is not lowered by the share of its fill the parent's cover holds", halfRidge / std::max(1e-9, fullRidge));

    // Ice moving east: inside the thin cover right of the seam the east tile draws the same ridge as beyond it.
    expectSeamless(along, " (flowing east)");
    float lowest = 1e9f, highest = 0;
    for (int x = Halo; x < Halo + 24; ++x) lowest = std::min(lowest, ridgeAt(along.east, along.eastWork, x)), highest = std::max(highest, ridgeAt(along.east, along.eastWork, x));
    expect(lowest > 0.8f * highest, "the ridge sinks where this tile's own cover is thin", lowest / std::max(1e-6f, highest));
    const TileLedger& ledger = along.east.ledger;
    expect(std::abs(ledger.eroded + ledger.sedimentIn - ledger.deposited - ledger.sedimentOut) < 1e-4 * ledger.eroded, "what the sheet moved on the east tile does not add up",
           ledger.eroded + ledger.sedimentIn - ledger.deposited - ledger.sedimentOut);

    // Ice moving west: the stretch over the seam starts on the east tile, finished after the west
    // tile laid its part of it.
    Pair against;
    leave(against, Case{.flow = -1});
    expectSeamless(against, " (flowing west)");

    // Each way with the bed frozen and the stream ending on the two tiles, then with the stretch over
    // the seam starting where its cover is all but gone: its ridge stands only as high as that cover
    // gave, alike on both tiles, and nothing is laid that was not taken.
    for (const int flow : {1, -1}) {
        const std::string way = flow > 0 ? " (flowing east)" : " (flowing west)";
        Pair frozen;
        leave(frozen, Case{.flow = flow, .slide = 0, .first = 2, .last = 29});
        expectBooksMatch(frozen, way);
        const int bare = flow > 0 ? TileCells - Refinement : TileCells;   // the parent cell the stretch over the seam starts in
        Pair starved;
        leave(starved, Case{.flow = flow, .slide = 0, .thinFrom = bare, .thinTo = bare + Refinement, .first = 2, .last = 29});
        expectBooksMatch(starved, way + " from thin cover");
        const float before = ridgeAt(starved.west, starved.westWork, Halo + TileCells - 1), after = ridgeAt(starved.east, starved.eastWork, Halo);
        const float upstream = flow > 0 ? ridgeAt(starved.west, starved.westWork, Halo + TileCells - 3 * Refinement) : ridgeAt(starved.east, starved.eastWork, Halo + 3 * Refinement);
        expect(upstream > 1 && std::max(before, after) < 0.2f * upstream, "a ridge stands higher than the cover it started in could give" + way, std::max(before, after) / std::max(1e-6f, upstream));
        expect(std::abs(after - before) < 0.02f * upstream, "the lowered ridge steps at the seam" + way, after - before);
    }
}

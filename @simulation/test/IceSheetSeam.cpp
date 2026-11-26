#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Local.hpp"
#include "measure/fn/Plane.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

// The last sheet over two neighbouring Local tiles, the west one finished first: a plain falling
// east under ice sliding east, a stream under the ice along a row, its fill spread by the parent
// and only half of it still in the parent's cover. Right of the seam the cover is thin beside the
// stream, so the later tile alone could not find the fill of the stretch that straddles the seam.

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "IceSheetSeam: " << message << " (" << value << ")\n";
    std::exit(1);
}

constexpr int Stream = 140;       // row the stream under the ice follows
constexpr int Thin = 8;           // columns right of the seam where the cover beside the stream is thin

// World column of domain column x in tile tx.
int column(int tx, int x) { return x - Halo + tx * TileCells; }

Domain plainAt(int tx) {
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
            const bool thin = g >= TileCells && g < TileCells + Thin && std::abs(z - Stream) < 8;
            domain.ground.sediment[Coarse][i] = domain.ground.sediment[Sand][i] = thin ? 0.05f : 2.0f;
            domain.ground.sediment[Fines][i] = 2;
            if (x == 0 || z == 0 || x == last || z == last) domain.hold[i] = Rim;
        }
    return domain;
}

ParentSheet eastwardAt(const Domain& domain, int tx) {
    ParentSheet parent;
    parent.any = true;
    const size_t cells = domain.cells();
    parent.iceNow.assign(cells, 0.0f), parent.iceThen.assign(cells, 800.0f), parent.slideX.assign(cells, 60.0f), parent.slideZ.assign(cells, 0.0f);
    parent.worn.assign(cells, 80.0f), parent.esker.assign(cells, 0.0f), parent.topNow.assign(cells, 0.0f), parent.topThen.assign(cells, 0.0f);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            parent.topThen[i] = float(1000 - 0.001 * column(tx, x) * domain.cell);
            parent.topNow[i] = domain.ground.rock[i] + domain.sedimentAt(i);
            if (std::abs(z - Stream) < 8) parent.esker[i] = 10.0f;
        }
    return parent;
}

// The parent's stretches whose cells lie over the domain, as parentTunnels lists them.
std::vector<Tunnel> tunnelsOver(const Domain& domain, double supplyShare) {
    const double stretch = Refinement * domain.cell, z = (Stream + 0.5) * domain.cell + domain.origin.z;
    const double extent = domain.size * domain.cell;
    std::vector<Tunnel> tunnels;
    for (int64_t g = int64_t(std::floor(domain.origin.x / stretch)); g <= int64_t(std::floor((domain.origin.x + extent) / stretch)); ++g) {
        const double x = (double(g) + 0.5) * stretch, fill = 10.0 * stretch * stretch;
        tunnels.push_back({{x, z}, {x + stretch, z}, fill, supplyShare * fill, 1e9f});
    }
    return tunnels;
}

// Highest esker cell across the stream in domain column x.
float ridgeAt(const Domain& domain, const SheetWork& work, int x) {
    float most = 0;
    for (int z = Stream - 4; z <= Stream + 4; ++z) most = std::max(most, work.left.esker[domain.index(x, z)]);
    return most;
}

} // namespace

void testIceSheetSeam() {
    // The west tile alone, with all of the fill still in the parent's cover and with half of it.
    Domain full = plainAt(0);
    SheetWork fullWork;
    expect(leaveFiner(full, fullWork, eastwardAt(full, 0), tunnelsOver(full, 1.0)), "the last sheet did not pass over the west tile");
    Domain west = plainAt(0);
    const ParentSheet westParent = eastwardAt(west, 0);
    SheetWork westWork;
    leaveFiner(west, westWork, westParent, tunnelsOver(west, 0.5));
    double whole = 0, half = 0;
    for (int x = Halo + 40; x < Halo + 200; ++x) whole += ridgeAt(full, fullWork, x), half += ridgeAt(west, westWork, x);
    expect(whole > 0 && std::abs(half / whole - 0.5) < 0.1, "the ridge is not lowered by the share of its fill the parent's cover holds", half / std::max(1e-9, whole));

    // The east tile after it: the cells the west tile owns are held at what its sheet left.
    Tile finished;
    finished.sheet = presentFiner(west, westWork, westParent);
    Domain east = plainAt(1);
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = 1; x < Halo; ++x) {
            const size_t i = east.index(x, z), from = west.index(x + TileCells, z);
            east.hold[i] = Settled;
            east.ground.rock[i] = west.ground.rock[from];
            for (int c = 0; c < SedimentClasses; ++c) east.ground.sediment[c][i] = west.ground.sediment[c][from];
        }
    east.finished[3] = &finished;
    SheetWork eastWork;
    leaveFiner(east, eastWork, eastwardAt(east, 1), tunnelsOver(east, 0.5));

    // The ridge carries on across the seam at the height the west tile left it.
    const float before = ridgeAt(west, westWork, Halo + TileCells - 1), after = ridgeAt(east, eastWork, Halo);
    expect(before > 1, "no ridge reaches the seam", before);
    expect(std::abs(after - before) < 0.1f * before, "the ridge steps at the seam", after - before);
    // Inside the thin cover the east tile draws the same ridge as beyond it.
    float lowest = 1e9f, highest = 0;
    for (int x = Halo; x < Halo + 3 * Thin; ++x) lowest = std::min(lowest, ridgeAt(east, eastWork, x)), highest = std::max(highest, ridgeAt(east, eastWork, x));
    expect(lowest > 0.8f * highest, "the ridge sinks where this tile's own cover is thin", lowest / std::max(1e-6f, highest));

    // The east tile's books close: what its sheet took up and laid, and what crossed its edges.
    const TileLedger& ledger = east.ledger;
    expect(std::abs(ledger.eroded + ledger.sedimentIn - ledger.deposited - ledger.sedimentOut) < 1e-4 * ledger.eroded, "what the sheet moved on the east tile does not add up",
           ledger.eroded + ledger.sedimentIn - ledger.deposited - ledger.sedimentOut);
}

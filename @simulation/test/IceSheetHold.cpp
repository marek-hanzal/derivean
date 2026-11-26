#include "aeolian/type/Aeolian.hpp"
#include "icesheet/fn/Parent.hpp"
#include "measure/fn/Plane.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

using namespace derivean::sim;

// The parent's last-sheet deposits are set aside from a Local domain's seeded cover while it
// settles and laid back when its own last sheet leaves. A uniform parent record of till and
// outwash over a uniform cover: thin cover gives what it holds, the wind's labels keep theirs, the
// cells that follow a finished neighbour or the rim move between two ends both without the
// deposits, and laying back returns every ground, trend and lake floor to the seed.

namespace {

void expect(bool passed, const std::string& message, double value = 0) {
    if (passed) return;
    std::cerr << "IceSheetHold: " << message << " (" << value << ")\n";
    std::exit(1);
}

constexpr float Till = 1.5f, Outwash = 0.8f;

std::shared_ptr<Tile> parentTile(bool recorded) {
    auto tile = std::make_shared<Tile>();
    tile->key = {Level::Region, 0, 0};
    const size_t cells = size_t(TileCells) * TileCells;
    tile->now.resize(cells);
    for (auto& part : tile->now.sediment) part.assign(cells, 3.0f);
    if (!recorded) return tile;
    auto record = std::make_shared<SheetRecord>();
    for (auto* field : {&record->till, &record->moraine, &record->outwash, &record->esker}) field->assign(cells, 0.0f);
    record->till.assign(cells, Till);
    record->outwash.assign(cells, Outwash);
    tile->sheet = record;
    return tile;
}

// Columns 0..2 thin, 3..5 under lag, 6..8 under loess and salt; the outer ring rim, the next
// Halo − 1 columns on the west a finished neighbour's.
Domain seeded() {
    Domain domain;
    domain.key = {Level::Local, 3, 5};
    domain.cell = specOf(Level::Local).cell;
    domain.origin = {(3 * TileCells - Halo) * domain.cell, (5 * TileCells - Halo) * domain.cell};
    allocateBare(domain);
    domain.aeolian.labels.resize(domain.cells());
    domain.lakeFloor.assign(domain.cells(), -1e9f);
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            const int band = (z / 4) % 3;
            const bool thin = band == 0 && x % 9 < 3;
            domain.ground.rock[i] = domain.ground.datum[i] = 100;
            domain.ground.sediment[Coarse][i] = domain.ground.sediment[Sand][i] = thin ? 0.2f : 3.0f;
            domain.ground.sediment[Fines][i] = thin ? 0.5f : 2.0f;
            if (x % 9 >= 3 && x % 9 < 6) domain.aeolian.labels.lag[i] = 2.9f;
            if (x % 9 >= 6) domain.aeolian.labels.loess[i] = 1.2f, domain.aeolian.labels.salt[i] = 0.7f;
            domain.trend[i] = domain.ground.rock[i] + domain.sedimentAt(i);
            if (band == 1) domain.lakeFloor[i] = 104;
            if (x < Halo) domain.hold[i] = Settled;
            if (x == 0 || z == 0 || x == last || z == last) domain.hold[i] = Rim;
        }
    domain.settledFrom = domain.settledTo = domain.ground;
    return domain;
}

float cover(const Ground& ground, size_t i) { return ground.sediment[Coarse][i] + ground.sediment[Sand][i] + ground.sediment[Fines][i]; }

} // namespace

void testIceSheetHold() {
    const auto recorded = parentTile(true), bare = parentTile(false);
    {
        Domain domain = seeded();
        holdBackLastSheet(domain, [&](TileKey) { return bare.get(); });
        expect(!domain.sheet, "a parent without a record holds nothing back");
    }
    Domain domain = seeded();
    const Domain seed = domain;
    holdBackLastSheet(domain, [&](TileKey) { return recorded.get(); });
    expect(domain.sheet && domain.sheet->held[0].size() == domain.cells(), "the deposits are held");
    const WindLabels& labels = domain.aeolian.labels;
    double worst = 0;
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const bool thin = (z / 4) % 3 == 0 && x % 9 < 3;
            const float taken = cover(seed.ground, i) - cover(domain.ground, i);
            const float wanted = thin ? cover(seed.ground, i) : Till + Outwash;
            worst = std::max(worst, double(std::abs(taken - wanted)));
            for (int c = 0; c < SedimentClasses; ++c) expect(domain.ground.sediment[c][i] >= -1e-6f, "no class goes negative", domain.ground.sediment[c][i]);
            expect(domain.ground.sediment[Coarse][i] >= labels.lag[i] - 1e-5f, "the wind's lag stays", domain.ground.sediment[Coarse][i]);
            expect(domain.ground.sediment[Fines][i] >= labels.loess[i] + labels.salt[i] - 1e-5f, "the wind's loess and salt stay", domain.ground.sediment[Fines][i]);
            // Both ends of a followed cell lack the deposits, so it settles beside its Free neighbours.
            for (const Ground* end : {&domain.settledFrom, &domain.settledTo})
                expect(std::abs(cover(*end, i) - cover(domain.ground, i)) < 1e-5f, "a followed cell's ends lack the deposits", x);
            expect(std::abs(seed.trend[i] - domain.trend[i] - taken) < 1e-4f, "the surface is held without them", domain.trend[i]);
            if (seed.lakeFloor[i] > 0) expect(std::abs(seed.lakeFloor[i] - domain.lakeFloor[i] - taken) < 1e-4f, "the lake floor is held without them", domain.lakeFloor[i]);
        }
    expect(worst < 1e-4, "held back is the parent's deposits, or all the cover where it is thinner", worst);
    restoreLastSheet(domain);
    worst = 0;
    for (size_t i = 0; i < domain.cells(); ++i) {
        for (int c = 0; c < SedimentClasses; ++c)
            for (const auto ground : {&Domain::ground, &Domain::settledFrom, &Domain::settledTo})
                worst = std::max(worst, double(std::abs((domain.*ground).sediment[c][i] - (seed.*ground).sediment[c][i])));
        worst = std::max({worst, double(std::abs(domain.trend[i] - seed.trend[i])), seed.lakeFloor[i] > 0 ? double(std::abs(domain.lakeFloor[i] - seed.lakeFloor[i])) : 0.0});
    }
    expect(worst < 1e-4, "laying back returns the seed", worst);
    expect(domain.sheet->held[0].empty(), "nothing is laid back twice");
}

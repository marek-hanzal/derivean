#include "icesheet/constant/Ice.hpp"
#include "icesheet/fn/Local.hpp"
#include "measure/fn/Plane.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

// About 400 lines, one owner: the last sheet over one hand-built Local plain, varied case by case.
// The last sheet over a settled Local domain, with its parent's record given by hand: a gentle
// plain under a cover of till, ice sliding east, three knobs of rock, a fracture zone across the
// flow and a stream under the ice along a line.

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "IceSheetLocal: " << message << " (" << value << ")\n";
    std::exit(1);
}

constexpr int Zone = 150;          // column of the fracture zone
constexpr int Stream = 240;        // row the stream under the ice follows
constexpr int Lane = 200;          // row of a second fracture zone, along the flow
constexpr int Knobs[3][2] = {{80, 80}, {100, 160}, {200, 110}};

double knob(double x, double z) {
    double height = 0;
    for (const auto& at : Knobs) height += 20 * std::exp(-((x - at[0]) * (x - at[0]) + (z - at[1]) * (z - at[1])) / 18.0);
    return height;
}

Domain plain() {
    Domain domain;
    domain.key = {Level::Local, 0, 0};
    domain.cell = specOf(Level::Local).cell;
    domain.origin = {-Halo * domain.cell, -Halo * domain.cell};
    allocateBare(domain);
    domain.unit.assign(domain.cells(), UnitCache{-1e30f, 1e30f, traitsOf(Material::Granite)});
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z)
        for (int x = 0; x <= last; ++x) {
            const size_t i = domain.index(x, z);
            domain.ground.rock[i] = domain.ground.datum[i] = float(200 - 0.002 * x * domain.cell + knob(x, z));
            domain.ground.sediment[Coarse][i] = 2, domain.ground.sediment[Sand][i] = 2, domain.ground.sediment[Fines][i] = 2;
            domain.weakness[i] = std::abs(x - Zone) <= 1 || std::abs(z - Lane) <= 1 ? 1.0f : 0.0f;
            if (x == 0 || z == 0 || x == last || z == last) domain.hold[i] = Rim;
        }
    return domain;
}

ParentSheet eastward(const Domain& domain) {
    ParentSheet parent;
    parent.any = true;
    const size_t cells = domain.cells();
    parent.iceNow.assign(cells, 0.0f), parent.iceThen.assign(cells, 800.0f), parent.slideX.assign(cells, 60.0f), parent.slideZ.assign(cells, 0.0f);
    parent.worn.assign(cells, 80.0f), parent.esker.assign(cells, 0.0f), parent.topNow.assign(cells, 0.0f), parent.topThen.assign(cells, 0.0f);
    for (int z = 0; z < domain.size; ++z)
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            parent.topThen[i] = float(1000 - 0.001 * x * domain.cell);
            parent.topNow[i] = domain.ground.rock[i] + domain.sedimentAt(i);
            // The parent spread its tunnel's fill over the kilometre around the stream.
            if (std::abs(z - Stream) < 8) parent.esker[i] = 10.0f;
        }
    return parent;
}

float groundOf(const Domain& domain, int x, int z) {
    const size_t i = domain.index(x, z);
    return domain.ground.rock[i] + domain.sedimentAt(i);
}

// How far each cell stands above the highest of its eight neighbours (zero at the domain's edge).
std::vector<float> standing(const Domain& domain) {
    std::vector<float> above(domain.cells(), 0.0f);
    for (int z = 1; z + 1 < domain.size; ++z)
        for (int x = 1; x + 1 < domain.size; ++x) {
            float highest = -1e30f;
            for (int k = 0; k < 8; ++k) highest = std::max(highest, groundOf(domain, x + NeighborX[k], z + NeighborZ[k]));
            above[domain.index(x, z)] = groundOf(domain, x, z) - highest;
        }
    return above;
}

// The last sheet leaves no cell standing higher above all its neighbours than the tallest esker.
bool leaveChecked(Domain& domain, SheetWork& work, const ParentSheet& parent, const std::vector<Tunnel>& tunnels) {
    const std::vector<float> before = standing(domain);
    const bool passed = leaveFiner(domain, work, parent, tunnels);
    const std::vector<float> after = standing(domain);
    for (size_t i = 0; i < after.size(); ++i) expect(after[i] <= std::max(0.0f, before[i]) + EskerTallest, "the last sheet left a column standing over its neighbours", after[i]);
    return passed;
}

} // namespace

void testIceSheetLocal() {
    Domain domain = plain();
    const ParentSheet parent = eastward(domain);
    std::vector<Tunnel> tunnels;
    const double z = (Stream + 0.5) * domain.cell + domain.origin.z, stretch = Refinement * domain.cell;
    for (double x = domain.origin.x; x < domain.origin.x + domain.size * domain.cell; x += stretch)
        tunnels.push_back({{x, z}, {x + stretch, z}, 10.0 * stretch * stretch, 10.0 * stretch * stretch, 1e9f});
    double before = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) before += groundOf(domain, x, z);
    SheetWork work;
    expect(leaveChecked(domain, work, parent, tunnels), "the last sheet did not pass over ground its parent had under ice");

    // The tile's own ground changed by what its ledger says.
    double after = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) after += groundOf(domain, x, z);
    const double area = domain.cell * domain.cell, change = (after - before) * area;
    const TileLedger& ledger = domain.ledger;
    expect(std::abs(change - (ledger.deposited - ledger.eroded)) < 1e-4 * ledger.eroded, "the ground's change is not the ledger's", change - (ledger.deposited - ledger.eroded));
    expect(std::abs(ledger.eroded + ledger.sedimentIn - ledger.deposited - ledger.sedimentOut) < 1e-4 * ledger.eroded, "what the last sheet moved does not add up",
           ledger.eroded + ledger.sedimentIn - ledger.deposited - ledger.sedimentOut);

    // Both fracture zones are dug into basins, the one the ice ran along deeper than the one it crossed.
    const auto basin = [&](int x0, int z0, int dx, int dz) {
        const size_t at = domain.index(x0, z0), beside = domain.index(x0 + dx, z0 + dz);
        return (domain.ground.rock[beside] + 0.002f * float(dx * domain.cell)) - domain.ground.rock[at];
    };
    const float crossed = basin(Zone, 130, -6, 0), along = basin(60, Lane, 0, -6);
    expect(crossed > 3, "the fracture zone across the flow was not dug out", crossed);
    expect(along > 1.5f * crossed, "the zone the ice ran along is not dug deeper than the one it crossed", along / std::max(1e-3f, crossed));

    // Till lies in the lee of every knob, far more than on its stoss side, in a tail drawn out along the flow.
    for (const auto& at : Knobs) {
        float lee = 0, stoss = 0, across = 0;
        for (int k = 3; k <= 8; ++k) {
            lee += work.left.till[domain.index(at[0] + k, at[1])];
            stoss += work.left.till[domain.index(at[0] - k, at[1])];
            across += work.left.till[domain.index(at[0], at[1] + k)];
        }
        expect(lee > 2 * stoss, "no tail of till behind a knob", lee / std::max(1e-3f, stoss));
        expect(lee > 1.5f * across, "the tail behind a knob is not drawn out along the flow", lee / std::max(1e-3f, across));
    }

    // The stream's fill stands as a ridge along its line.
    float ridge = 0;
    for (int x = Halo; x < Halo + TileCells; x += 16) ridge += groundOf(domain, x, Stream) - 0.5f * (groundOf(domain, x, Stream - 10) + groundOf(domain, x, Stream + 10));
    ridge /= TileCells / 16;
    expect(ridge > 5, "the tunnel fill does not stand as a ridge", ridge);
    // It runs unbroken along the stream and stands no higher than eskers do, however narrow its tunnel against a cell.
    float lowest = 1e9f, highest = 0;
    for (int x = Halo; x < Halo + TileCells; ++x) {
        float across = 0;
        for (int z = Stream - 2; z <= Stream + 2; ++z) across = std::max(across, work.left.esker[domain.index(x, z)]), highest = std::max(highest, work.left.esker[domain.index(x, z)]);
        lowest = std::min(lowest, across);
    }
    expect(lowest > 1, "the esker breaks along its stream", lowest);
    expect(highest <= EskerTallest, "an esker stands higher than any on Earth", highest);

    // The stream keeps to a winding trough of its bed, whose sides fall faster than the ice surface.
    const auto trough = [](int x) { return Stream + 6.0 * std::sin(2 * M_PI * x / 128.0); };
    Domain winding = plain();
    for (int z = 0; z < winding.size; ++z)
        for (int x = 0; x < winding.size; ++x) {
            const double off = z - trough(x);
            winding.ground.rock[winding.index(x, z)] -= float(30 * std::exp(-off * off / 4.0));
        }
    SheetWork windingWork;
    leaveChecked(winding, windingWork, eastward(winding), tunnels);
    double missed = 0, straight = 0;
    int columns = 0;
    for (int x = Halo + 8; x < Halo + TileCells - 8; ++x) {
        int crest = -1;
        float top = 1;
        for (int z = Stream - 12; z <= Stream + 12; ++z)
            if (const float h = windingWork.left.esker[winding.index(x, z)]; h > top) top = h, crest = z;
        if (crest < 0) continue;
        missed += std::abs(crest - trough(x)), straight += std::abs(crest - Stream), ++columns;
    }
    expect(columns > TileCells / 2, "the esker in a winding trough is mostly missing", columns);
    expect(missed / columns < 1.5 && straight / columns > 2.5, "the esker does not wind with the trough it ran in", missed / columns);

    // Where a finished neighbour's esker comes over the edge, the stream carries on from it.
    Domain joining = plain();
    Tile west;
    auto ridged = std::make_shared<SheetRecord>();
    ridged->esker.assign(size_t(TileCells) * TileCells, 0.0f);
    for (auto& part : ridged->passing) part.assign(size_t(TileCells) * TileCells, 0.0f);
    constexpr int Beside = Stream + 5;
    for (int z = 0; z < joining.size; ++z)
        for (int x = 1; x < Halo; ++x) {
            joining.hold[joining.index(x, z)] = Settled;
            if (std::abs(z - Beside) <= 1 && z >= Halo && z < Halo + TileCells) ridged->esker[size_t(z - Halo) * TileCells + size_t(x - Halo + TileCells)] = 5.0f;
        }
    west.sheet = ridged;
    joining.finished[3] = &west;
    SheetWork joiningWork;
    leaveChecked(joining, joiningWork, eastward(joining), tunnels);
    int crest = -1;
    float top = 1;
    for (int z = Stream - 12; z <= Stream + 12; ++z)
        if (const float h = joiningWork.left.esker[joining.index(Halo + 1, z)]; h > top) top = h, crest = z;
    expect(std::abs(crest - Beside) <= 1, "the esker does not carry on from the one a finished neighbour left at the edge", crest - Beside);

    // Running into a finished neighbour that recorded its esker beside the route, the stream ends on it.
    Domain downstream = plain();
    Tile east;
    auto ahead = std::make_shared<SheetRecord>();
    ahead->esker.assign(size_t(TileCells) * TileCells, 0.0f);
    for (auto& part : ahead->passing) part.assign(size_t(TileCells) * TileCells, 0.0f);
    for (int z = 0; z < downstream.size; ++z)
        for (int x = Halo + TileCells; x < downstream.size - 1; ++x) {
            downstream.hold[downstream.index(x, z)] = Settled;
            if (std::abs(z - Beside) <= 1 && z >= Halo && z < Halo + TileCells) ahead->esker[size_t(z - Halo) * TileCells + size_t(x - Halo - TileCells)] = 5.0f;
        }
    east.sheet = ahead;
    downstream.finished[5] = &east;
    SheetWork downstreamWork;
    leaveChecked(downstream, downstreamWork, eastward(downstream), tunnels);
    crest = -1, top = 1;
    for (int z = Stream - 12; z <= Stream + 12; ++z)
        if (const float h = downstreamWork.left.esker[downstream.index(Halo + TileCells - 2, z)]; h > top) top = h, crest = z;
    expect(std::abs(crest - Beside) <= 1, "the esker does not end on the one a finished neighbour left at the edge", crest - Beside);

    // Where the head rises over a ridge of rock across the stream, the water ponded and left no
    // esker; beyond, the esker carries on.
    constexpr int Sill = 150;
    Domain sill = plain();
    for (int z = 0; z < sill.size; ++z)
        for (int x = 0; x < sill.size; ++x) {
            // Askew across the stream, so the head also falls along the sill.
            const double off = x - Sill + 0.3 * (z - Stream);
            sill.ground.rock[sill.index(x, z)] += float(60 * std::exp(-off * off / 8.0));
        }
    SheetWork sillWork;
    leaveChecked(sill, sillWork, eastward(sill), tunnels);
    const auto across = [&](int x) {
        float most = 0;
        for (int z = Stream - 8; z <= Stream + 8; ++z) most = std::max(most, sillWork.left.esker[sill.index(x, z)]);
        return most;
    };
    float beyond = 1e9f;
    for (int x = Sill + 10; x < Halo + TileCells; ++x) beyond = std::min(beyond, across(x));
    expect(across(Sill - 10) < 1 && across(Sill - 30) < 1, "an esker stands where its water ponded behind a rise", std::max(across(Sill - 10), across(Sill - 30)));
    expect(beyond > 1, "the esker does not carry on beyond the pond", beyond);

    // A smaller stream joining the trunk from the north-west leaves an esker that reaches the trunk's.
    std::vector<Tunnel> withTributary = tunnels;
    const double trunkX = 12 * stretch + domain.origin.x;
    for (int k = 2; k >= 1; --k) withTributary.push_back({{trunkX - k * stretch, z - k * stretch}, {trunkX - (k - 1) * stretch, z - (k - 1) * stretch}, 10.0 * stretch * stretch, 10.0 * stretch * stretch, 1e8f});
    Domain confluence = plain();
    ParentSheet fed = eastward(confluence);
    // The parent spread the tributary's fill along its line too.
    const int joinX = int((trunkX - confluence.origin.x) / confluence.cell);
    for (int z = 0; z < confluence.size; ++z)
        for (int x = joinX - 2 * Refinement - 8; x <= joinX; ++x)
            if (std::abs((x - joinX) - (z - Stream)) < 11) fed.esker[confluence.index(x, z)] = 10.0f;
    SheetWork confluenceWork;
    leaveChecked(confluence, confluenceWork, fed, withTributary);
    std::vector<bool> reached(confluence.cells(), false);
    std::vector<size_t> open{confluence.index(Halo + TileCells / 2 + 40, Stream)};
    for (int z = Stream - 2; z <= Stream + 2; ++z)
        if (confluenceWork.left.esker[confluence.index(Halo + TileCells / 2 + 40, z)] > 1) open.push_back(confluence.index(Halo + TileCells / 2 + 40, z));
    while (!open.empty()) {
        const size_t c = open.back();
        open.pop_back();
        if (reached[c] || confluenceWork.left.esker[c] <= 1) continue;
        reached[c] = true;
        for (int k = 0; k < 8; ++k) {
            const int nx = int(c % size_t(confluence.size)) + NeighborX[k], nz = int(c / size_t(confluence.size)) + NeighborZ[k];
            if (nx >= 0 && nz >= 0 && nx < confluence.size && nz < confluence.size) open.push_back(confluence.index(nx, nz));
        }
    }
    bool upstream = false;
    for (int dz = -8; dz <= 8; ++dz)
        for (int dx = -8; dx <= 8; ++dx) upstream = upstream || reached[confluence.index(int((trunkX - 1.5 * stretch - confluence.origin.x) / confluence.cell) + dx, Stream - 24 + dz)];
    expect(upstream, "the esker of a stream joining the trunk does not reach the trunk's");

    // A tile whose up-ice halo a finished neighbour owns, holding what the last sheet left there,
    // gets the till the same ice brings it.
    Domain later = plain();
    for (int z = 0; z < later.size; ++z)
        for (int x = 1; x < Halo; ++x) {
            const size_t i = later.index(x, z);
            later.hold[i] = Settled;
            later.ground.rock[i] = domain.ground.rock[i];
            for (int c = 0; c < SedimentClasses; ++c) later.ground.sediment[c][i] = domain.ground.sediment[c][i];
        }
    // The neighbour's ice carried on from those cells what this domain's own ice did.
    Tile neighbour;
    auto record = std::make_shared<SheetRecord>();
    for (auto& part : record->passing) part.assign(size_t(TileCells) * TileCells, 0.0f);
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = 0; x < Halo; ++x)
            for (int c = 0; c < SedimentClasses; ++c) record->passing[c][size_t(z - Halo) * TileCells + size_t(x - Halo + TileCells)] = work.left.passing[c][domain.index(x, z)];
    neighbour.sheet = record;
    later.finished[3] = &neighbour;
    SheetWork laterWork;
    leaveChecked(later, laterWork, parent, {});
    double near = 0, alone = 0;
    for (int z = 40; z < 70; ++z)
        for (int x = Halo; x < Halo + 6; ++x) near += laterWork.left.till[later.index(x, z)], alone += work.left.till[domain.index(x, z)];
    expect(std::abs(near / alone - 1) < 0.1, "till steps where a finished neighbour's ice enters the tile", near / alone);

    const TileLedger& joined = later.ledger;
    expect(std::abs(joined.eroded + joined.sedimentIn - joined.deposited - joined.sedimentOut) < 1e-4 * joined.eroded, "what the ice brought from the neighbour does not add up",
           joined.eroded + joined.sedimentIn - joined.deposited - joined.sedimentOut);

    // Where the ice ended, what it brought stands as moraine no steeper than till, and the meltwater
    // spreads the rest beyond.
    Domain margin = plain();
    ParentSheet ending = eastward(margin);
    double untouched = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) untouched += groundOf(margin, x, z);
    constexpr int End = Halo + 180;
    for (int z = 0; z < margin.size; ++z)
        for (int x = End; x < margin.size; ++x) ending.iceThen[margin.index(x, z)] = 0;
    SheetWork marginWork;
    leaveChecked(margin, marginWork, ending, {});
    float moraine = 0, steepest = 0, spread = 0;
    for (int z = Halo + 20; z < Halo + 200; ++z) {
        moraine = std::max(moraine, marginWork.left.moraine[margin.index(End, z)]);
        steepest = std::max(steepest, (groundOf(margin, End, z) - groundOf(margin, End + 1, z)) / float(margin.cell));
        for (int x = End + 2; x < End + 40; ++x) spread += marginWork.left.outwash[margin.index(x, z)];
    }
    expect(moraine > 1, "no moraine where the ice ended", moraine);
    expect(steepest <= TillRepose * 1.01f, "the moraine stands steeper than till", steepest);
    expect(spread > 0, "no outwash beyond the moraine", spread);
    const TileLedger& ended = margin.ledger;
    double left = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x) left += groundOf(margin, x, z);
    const double moved = (left - untouched) * area - (ended.deposited - ended.eroded);
    expect(std::abs(moved) < 1e-4 * ended.deposited, "the ground at the ice's end changed by other than its ledger", moved);
    expect(std::abs(ended.eroded + ended.sedimentIn - ended.deposited - ended.sedimentOut) < 1e-4 * ended.eroded, "what the ice left at its end does not add up",
           ended.eroded + ended.sedimentIn - ended.deposited - ended.sedimentOut);

    // An ice margin on land beside the sea: what the meltwater brings fills the sea floor no higher
    // than the sea, and the ledger closes.
    Domain shore = plain();
    shore.seaLevel = 199;
    // A thick cover for the ice to bring down to the shore, and a valley that gathers all the
    // meltwater into one narrow inlet of the sea.
    for (int z = 0; z < shore.size; ++z)
        for (int x = 0; x < shore.size; ++x) {
            const size_t i = shore.index(x, z);
            for (int c = 0; c < SedimentClasses; ++c) shore.ground.sediment[c][i] = 10;
            shore.ground.rock[i] += float(0.05 * std::abs(z - Stream) * shore.cell);
        }
    ParentSheet ashore = eastward(shore);
    for (int z = 0; z < shore.size; ++z)
        for (int x = End; x < shore.size; ++x) ashore.iceThen[shore.index(x, z)] = 0;
    std::vector<bool> sea(shore.cells());
    for (size_t i = 0; i < shore.cells(); ++i) sea[i] = shore.ground.rock[i] + shore.sedimentAt(i) < float(shore.seaLevel);
    SheetWork shoreWork;
    leaveChecked(shore, shoreWork, ashore, {});
    float aboveSea = -1e9f, laid = 0;
    for (int z = Halo; z < Halo + TileCells; ++z)
        for (int x = Halo; x < Halo + TileCells; ++x)
            if (sea[shore.index(x, z)]) aboveSea = std::max(aboveSea, groundOf(shore, x, z) - float(shore.seaLevel)), laid += shoreWork.left.outwash[shore.index(x, z)];
    expect(laid > 0, "no outwash reached the sea", laid);
    expect(aboveSea < 0.5f, "outwash stands above the sea it was laid in", aboveSea);
    const TileLedger& shored = shore.ledger;
    expect(std::abs(shored.eroded + shored.sedimentIn - shored.deposited - shored.sedimentOut) < 1e-4 * shored.eroded, "what the ice left at the shore does not add up",
           shored.eroded + shored.sedimentIn - shored.deposited - shored.sedimentOut);

    // A hollow under active ice fills with till; under thin ice that stagnated it stays open as a kettle.
    const auto hollow = [&](float thickness) {
        Domain pitted = plain();
        constexpr int At[2] = {120, 100};
        for (int dz = -1; dz <= 1; ++dz)
            for (int dx = -1; dx <= 1; ++dx) pitted.ground.rock[pitted.index(At[0] + dx, At[1] + dz)] -= 6.0f;
        ParentSheet sheet = eastward(pitted);
        sheet.iceThen.assign(pitted.cells(), thickness);
        SheetWork pittedWork;
        leaveChecked(pitted, pittedWork, sheet, {});
        float rim = 1e9f;
        for (int k = 0; k < 8; ++k) rim = std::min(rim, groundOf(pitted, At[0] + 2 * NeighborX[k], At[1] + 2 * NeighborZ[k]));
        return rim - groundOf(pitted, At[0], At[1]);
    };
    const float filled = hollow(800), kettle = hollow(StagnantIce - 50);
    expect(kettle > 3, "a hollow under stagnant ice did not stay open", kettle);
    expect(filled < 0.5f * kettle, "active ice did not fill a hollow more than stagnant ice", filled / std::max(1e-3f, kettle));

    // A hollow in the ice surface keeps no more of what the ice brings into it than till stands
    // in; the meltwater takes the rest (the check in leaveChecked), and the ledger still closes.
    Domain bowl = plain();
    ParentSheet dipped = eastward(bowl);
    for (int z = 0; z < bowl.size; ++z)
        for (int x = 0; x < bowl.size; ++x) dipped.topThen[bowl.index(x, z)] -= float(60 * std::exp(-((x - 150.0) * (x - 150.0) + (z - 100.0) * (z - 100.0)) / 200.0));
    SheetWork bowlWork;
    leaveChecked(bowl, bowlWork, dipped, {});
    const TileLedger& pooled = bowl.ledger;
    expect(std::abs(pooled.eroded + pooled.sedimentIn - pooled.deposited - pooled.sedimentOut) < 1e-4 * pooled.eroded, "what a hollow in the ice surface kept does not add up",
           pooled.eroded + pooled.sedimentIn - pooled.deposited - pooled.sedimentOut);

    // Ground no sheet reached is left alone and carries no record.
    Domain bare = plain();
    ParentSheet none = eastward(bare);
    none.iceThen.assign(bare.cells(), 0.0f);
    SheetWork bareWork;
    expect(!leaveFiner(bare, bareWork, none, {}), "the last sheet passed over ground it never reached");
    expect(!presentFiner(bare, bareWork, none), "ground no sheet reached carries a record");
}

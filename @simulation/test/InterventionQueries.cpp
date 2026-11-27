#include "BuildGraphFixture.hpp"
#include "InterventionShared.hpp"

#include "hydrology/constant/Water.hpp"
#include "intervention/fn/Intervention.hpp"
#include "simulation/fn/Live.hpp"

#include <memory>

using namespace derivean::sim;

namespace {

// A site cell whose edits reach (EditReach) stay within the first site tile.
constexpr int64_t Middle = 136;
constexpr double Cell = specOf(Level::Site).cell;
constexpr double Edited = 10.0;   // calendar second of the edits
constexpr double Spread = 60.0;   // s the water is then left to move, so that a solver step stores the grid

Vec2 centreOf(int64_t gx, int64_t gz) { return {(double(gx) + 0.5) * Cell, (double(gz) + 0.5) * Cell}; }

// A level tile around the middle cell: rock at 100 m under 1 m of one class of cover, the water
// standing at `water` (below the ground: dry).
std::shared_ptr<const Tile> flat(Level level, SedimentClass cover, float water) {
    constexpr size_t Cells = size_t(TileCells) * TileCells;
    auto tile = std::make_shared<Tile>();
    tile->key = tileAt(level, centreOf(Middle, Middle));
    tile->now.resize(Cells);
    tile->now.rock.assign(Cells, 100);
    tile->now.datum.assign(Cells, 100);
    tile->now.sediment[cover].assign(Cells, 1);
    for (auto* field : {&tile->discharge, &tile->load, &tile->rain, &tile->eroded, &tile->erosionRate, &tile->deposited, &tile->depositionRate})
        field->assign(Cells, 0);
    tile->warmth.assign(Cells, 8);
    tile->water.assign(Cells, water);
    tile->seaLevel = 0;
    tile->rock.assign(Cells, uint8_t(Material::Granite));
    tile->flowTo.assign(Cells, 255);
    return tile;
}

// The site and ground tiles over the middle cell and a dry aquifer under it: all the voxels read.
void seedFlat(World& world, SedimentClass cover, float water) {
    BuildGraphFixture::seed(world, flat(Level::Site, cover, water));
    BuildGraphFixture::seed(world, flat(Level::Ground, cover, water));
    auto aquifer = std::make_shared<GroundwaterTile>();
    aquifer->key = tileAt(Level::Local, centreOf(Middle, Middle));
    BuildGraphFixture::seed(world, std::move(aquifer));
}

Material voxelAt(World& world, int64_t gx, int64_t gz, double y) {
    const Vec2 at = centreOf(gx, gz);
    return materialAt(world, at.x, y, at.z);
}

// One lattice node at the centre of a site cell, 1 m of every class on rock and datum at 100 m.
struct Node {
    bool moved{};
    float rock{}, datum{}, cover[SedimentClasses]{};
};

Node nodeAt(const World& world, int64_t gx, int64_t gz) {
    GroundLattice lattice;
    lattice.spacing = Cell, lattice.shift = 0.5 * Cell;
    lattice.originX = gx, lattice.originZ = gz, lattice.countX = lattice.countZ = 1;
    lattice.rock = lattice.datum = {100.0f};
    lattice.crust = {0.0f};
    for (auto& layer : lattice.cover) layer = {1.0f};
    Node node;
    node.moved = changeLattice(world, lattice);
    node.rock = lattice.rock[0], node.datum = lattice.datum[0];
    for (int c = 0; c < SedimentClasses; ++c) node.cover[c] = lattice.cover[c][0];
    return node;
}

bool nodeIs(const Node& node, bool moved, float rock, float datum, float coarse, float sand, float fines) {
    return node.moved == moved && node.rock == rock && node.datum == datum && node.cover[Coarse] == coarse && node.cover[Sand] == sand && node.cover[Fines] == fines;
}

// The cells the voxel checks use, apart so that one cell's edit does not reach another's voxels.
constexpr int64_t Swapped = Middle, Cut = Middle + 20, Sunk = Middle - 20, Kept = Middle + 20;   // Kept is a row, the others columns

// Every class of cover moves the voxels on its own: the same height may hold another material.
void expectNodes(const World& world, const char* when) {
    const Node swapped = nodeAt(world, Swapped, Middle), cut = nodeAt(world, Cut, Middle), sunk = nodeAt(world, Sunk, Middle), kept = nodeAt(world, Middle, Kept);
    expectLive(nodeIs(swapped, true, 100, 100, 1, 0, 2), when, swapped.cover[Fines]);
    expectLive(nodeIs(cut, true, 99.5f, 100, 1, 0, 1), when, cut.rock);
    expectLive(nodeIs(sunk, true, 100, 99.5f, 1, 1, 1), when, sunk.datum);
    expectLive(nodeIs(kept, false, 100, 100, 1, 1, 1), when, kept.cover[Sand]);
}

// Sand swapped for fines at the same height reads as fines, in the lattice and the voxels, and
// so it does again when a jump replays the edits. Beside it: a cut into the rock, a floor that
// founders under its layering and a cell of the live tile no edit touched.
void testCoverSwap() {
    World control({.seed = 42, .age = 0, .threads = 1});
    seedFlat(control, Fines, 99);
    const Material fines = voxelAt(control, Swapped, Middle, 100.5);
    World world({.seed = 42, .age = 0, .threads = 1});
    seedFlat(world, Sand, 99);
    const Material sand = voxelAt(world, Swapped, Middle, 100.5);
    expectLive(sand != fines, "the flat fixture reads sand and fines as one material", double(sand));

    advance(world, Edited);
    intervene(world, siteCell(Swapped, Middle, 100.0));
    intervene(world, siteCell(Swapped, Middle, 101.0, Fines));
    intervene(world, siteCell(Cut, Middle, 99.5));
    auto sink = std::make_shared<Deposit>();
    sink->origin = {double(Sunk) * Cell, double(Middle) * Cell};
    sink->cell = Cell;
    sink->width = sink->height = 1;
    sink->drop = {0.0f};
    sink->sink = {0.5f};
    intervene(world, deposit(sink));
    const SiteColumn swapped = siteColumn(world, Swapped, Middle);
    expectLive(swapped.live && swapped.ground == 101 && swapped.cover[Sand] == 0 && swapped.cover[Fines] == 1, "the swap did not leave 1 m of fines at the old height", swapped.ground);
    expectNodes(world, "the lattice does not follow what the edits changed");
    expectLive(voxelAt(world, Swapped, Middle, 100.5) == fines, "sand swapped for fines still reads as sand", double(voxelAt(world, Swapped, Middle, 100.5)));

    jumpTo(world, 0);
    expectLive(!nodeAt(world, Swapped, Middle).moved && voxelAt(world, Swapped, Middle, 100.5) == sand, "a jump before the swap does not read the untouched sand");
    jumpTo(world, Edited);
    expectNodes(world, "the replayed lattice does not follow what the edits changed");
    expectLive(voxelAt(world, Swapped, Middle, 100.5) == fines, "replayed, sand swapped for fines still reads as sand", double(voxelAt(world, Swapped, Middle, 100.5)));
}

// m, where the water of a column stands as the queries give it: below the ground where it is dry.
float levelOf(float ground, double depth) { return depth > OpenWater ? float(double(ground) + depth) : ground - 1.0f; }

// The live grid, the site column and the surface read at site and ground level give the same
// ground and the same water on every cell around the middle one; returns that water there.
float expectAgreed(World& world, const char* when) {
    const LiveSet* set = world.interventions.overlay.sets.empty() ? nullptr : &world.interventions.overlay.sets.front();
    expectLive(world.interventions.overlay.sets.size() <= 1, "the flat tile is in more than one group", double(world.interventions.overlay.sets.size()));
    float middle = 0;
    for (int64_t gz = Middle - 2; gz <= Middle + 2; ++gz)
        for (int64_t gx = Middle - 2; gx <= Middle + 2; ++gx) {
            const SiteColumn column = siteColumn(world, gx, gz);
            const float level = levelOf(column.ground, column.depth);
            if (gx == Middle && gz == Middle) middle = level;
            if (set) {
                const size_t at = size_t(gz - int64_t(set->originZ) * TileCells) * size_t(set->grid.width) + size_t(gx - int64_t(set->originX) * TileCells);
                expectLive(set->grid.ground[at] == column.ground && levelOf(set->grid.ground[at], set->grid.depth[at]) == level, when, levelOf(set->grid.ground[at], set->grid.depth[at]) - level);
            }
            for (const Level finest : {Level::Site, Level::Ground}) {
                const SurfaceSample sample = surfaceAt(world, centreOf(gx, gz), finest);
                expectLive(sample.found && sample.level == finest && sample.ground == column.ground && sample.water == level, when, double(sample.water) - double(level));
            }
        }
    return middle;
}

// A wet bed lowered by a metre keeps its depth and stands a metre lower in every query: at once,
// after the groups are brought to the same second, and when a jump replays the cut. Once the
// water has moved, the queries still agree.
void testLoweredBed() {
    World world({.seed = 42, .age = 0, .threads = 1});
    seedFlat(world, Sand, 103);
    advance(world, Edited);
    expectLive(expectAgreed(world, "the untouched water does not agree") == 103, "the untouched water does not stand at 103 m");
    intervene(world, siteCell(Middle, Middle, 100.0));
    expectLive(expectAgreed(world, "the lowered bed's water does not agree") == 102, "the lowered bed's water does not stand at 102 m");
    syncLive(world);
    expectLive(expectAgreed(world, "the lowered bed's water does not agree once synced") == 102, "the synced bed's water does not stand at 102 m");
    jumpTo(world, 0);
    expectLive(expectAgreed(world, "the water before the cut does not agree") == 103, "the water before the cut does not stand at 103 m");
    jumpTo(world, Edited);
    expectLive(expectAgreed(world, "the replayed bed's water does not agree") == 102, "the replayed bed's water does not stand at 102 m");
    advance(world, Spread);
    expectAgreed(world, "the bed's water does not agree once it has moved");
}

// A cell raised out of 2 m of water pushes that water onto the cell beside it, which then stands
// 2 m higher in every query until the solvers spread it; and so again when a jump replays it.
// Once the water has moved, the queries still agree.
void testPushedWater() {
    World world({.seed = 42, .age = 0, .threads = 1});
    seedFlat(world, Sand, 103);
    advance(world, Edited);
    const auto expectPushed = [&](const char* when) {
        expectAgreed(world, when);
        const SiteColumn raised = siteColumn(world, Middle, Middle);
        expectLive(raised.ground == 104 && raised.depth == 0, "the raised cell is not dry at 104 m", raised.depth);
        int risen = 0;
        for (int64_t gz = Middle - 1; gz <= Middle + 1; ++gz)
            for (int64_t gx = Middle - 1; gx <= Middle + 1; ++gx) risen += levelOf(siteColumn(world, gx, gz).ground, siteColumn(world, gx, gz).depth) == 105;
        expectLive(risen == 1, "the pushed water does not stand on one cell beside the raised one", risen);
    };
    intervene(world, siteCell(Middle, Middle, 104.0));
    expectPushed("the pushed water does not agree");
    syncLive(world);
    expectPushed("the pushed water does not agree once synced");
    jumpTo(world, 0);
    expectAgreed(world, "the water before the raise does not agree");
    jumpTo(world, Edited);
    expectPushed("the replayed pushed water does not agree");
    advance(world, Spread);
    expectAgreed(world, "the pushed water does not agree once it has moved");
}

} // namespace

// What the voxels and the surface read of an edited cell is what its column and its group's grid
// hold, on a flat fixture of its own.
void testInterventionQueries() {
    testCoverSwap();
    testLoweredBed();
    testPushedWater();
}

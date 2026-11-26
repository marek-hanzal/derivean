#include "GroundwaterFixture.hpp"
#include "LandscapeFixture.hpp"

#include "groundwater/fn/Inflow.hpp"
#include "groundwater/fn/Regional.hpp"
#include "simulation/type/World.hpp"

#include <cmath>
#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr int Field = RegionalCells;

// A plateau 400 m high cut by two rivers 100 m high running north–south 128 region cells apart,
// at the field's columns 128 and 256, both inside the tile's own cells (64..319). Recharge
// 0.1 m/yr everywhere off the rivers.
std::vector<RegionalColumn> plateau(float transmissivity) {
    const float area = float(specOf(Level::Region).cell * specOf(Level::Region).cell);
    std::vector<RegionalColumn> field(size_t(Field) * Field);
    for (int z = 0; z < Field; ++z)
        for (int x = 0; x < Field; ++x) {
            RegionalColumn& column = field[size_t(z) * Field + size_t(x)];
            const bool river = x == 128 || x == 256;
            column.ground = river ? 100.0f : 400.0f;
            column.stage = column.ground;
            column.outlet = river ? 1 : 0;
            column.recharge = river ? 0.0f : 0.1f * area;
            column.transmissivity = transmissivity;
        }
    return field;
}

void checkBudget(const RegionalTile& tile, const char* what) {
    const RegionalBudget& b = tile.budget;
    const double closes = std::abs(b.recharge - b.risen - b.exported - b.lag) / std::max(1.0, b.recharge);
    expectWater(closes < 1e-6, what, closes);
    double up = 0;
    for (float value : tile.upwelling) up += value;
    expectWater(std::abs(up - b.arriving) <= 1e-6 * std::max(1.0, up), "the regional upwelling of a tile's cells is not what its budget says arrives", up - b.arriving);
}

int64_t floorMod(int64_t value, int64_t by) { return ((value % by) + by) % by; }

// The field of local tile `key` by hand, by global cell: ground falling to the south-east, a
// draining stream on every 37th column and a pond every 64 cells; nothing else drains.
WaterField handColumns(TileKey key) {
    WaterField field;
    field.key = key;
    field.firstX = int64_t(key.x) * TileCells - field.margin, field.firstZ = int64_t(key.z) * TileCells - field.margin;
    field.column.assign(field.cells(), AquiferColumn{});
    for (int z = 0; z < field.size; ++z)
        for (int x = 0; x < field.size; ++x) {
            const int64_t gx = field.firstX + x, gz = field.firstZ + z;
            AquiferColumn& column = field.column[field.index(x, z)];
            column.ground = 200.0f - 0.01f * float(gx + gz - 2 * 4000);
            if (floorMod(gx, 37) == 0) column.drain = Drain::Stream, column.stream = 4.0f * DrainingFlow;
            if (floorMod(gx, 64) == 5 && floorMod(gz, 64) == 5) column.drain = Drain::Still;
        }
    return field;
}

void checkInflow() {
    // A region tile with every one of its cells sending 1e6 m³/yr up.
    RegionalTile region;
    region.key = {Level::Region, -1, 2};
    const size_t cells = size_t(TileCells) * TileCells;
    region.upwelling.assign(cells, 1.0e6f), region.rate.assign(cells, 0.2f), region.high.assign(cells, 900.0f), region.far.assign(cells, 3.0e4f), region.deep.assign(cells, 250.0f);
    // Two neighbouring local tiles whose fields lie inside it.
    const TileKey west{Level::Local, -13, 37}, east{Level::Local, -12, 37};
    WaterField a = handColumns(west), b = handColumns(east);
    addRegionalInflow(a, {&region});
    addRegionalInflow(b, {&region});

    // All the upwelling of the region cells a field holds, and only into cells that drain or, in a
    // region cell with none, into its lowest.
    double total = 0;
    const int blocks = a.size / Refinement;
    for (int bz = 0; bz < blocks; ++bz)
        for (int bx = 0; bx < blocks; ++bx) {
            int drains = 0, fed = 0, fedDrains = 0;
            for (int z = bz * Refinement; z < (bz + 1) * Refinement; ++z)
                for (int x = bx * Refinement; x < (bx + 1) * Refinement; ++x) {
                    const size_t i = a.index(x, z);
                    const AquiferColumn& column = a.column[i];
                    drains += column.drain != Drain::None;
                    if (column.regional <= 0) continue;
                    total += column.regional, ++fed, fedDrains += column.drain != Drain::None;
                    const bool lowest = x == (bx + 1) * Refinement - 1 && z == (bz + 1) * Refinement - 1;
                    expectWater(column.drain != Drain::None || lowest, "regional water comes up in a local cell that neither drains nor is its region cell's lowest", x + 1000.0 * z);
                    const Borne& borne = a.upwelled[i];
                    expectWater(borne.far == 3.0e4f && borne.rate == 0.2f && borne.high == 900.0f && borne.deep == 250.0f, "regional water forgets where it went down", x);
                }
            expectWater(drains > 0 ? fed == drains && fedDrains == drains : fed == 1, "regional water is not spread over the drains of its region cell", bx + 1000.0 * bz);
        }
    const double expected = 1.0e6 * double(blocks) * double(blocks);
    expectWater(std::abs(total - expected) <= 1e-5 * expected, "a field does not take in the upwelling of the region cells under it", total - expected);

    // Both fields give the cells they share the same inflow.
    const int shift = TileCells;
    for (int z = 0; z < a.size; ++z)
        for (int x = shift; x < a.size; ++x) {
            const AquiferColumn &p = a.column[a.index(x, z)], &q = b.column[b.index(x - shift, z)];
            expectWater(p.regional == q.regional, "two fields give a cell they share different regional inflow", x);
        }

    // No region tile under it: nothing comes up.
    RegionalTile elsewhere = region;
    elsewhere.key = {Level::Region, 5, 5};
    WaterField c = handColumns(west);
    addRegionalInflow(c, {&elsewhere});
    expectWater(c.upwelled.empty(), "a field takes regional water from a region tile not under it", 0);
    std::printf("ground water: regional inflow of the field of local tile %d %d: %.3g m3/yr from its %d region cells, the same in its neighbour's field\n", west.x, west.z, total,
                blocks * blocks);
}

} // namespace

void testGroundwaterRegion() {
    // The share that joins the regional flow grows with transmissivity and is most under a tight bed.
    expectWater(deepShareOf(0, false) > 0 && deepShareOf(1e4f, false) < deepShareOf(1e6f, false) && deepShareOf(1e9f, false) < 1.0f, "the deep share is not a growing share of transmissivity",
                deepShareOf(1e6f, false));
    expectWater(deepShareOf(1e4f, true) >= 0.7f && deepShareOf(1e4f, true) >= deepShareOf(1e4f, false), "water under a tight bed does not mostly join the regional flow", deepShareOf(1e4f, true));

    // Open rock: the regional water stands low enough to reach the rivers from everywhere.
    const TileKey key{Level::Region, 0, 0};
    const std::shared_ptr<RegionalTile> open = solveRegional(key, plateau(5.0e5f));
    checkBudget(*open, "the regional flow of open rock loses water");
    expectWater(open->budget.lag == 0 && open->budget.risen > 0.999 * open->budget.recharge, "water of open rock between two rivers does not all come up at them",
                open->budget.risen / open->budget.recharge);
    double atRivers = 0, farthest = 0;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = size_t(z) * TileCells + size_t(x);
            if (open->upwelling[i] <= 0) continue;
            expectWater(open->outlet[i] == 1, "water of open rock comes up away from the rivers", x);
            atRivers += open->upwelling[i], farthest = std::max(farthest, double(open->far[i]));
            expectWater(open->head[i] == 100.0f && open->rate[i] > 0.099f && open->rate[i] < 0.101f, "a river's upwelling does not say where its water went down", open->rate[i]);
        }
    // Between the rivers water travels up to 64 cells; outside them from the field's rim, 128.
    expectWater(farthest > 20e3 && farthest < 70e3, "the regional water's mean path is not tens of kilometres", farthest);
    const size_t mid = size_t(100) * TileCells + size_t(128);   // own cell midway between the rivers: field column 192
    expectWater(open->head[mid] > 100.0f && open->head[mid] < 400.0f, "the regional head between rivers is not between their stage and the ground", open->head[mid]);

    // Tight rock: the table meets the ground a few kilometres from the rivers and the water comes up there.
    const std::shared_ptr<RegionalTile> tight = solveRegional(key, plateau(2.0e4f));
    checkBudget(*tight, "the regional flow of tight rock loses water");
    double seeps = 0, rivers = 0;
    for (size_t i = 0; i < tight->upwelling.size(); ++i) (tight->outlet[i] == 4 ? seeps : rivers) += tight->upwelling[i];
    expectWater(seeps > 0.5 * tight->budget.arriving && rivers > 0, "in tight rock the regional water does not mostly come up where its table meets the ground", seeps / tight->budget.arriving);
    // The same tight rock under a tight bed: no water comes up off the rivers, so it stands above
    // the ground between them.
    std::vector<RegionalColumn> covered = plateau(2.0e4f);
    for (RegionalColumn& column : covered) column.confined = !column.outlet;
    const std::shared_ptr<RegionalTile> held = solveRegional(key, covered);
    checkBudget(*held, "the regional flow under a tight bed loses water");
    float highest = 0;
    for (size_t i = 0; i < held->upwelling.size(); ++i) {
        expectWater(held->upwelling[i] <= 0 || held->outlet[i] == 1, "water under a tight bed comes up away from the rivers", double(i));
        highest = std::max(highest, held->head[i]);
    }
    expectWater(held->budget.risen > 0.999 * held->budget.recharge && highest > 400.0f, "water under a tight bed does not all reach the rivers under pressure", highest);

    std::printf("ground water: regional plateau: open rock %.1f %% at the rivers, mean path up to %.1f km, head %.0f m between them; tight rock %.1f %% where the table meets the ground; under a tight bed all at the rivers, head up to %.0f m\n",
                100 * atRivers / open->budget.arriving, farthest / 1000, open->head[mid], 100 * seeps / tight->budget.arriving, highest);

    checkInflow();

    // A real region tile: its budget closes and part of its water comes up inside it.
    World& world = testWorld();
    const TileKey real = tileAt(Level::Region, testFocus());
    const std::shared_ptr<const RegionalTile> tile = world.groundwater.requireRegional(world.atlas, real);
    checkBudget(*tile, "the regional flow of a real region tile loses water");
    const RegionalBudget& b = tile->budget;
    expectWater(b.recharge > 0 && b.arriving > 0, "a real region tile has no regional flow", b.recharge);
    int outlets[5] = {};
    for (size_t i = 0; i < tile->upwelling.size(); ++i)
        if (tile->upwelling[i] > 0) ++outlets[tile->outlet[i]];
    const double area = tileSpan(Level::Region) * tileSpan(Level::Region);
    std::printf("ground water: region tile %d %d: deep recharge %.1f mm/yr, %.1f %% comes up in the tile, %.1f %% beyond it, %.1f %% beyond the field; "
                "up at %d river, %d lake, %d sea cells and %d where the table meets the ground\n",
                real.x, real.z, 1000 * b.recharge / area, 100 * b.risen / b.recharge, 100 * b.exported / b.recharge, 100 * b.lag / b.recharge, outlets[1], outlets[2], outlets[3], outlets[4]);
}

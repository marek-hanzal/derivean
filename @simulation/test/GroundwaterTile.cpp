#include "GroundwaterFixture.hpp"
#include "LandscapeFixture.hpp"

#include "simulation/type/World.hpp"

#include <cstdio>
#include <cstring>
#include <thread>

using namespace derivean::sim;

namespace {

// The nine finished local tiles a tile's ground water reads.
struct Nine {
    std::shared_ptr<const Tile> held[9];
    const Tile* around[9];
};

Nine tilesAround(World& world, TileKey key) {
    Nine nine;
    for (int at = 0; at < 9; ++at) world.atlas.request({Level::Local, key.x + at % 3 - 1, key.z + at / 3 - 1});
    for (int at = 0; at < 9; ++at) {
        nine.held[at] = world.atlas.require({Level::Local, key.x + at % 3 - 1, key.z + at / 3 - 1});
        nine.around[at] = nine.held[at].get();
    }
    return nine;
}

template <typename T> bool sameBits(const std::vector<T>& a, const std::vector<T>& b) {
    return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(T)) == 0);
}

// Whether two builds of a tile agree to the last bit in everything a reader can see.
bool sameTile(const GroundwaterTile& a, const GroundwaterTile& b) {
    if (!sameBits(a.table, b.table) || !sameBits(a.deepTable, b.deepTable) || !sameBits(a.recharge, b.recharge) || !sameBits(a.emergence, b.emergence) || !sameBits(a.state, b.state)) return false;
    for (int storey = 0; storey < 2; ++storey)
        if (!sameBits(a.rock[storey], b.rock[storey]) || !sameBits(a.from[storey], b.from[storey])) return false;
    if (a.springs.size() != b.springs.size() || a.caves->conduits.size() != b.caves->conduits.size()) return false;
    for (size_t i = 0; i < a.springs.size(); ++i) {
        const Spring &s = a.springs[i], &t = b.springs[i];
        if (s.position.x != t.position.x || s.position.z != t.position.z || s.elevation != t.elevation || s.discharge != t.discharge || s.temperature != t.temperature ||
            s.silica != t.silica || s.carbonate != t.carbonate || s.steamShare != t.steamShare || s.id != t.id || a.records[i].catchment != b.records[i].catchment)
            return false;
    }
    for (size_t i = 0; i < a.caves->conduits.size(); ++i) {
        const Conduit &c = a.caves->conduits[i], &d = b.caves->conduits[i];
        if (c.ax != d.ax || c.ay != d.ay || c.az != d.az || c.bx != d.bx || c.by != d.by || c.bz != d.bz || c.radius != d.radius || c.flux != d.flux || c.age != d.age || c.id != d.id) return false;
    }
    return sameBits(a.caves->cellStart, b.caves->cellStart) && sameBits(a.caves->cellItems, b.caves->cellItems) && std::memcmp(&a.budget, &b.budget, sizeof(WaterBudget)) == 0;
}

// m³/yr that the nodes of one column of a field pass to the column east of it.
double passedEast(const WaterField& field, int x) {
    double total = 0;
    for (int z = field.margin; z < field.size - field.margin; ++z)
        for (int storey = Upper; storey <= Lower; ++storey)
            for (const int k : {7, 0, 1}) {
                const size_t cell = field.index(x, z), node = cell * 2 + size_t(storey);
                if (storey == Upper && Way(field.way[cell * 8 + size_t(k)]) == Way::Outcrop) continue;
                total += double(field.sideways[node]) * field.share[node * 8 + size_t(k)];
            }
    return total;
}

} // namespace

void testGroundwaterTile() {
    World& world = testWorld();
    const TileKey key = tileAt(Level::Local, testFocus()), east{Level::Local, key.x + 1, key.z};
    const Nine here = tilesAround(world, key), there = tilesAround(world, east);
    Setting alone = world.atlas.setting();
    alone.workers = nullptr;
    const Setting pooled = world.atlas.setting();

    // One thread and the pool give the same tile, bit for bit.
    WaterField single, many, beside;
    readColumns(alone, key, here.around, single);
    const std::shared_ptr<GroundwaterTile> first = solveField(single);
    readColumns(pooled, key, here.around, many);
    const std::shared_ptr<GroundwaterTile> second = solveField(many);
    expectWater(sameTile(*first, *second), "a tile's ground water depends on the number of threads");

    const WaterBudget& budget = first->budget;
    expectWater(budget.recharge > 0 && budget.recharge < budget.runoff, "recharge is not a share of the runoff", budget.recharge / std::max(1.0, budget.runoff));
    expectWater(std::abs(imbalance(budget)) < 1e-6, "water entering the ground of a tile is not all accounted for", imbalance(budget));
    for (size_t cell = 0; cell < first->table.size(); ++cell) expectWater(first->table[cell] >= first->deepTable[cell] - 1e-3f, "perched water stands below the water under it", double(cell));

    // Two tiles say the same of the cells along their shared edge without one reading the other:
    // the last own column of the western tile and the first of the eastern, in both fields.
    readColumns(pooled, east, there.around, beside);
    solveField(beside);
    for (int column = TileCells - 1; column <= TileCells; ++column)
        for (int z = 0; z < TileCells; ++z) {
            const size_t a = many.index(many.margin + column, many.margin + z), b = beside.index(beside.margin + column - TileCells, beside.margin + z);
            for (size_t node = 0; node < 2; ++node) {
                const NodeFate &f = many.fate[a * 2 + node], &g = beside.fate[b * 2 + node];
                const bool same = many.level[a * 2 + node] == beside.level[b * 2 + node] && many.flux[a * 2 + node] == beside.flux[b * 2 + node] &&
                                  many.sideways[a * 2 + node] == beside.sideways[b * 2 + node] && f.emerges == g.emerges && f.outcrop == g.outcrop && f.lost == g.lost &&
                                  f.captured == g.captured && f.lag == g.lag && many.column[a].sunk == beside.column[b].sunk;
                expectWater(same, "two tiles disagree about the ground water of a cell on their shared edge", z);
            }
        }
    const double eastward = passedEast(many, many.margin + TileCells - 1);
    expectWater(eastward == passedEast(beside, beside.margin - 1), "water crossing a tile edge is not the same amount on both sides", eastward);

    // Through the world: whichever tile is asked for first, from however many threads, the answer is the one above.
    std::shared_ptr<const GroundwaterTile> asked[3];
    std::thread other([&] { asked[0] = groundwaterAt(world, east); }), same([&] { asked[1] = groundwaterAt(world, key); });
    asked[2] = groundwaterAt(world, key);
    other.join(), same.join();
    expectWater(asked[1] == asked[2] && sameTile(*asked[2], *second), "the world's answer for a tile depends on who asks first");
    expectWater(caveFieldAt(world, testFocus()) == asked[2]->caves, "the cave field of a point is not that of its tile");

    int kinds[4] = {};
    for (const SpringRecord& record : first->records) ++kinds[int(record.kind)];
    std::printf("ground water: local tile %d %d: recharge %.0f mm/yr of %.0f mm/yr runoff; springs %.1f %%, seepage %.1f %%, baseflow %.1f %%, deep %.1f %%, followed no farther %.1f %% of it; "
                "%d table, %d contact, %d fault, %d karst springs; %zu conduits; %.3g m3/yr cross its east edge\n",
                key.x, key.z, 1000 * budget.recharge / (tileSpan(Level::Local) * tileSpan(Level::Local)), 1000 * budget.runoff / (tileSpan(Level::Local) * tileSpan(Level::Local)),
                100 * budget.springs / budget.recharge, 100 * budget.seepage / budget.recharge, 100 * budget.baseflow / budget.recharge, 100 * budget.deep / budget.recharge,
                100 * budget.underflow / budget.recharge, kinds[0], kinds[1], kinds[2], kinds[3], first->caves->conduits.size(), eastward);
}

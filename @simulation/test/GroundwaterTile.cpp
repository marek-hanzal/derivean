// One real tile checked along every way the world serves it (about 270 lines): the checks share
// the tile, its neighbour and the fields that cost minutes to build, so they stay one owner.
#include "GroundwaterFixture.hpp"
#include "LandscapeFixture.hpp"

#include "groundwater/fn/Inflow.hpp"
#include "groundwater/fn/Refine.hpp"
#include "simulation/type/World.hpp"

#include <algorithm>
#include <climits>
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

int64_t floorDiv(int64_t a, int64_t b) { return a / b - (a % b != 0 && (a < 0) != (b < 0)); }

// The springs `springsIn` gives for a rectangle of whole local cells are those of the tiles' springs
// whose cells it covers, each once, and each vent lies in its cell. The rectangle is cut at every
// cell edge a vent's cell has, across and along, and the two parts give the springs of the whole.
// `cutsX` are further cell edges across which it is cut. Returns how many springs it has.
size_t checkRectangle(World& world, const std::vector<const GroundwaterTile*>& tiles, int64_t x0, int64_t z0, int64_t x1, int64_t z1, std::vector<int64_t> edgesX = {}) {
    const Vec2 low{double(x0) * WaterCell, double(z0) * WaterCell}, high{double(x1) * WaterCell, double(z1) * WaterCell};
    const std::vector<Spring> whole = springsIn(world, low, high);
    size_t owned = 0;
    std::vector<int64_t> edgesZ;
    for (const GroundwaterTile* tile : tiles)
        for (size_t index = 0; index < tile->springs.size(); ++index) {
            int64_t x, z;
            cellOf(*tile, index, x, z);
            if (x < x0 || x >= x1 || z < z0 || z >= z1) continue;
            ++owned;
            for (const int64_t edge : {x, x + 1})
                if (edge > x0 && edge < x1) edgesX.push_back(edge);
            for (const int64_t edge : {z, z + 1})
                if (edge > z0 && edge < z1) edgesZ.push_back(edge);
            int met = 0;
            for (const Spring& spring : whole) {
                if (spring.id != tile->springs[index].id) continue;
                ++met;
                const double cornerX = double(x) * WaterCell, cornerZ = double(z) * WaterCell;
                expectWater(spring.position.x >= cornerX && spring.position.x < cornerX + WaterCell && spring.position.z >= cornerZ && spring.position.z < cornerZ + WaterCell,
                            "a vent was put outside the cell its water left the ground in", spring.position.x - cornerX);
            }
            expectWater(met == 1, "a spring of a covered cell is not answered once", double(met));
        }
    expectWater(whole.size() == owned, "a rectangle answers springs of cells it does not cover", double(whole.size()) - double(owned));
    const auto halves = [&](const std::vector<Spring>& a, const std::vector<Spring>& b) {
        expectWater(a.size() + b.size() == whole.size(), "the springs of a rectangle are not those of its parts", double(a.size() + b.size()) - double(whole.size()));
        for (const Spring& spring : whole) {
            int met = 0;
            for (const std::vector<Spring>* part : {&a, &b})
                for (const Spring& one : *part)
                    met += one.id == spring.id && one.position.x == spring.position.x && one.position.z == spring.position.z && one.elevation == spring.elevation;
            expectWater(met == 1, "a spring does not answer every query that covers it, once", double(spring.id));
        }
    };
    std::sort(edgesX.begin(), edgesX.end()), edgesX.erase(std::unique(edgesX.begin(), edgesX.end()), edgesX.end());
    std::sort(edgesZ.begin(), edgesZ.end()), edgesZ.erase(std::unique(edgesZ.begin(), edgesZ.end()), edgesZ.end());
    for (const int64_t edge : edgesX) {
        const double cut = double(edge) * WaterCell;
        halves(springsIn(world, low, {cut, high.z}), springsIn(world, {cut, low.z}, high));
    }
    for (const int64_t edge : edgesZ) {
        const double cut = double(edge) * WaterCell;
        halves(springsIn(world, low, {high.x, cut}), springsIn(world, {low.x, cut}, high));
    }
    return owned;
}

// The springs of one site tile's ground, all of them, and that each can be explained where it is.
// A site tile of the first pass well inside the local tile waits for no other site tile.
void checkVents(World& world, const GroundwaterTile& tile) {
    constexpr int PerSite = TileCells / Refinement;   // local cells along a site tile
    // The first-pass site tile with the most springs.
    int64_t bestX = 0, bestZ = 0;
    int most = 0;
    std::vector<int> count(size_t(Refinement) * Refinement, 0);
    for (size_t index = 0; index < tile.springs.size(); ++index) {
        const int cellX = tile.records[index].cell % TileCells, cellZ = tile.records[index].cell / TileCells;
        const int64_t siteX = floorDiv(int64_t(tile.key.x) * TileCells + cellX, PerSite), siteZ = floorDiv(int64_t(tile.key.z) * TileCells + cellZ, PerSite);
        if ((siteX & 1) || (siteZ & 1) || cellX < PerSite || cellZ < PerSite || cellX >= TileCells - PerSite || cellZ >= TileCells - PerSite) continue;
        if (cellX % PerSite == 0 || cellZ % PerSite == 0 || cellX % PerSite == PerSite - 1 || cellZ % PerSite == PerSite - 1) continue;
        int& here = count[size_t(cellZ / PerSite) * Refinement + size_t(cellX / PerSite)];
        if (++here > most) most = here, bestX = siteX, bestZ = siteZ;
    }
    expectWater(most > 0, "the test tile has no spring on a first-pass site tile to check", tile.key.x);
    // One cell in from the site tile's edges, so that no vent asks for a site tile beside it.
    const int64_t x0 = bestX * PerSite + 1, z0 = bestZ * PerSite + 1, x1 = (bestX + 1) * PerSite - 1, z1 = (bestZ + 1) * PerSite - 1;
    const size_t found = checkRectangle(world, {&tile}, x0, z0, x1, z1);
    for (const Spring& spring : springsIn(world, {double(x0) * WaterCell, double(z0) * WaterCell}, {double(x1) * WaterCell, double(z1) * WaterCell})) {
        const SpringAccount account = explainSpring(world, spring.position, 1.0);
        expectWater(account.found && account.spring.id == spring.id && !account.crossed.empty() && account.record.catchment > 0, "a spring cannot be explained where it is",
                    double(spring.id));
    }
    std::printf("ground water: %zu springs on the site tile at %lld %lld, each in its cell, answered once by every cut at a cell edge and explained\n", found,
                static_cast<long long>(bestX), static_cast<long long>(bestZ));
}

// Along the edge two local tiles share: the springs of a strip over it are those of both tiles,
// each once, and no tube of either reaches past its own tile, so the field of the tile a point lies
// in holds every void there.
void checkEdge(World& world, const GroundwaterTile& west, const GroundwaterTile& east) {
    const double span = tileSpan(Level::Local);
    for (const GroundwaterTile* tile : {&west, &east}) {
        const double x0 = tile->key.x * span, z0 = tile->key.z * span;
        double nearest = 1e30;
        for (const Conduit& c : tile->caves->conduits)
            nearest = std::min({nearest, std::min(c.ax, c.bx) - c.radius - x0, x0 + span - std::max(c.ax, c.bx) - c.radius, std::min(c.az, c.bz) - c.radius - z0,
                                z0 + span - std::max(c.az, c.bz) - c.radius});
        expectWater(nearest >= 0, "a tube reaches past the edge of its tile", nearest);
        if (!tile->caves->conduits.empty()) std::printf("ground water: tubes of local tile %d %d keep %.1f m from its edges\n", tile->key.x, tile->key.z, nearest);
    }
    // A strip either side of the edge, at least `Strip` cells wide and wide enough to hold the spring
    // nearest to the edge, along 32 rows around that spring (two or three site tiles), so the check
    // does not hang on where the landscape happens to put its springs.
    constexpr int64_t Strip = 8, WidestStrip = 64;
    const int64_t edge = int64_t(east.key.x) * TileCells, top = int64_t(west.key.z) * TileCells;
    int64_t nearest = INT64_MAX, row = 0;
    for (const GroundwaterTile* tile : {&west, &east})
        for (size_t index = 0; index < tile->springs.size(); ++index) {
            int64_t x, z;
            cellOf(*tile, index, x, z);
            const int64_t away = x < edge ? edge - 1 - x : x - edge;
            if (away < nearest || (away == nearest && z < row)) nearest = away, row = z;
        }
    expectWater(nearest < WidestStrip, "the test tiles have no spring near their shared edge", double(nearest));
    const int64_t strip = std::max(Strip, nearest + 1), first = std::clamp(row - 15, top, top + TileCells - 32), last = first + 31;
    const size_t count = checkRectangle(world, {&west, &east}, edge - strip, first, edge + strip, last + 1, {edge});
    std::printf("ground water: %zu springs within %lld cells of the shared edge (nearest %lld cells from it), rows %lld to %lld, each answered once\n", count,
                static_cast<long long>(strip), static_cast<long long>(nearest), static_cast<long long>(first), static_cast<long long>(last));
}

} // namespace

void testGroundwaterTile() {
    World& world = testWorld();
    const TileKey key = tileAt(Level::Local, testFocus()), east{Level::Local, key.x + 1, key.z};
    const Nine here = tilesAround(world, key), there = tilesAround(world, east);
    Setting alone = world.atlas.setting();
    alone.workers = nullptr;
    const Setting pooled = world.atlas.setting();

    // The regional flow coming up into the tiles, as the world hands it to their builds.
    const auto hereHeld = world.groundwater.regionsUnder(world.atlas, key), thereHeld = world.groundwater.regionsUnder(world.atlas, east);
    std::vector<const RegionalTile*> hereRegions, thereRegions;
    for (const auto& region : hereHeld) hereRegions.push_back(region.get());
    for (const auto& region : thereHeld) thereRegions.push_back(region.get());

    // One thread and the pool give the same tile, bit for bit.
    WaterField single, many, beside;
    readColumns(alone, key, here.around, single);
    addRegionalInflow(single, hereRegions);
    const std::shared_ptr<GroundwaterTile> first = solveField(single);
    readColumns(pooled, key, here.around, many);
    addRegionalInflow(many, hereRegions);
    const std::shared_ptr<GroundwaterTile> second = solveField(many);
    expectWater(sameTile(*first, *second), "a tile's ground water depends on the number of threads");

    const WaterBudget& budget = first->budget;
    expectWater(budget.recharge > 0 && budget.recharge < budget.runoff, "recharge is not a share of the runoff", budget.recharge / std::max(1.0, budget.runoff));
    size_t upwelled = 0;
    for (const AquiferColumn& column : single.column) upwelled += column.regional > 0;
    expectWater(upwelled > 0, "no regional water comes up anywhere in the field of the test tile", 0);
    expectWater(std::abs(imbalance(budget)) < 1e-6, "water entering the ground of a tile is not all accounted for", imbalance(budget));
    // What the tile's own storeys carry: its recharge less what it hands the regional flow.
    const double carried = budget.recharge - budget.handed + budget.sunk + budget.regional;
    expectWater(budget.underflow < 0.5 * carried, "most of the ground water a tile carries is followed no farther", budget.underflow / carried);

    // Two tiles say the same of the cells along their shared edge without one reading the other:
    // the last own column of the western tile and the first of the eastern, in both fields.
    readColumns(pooled, east, there.around, beside);
    addRegionalInflow(beside, thereRegions);
    const std::shared_ptr<GroundwaterTile> third = solveField(beside);
    for (int column = TileCells - 1; column <= TileCells; ++column)
        for (int z = 0; z < TileCells; ++z) {
            const size_t a = many.index(many.margin + column, many.margin + z), b = beside.index(beside.margin + column - TileCells, beside.margin + z);
            for (size_t node = 0; node < 2; ++node) {
                const NodeFate &f = many.fate[a * 2 + node], &g = beside.fate[b * 2 + node];
                const bool same = many.level[a * 2 + node] == beside.level[b * 2 + node] && many.flux[a * 2 + node] == beside.flux[b * 2 + node] &&
                                  many.sideways[a * 2 + node] == beside.sideways[b * 2 + node] && f.emerges == g.emerges && f.outcrop == g.outcrop && f.lost == g.lost &&
                                  f.captured == g.captured && f.lag == g.lag && many.column[a].sunk == beside.column[b].sunk && many.column[a].regional == beside.column[b].regional &&
                                  std::memcmp(&many.borne[a * 2 + node], &beside.borne[b * 2 + node], sizeof(Borne)) == 0;
                expectWater(same, "two tiles disagree about the ground water of a cell on their shared edge", z);
            }
        }
    const double eastward = passedEast(many, many.margin + TileCells - 1);
    expectWater(eastward == passedEast(beside, beside.margin - 1), "water crossing a tile edge is not the same amount on both sides", eastward);

    // Through the world: both tiles asked for at once from three threads; the answers are the ones above.
    std::shared_ptr<const GroundwaterTile> asked[3];
    std::thread other([&] { asked[0] = groundwaterAt(world, east); }), same([&] { asked[1] = groundwaterAt(world, key); });
    asked[2] = groundwaterAt(world, key);
    other.join(), same.join();
    expectWater(asked[1] == asked[2] && sameTile(*asked[2], *second) && sameTile(*asked[0], *third), "the world's answer for a tile is not the tile worked out alone");
    expectWater(caveFieldAt(world, testFocus()) == asked[2]->caves, "the cave field of a point is not that of its tile");
    checkVents(world, *asked[2]);
    checkEdge(world, *asked[2], *asked[0]);

    // One accounting: what the tile hands the regional flow is what the region takes down under it.
    // Both take it by deepRechargeOf from the same beds, but read the ground at 64 m here and at
    // 1 km there (slope, cover, streams), so the two agree only within a factor: 2.5, since the plume's
    // last round lays every class of sediment (de9801d) the finer cover at 64 m hands 0.498 of it on Windows.
    double deepUnder = 0;
    for (const auto& region : hereHeld)
        for (int z = 0; z < TileCells / Refinement; ++z)
            for (int x = 0; x < TileCells / Refinement; ++x) {
                const int64_t rx = int64_t(key.x) * (TileCells / Refinement) + x - int64_t(region->key.x) * TileCells, rz = int64_t(key.z) * (TileCells / Refinement) + z - int64_t(region->key.z) * TileCells;
                if (rx >= 0 && rz >= 0 && rx < TileCells && rz < TileCells) deepUnder += region->recharge[size_t(rz) * TileCells + size_t(rx)];
            }
    expectWater(budget.handed > 0.4 * deepUnder && budget.handed < 2.5 * deepUnder, "a tile hands the regional flow other water than the region takes down under it", budget.handed / std::max(1.0, deepUnder));
    std::printf("ground water: local tile %d %d hands the regional flow %.3g m3/yr (%.1f %% of its recharge); the region takes %.3g m3/yr down under it and brings %.3g m3/yr up into its own cells; %zu cells of its field take some\n",
                key.x, key.z, budget.handed, 100 * budget.handed / budget.recharge, deepUnder, budget.regional, upwelled);

    int kinds[4] = {};
    for (const SpringRecord& record : first->records) ++kinds[int(record.kind)];
    std::printf("ground water: local tile %d %d: recharge %.0f mm/yr of %.0f mm/yr runoff; of what its storeys carry: springs %.1f %%, seepage %.1f %%, baseflow %.1f %%, deep %.1f %%, "
                "followed no farther %.1f %%, from the regional flow %.1f %%; %d table, %d contact, %d fault, %d karst springs; %zu conduits; %.3g m3/yr cross its east edge\n",
                key.x, key.z, 1000 * budget.recharge / (tileSpan(Level::Local) * tileSpan(Level::Local)), 1000 * budget.runoff / (tileSpan(Level::Local) * tileSpan(Level::Local)),
                100 * budget.springs / carried, 100 * budget.seepage / carried, 100 * budget.baseflow / carried, 100 * budget.deep / carried, 100 * budget.underflow / carried,
                100 * budget.regional / carried, kinds[0], kinds[1], kinds[2], kinds[3], first->caves->conduits.size(), eastward);
}

#include "groundwater/fn/Regional.hpp"

#include "core/service/Workers.hpp"
#include "groundwater/constant/Hydraulics.hpp"
#include "groundwater/fn/Beds.hpp"
#include "groundwater/fn/Recharge.hpp"
#include "landscape/fn/Forcing.hpp"
#include "landscape/type/Setting.hpp"
#include "volcanic/fn/Volcanic.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <queue>

namespace derivean::sim {
namespace {

constexpr float DeepLeast = 0.05f, DeepMost = 0.8f;   // share of the recharge in the regional flow, tightest to most open rock
constexpr float DeepReference = 2.0e5f;   // m²/yr at which half the span between them is reached
constexpr float ConfinedShare = 0.7f;     // under a tight bed most of the water stays below it
constexpr float TightThrough = 0.05f;     // m/yr across the bed under a metre of head: less than most recharge
constexpr float TightBed = 5.0f;          // m a tight bed must be to confine the rock under it
constexpr float RegionalHill = 5000.0f;   // m the regional water travels on average to its outlet: sets its least gradient
constexpr float RegionalLeast = 1e-4f, RegionalMost = 0.05f;   // regional water gradients: gentler than a local hillside allows
// Local cells along a region cell; a local tile covers whole region cells.
constexpr int PerRegion = Refinement;
static_assert(specOf(Level::Region).cell == PerRegion * specOf(Level::Local).cell && TileCells % PerRegion == 0);
constexpr float RiverFlow = DrainingFlow * float(PerRegion * PerRegion);   // m³/yr: a region cell's river cuts down to the regional water

float groundOf(const Tile& tile, size_t i) {
    return tile.now.rock[i] + tile.now.sediment[Coarse][i] + tile.now.sediment[Sand][i] + tile.now.sediment[Fines][i];
}

// The nine region tiles a field reads, by global cell; cells beyond them read the nearest one inside.
struct Around {
    TileKey key;
    const Tile* const* tiles;

    const Tile& at(int64_t x, int64_t z, size_t& index) const {
        const int64_t low = int64_t(key.x - 1) * TileCells, lowZ = int64_t(key.z - 1) * TileCells;
        x = std::clamp<int64_t>(x, low, low + 3 * TileCells - 1), z = std::clamp<int64_t>(z, lowZ, lowZ + 3 * TileCells - 1);
        const int tx = int((x - low) / TileCells), tz = int((z - lowZ) / TileCells);
        index = size_t(z - lowZ - int64_t(tz) * TileCells) * TileCells + size_t(x - low - int64_t(tx) * TileCells);
        return *tiles[tz * 3 + tx];
    }
    float ground(int64_t x, int64_t z) const {
        size_t index;
        const Tile& tile = at(x, z, index);
        return groundOf(tile, index);
    }
};

} // namespace

DeepColumn deepColumnOf(const WaterLayer* layers, int count) {
    DeepColumn column;
    for (int l = 0; l < count; ++l) column.transmissivity += layers[l].along * std::max(0.0f, layers[l].top - layers[l].bottom);
    float below = column.transmissivity, run = 0;
    for (int l = 0; l < count; ++l) {
        const float thick = std::max(0.0f, layers[l].top - layers[l].bottom);
        below -= layers[l].along * thick;
        // Beds as Storeys.cpp reads them (cover, piles and basement are not beds) that lie over at
        // least half of what conducts: the water must cross them to go deep. Under that depth the
        // regional flow runs along the rock above a tight bed and need not cross it.
        if (layers[l].unit != 0 && thick >= 1.0f && below >= 0.5f * column.transmissivity) column.passes = std::min(column.passes, layers[l].through);
        if (!column.confined) column.confined = layers[l].unit != 0 && thick >= TightBed && layers[l].through < TightThrough && below >= 0.5f * column.transmissivity;
        run = layers[l].soluble >= SolubleEnough ? run + thick : 0.0f;
        column.karst = column.karst || run >= KarstThickness;
    }
    return column;
}

float deepShareOf(float transmissivity, bool confined) {
    const float t = std::max(0.0f, transmissivity);
    const float share = DeepLeast + (DeepMost - DeepLeast) * t / (t + DeepReference);
    return confined ? std::max(share, ConfinedShare) : share;
}

float deepRechargeOf(const DeepColumn& column, float recharge, float area) {
    if (column.karst || recharge <= 0) return 0;
    return std::min(recharge * deepShareOf(column.transmissivity, column.confined), column.passes * area);
}

void readRegionalColumns(const Setting& setting, TileKey key, const Tile* const around[9], std::vector<RegionalColumn>& field) {
    const double cell = specOf(Level::Region).cell, area = cell * cell;
    const int64_t firstX = int64_t(key.x) * TileCells - RegionalMargin, firstZ = int64_t(key.z) * TileCells - RegionalMargin;
    field.assign(size_t(RegionalCells) * RegionalCells, RegionalColumn{});
    const Around tiles{key, around};
    const auto rows = [&](int begin, int end) {
        for (int z = begin; z < end; ++z)
            for (int x = 0; x < RegionalCells; ++x) {
                const int64_t gx = firstX + x, gz = firstZ + z;
                size_t from;
                const Tile& tile = tiles.at(gx, gz, from);
                RegionalColumn& column = field[size_t(z) * RegionalCells + size_t(x)];
                column.ground = groundOf(tile, from);
                const bool sea = column.ground < float(tile.seaLevel), lake = tile.water[from] > column.ground + 1e-3f;
                column.outlet = sea ? 3 : lake ? 2 : tile.discharge[from] >= RiverFlow ? 1 : 0;
                column.stage = sea ? float(tile.seaLevel) : lake ? tile.water[from] : column.ground;
                if (sea || lake) continue;

                // The rock under the cell, as a local column reads it, down to the base of circulation.
                BedColumn bed;
                bed.seed = setting.seed;
                bed.strata = strataAt(*setting.planet, planetPoint(setting.seed, {(double(gx) + 0.5) * cell, (double(gz) + 0.5) * cell}));
                bed.node = &bed.strata;
                bed.material = bed.strata.material;
                bed.fold = foldAt(setting.seed, bed.strata, bed.material);
                bed.rockTop = tile.now.rock[from], bed.datum = tile.now.datum[from];
                for (int c = 0; c < SedimentClasses; ++c) bed.cover[c] = tile.now.sediment[c][from];
                const PileColumn pile = pileColumn(tile, from);
                bed.pile = pile.thickness, bed.lava = float(pile.lava) / 255.0f;
                BedWalk walk;
                walkBeds(bed, walk);
                const DeepColumn deep = deepColumnOf(walk.layers, walk.count);
                column.transmissivity = deep.transmissivity, column.confined = deep.confined, column.depth = std::max(0.0f, column.ground - walk.base);
                const float slope = float(std::hypot(tiles.ground(gx + 1, gz) - tiles.ground(gx - 1, gz), tiles.ground(gx, gz + 1) - tiles.ground(gx, gz - 1)) / (2 * cell));
                const Soak soak{tile.rain[from], tile.warmth[from], slope, {bed.cover[0], bed.cover[1], bed.cover[2]}, tile.ice[from], Material(tile.rock[from])};
                // As a local column takes it (fn/Columns.cpp): the cell's own soak and what a dry
                // country's loose bed took from passing floods beyond that share.
                const float share = infiltrationShare(soak);
                const double seeped = tile.seeped.empty() ? 0.0 : double(tile.seeped[from]) * (1.0f - share);
                column.recharge = deepRechargeOf(deep, float(double(runoffOf(tile.rain[from], tile.warmth[from])) * share * area + seeped), area);
            }
    };
    if (setting.workers) setting.workers->parallelFor(RegionalCells, rows, 4);
    else rows(0, RegionalCells);
}

std::shared_ptr<RegionalTile> solveRegional(TileKey key, const std::vector<RegionalColumn>& field) {
    const int size = RegionalCells;
    const size_t cells = field.size();
    const float cell = float(specOf(Level::Region).cell), area = cell * cell;
    // An outlet stands at its stage and any other cell at most at its ground; a cell left at that
    // height by the search below is where its water comes up. Under a tight bed water stands as
    // high as its way out needs.
    constexpr float Unset = std::numeric_limits<float>::infinity();
    std::vector<float> head(cells, Unset);
    std::vector<int32_t> parent(cells, -1);
    using Item = std::pair<float, int32_t>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
    for (size_t i = 0; i < cells; ++i) {
        if (field[i].confined && !field[i].outlet) continue;
        head[i] = field[i].outlet ? field[i].stage : field[i].ground;
        open.push({head[i], int32_t(i)});
    }
    std::vector<uint8_t> done(cells, 0);
    while (!open.empty()) {
        const auto [level, at] = open.top();
        open.pop();
        if (done[size_t(at)] || level > head[size_t(at)]) continue;
        done[size_t(at)] = 1;
        const int x = at % size, z = at / size;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t other = size_t(nz) * size_t(size) + size_t(nx);
            const RegionalColumn& column = field[other];
            if (done[other] || column.outlet) continue;
            const float rate = column.recharge / area;
            const float gradient = column.transmissivity > 0 ? std::clamp(rate * RegionalHill / column.transmissivity, RegionalLeast, RegionalMost) : RegionalMost;
            const float reached = level + gradient * NeighborReach[k] * cell;
            if (reached >= head[other]) continue;
            head[other] = reached, parent[other] = at;
            open.push({reached, int32_t(other)});
        }
    }

    // Each cell's water follows its way down, highest heads first; every step of a way goes lower.
    // What it carries is summed by flow: where it went down, how deep and how far it went.
    std::vector<int32_t> order(cells);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int32_t a, int32_t b) { return head[size_t(a)] > head[size_t(b)] || (head[size_t(a)] == head[size_t(b)] && a < b); });
    std::vector<double> all(cells), own(cells), rate(cells), high(cells), deep(cells), far(cells, 0.0);
    const auto inside = [&](size_t i) {
        const int x = int(i % size_t(size)), z = int(i / size_t(size));
        return x >= RegionalMargin && z >= RegionalMargin && x < RegionalMargin + TileCells && z < RegionalMargin + TileCells;
    };
    for (size_t i = 0; i < cells; ++i) {
        all[i] = field[i].recharge, own[i] = inside(i) ? field[i].recharge : 0.0;
        rate[i] = all[i] * all[i] / area, high[i] = all[i] * field[i].ground, deep[i] = all[i] * field[i].depth;
    }

    auto tile = std::make_shared<RegionalTile>();
    tile->key = key;
    const size_t ownCells = size_t(TileCells) * TileCells;
    for (std::vector<float>* values : {&tile->recharge, &tile->upwelling, &tile->head, &tile->rate, &tile->high, &tile->far, &tile->deep}) values->assign(ownCells, 0.0f);
    tile->outlet.assign(ownCells, 0);
    RegionalBudget& budget = tile->budget;
    for (const int32_t at : order) {
        const size_t i = size_t(at);
        const int x = at % size, z = at / size;
        if (parent[i] >= 0) {
            const size_t to = size_t(parent[i]);
            const double step = std::hypot(double(int(to % size_t(size)) - x), double(int(to / size_t(size)) - z)) * cell;
            all[to] += all[i], own[to] += own[i], rate[to] += rate[i], high[to] += high[i], deep[to] += deep[i], far[to] += far[i] + all[i] * step;
            continue;
        }
        // It comes up here. On the field's rim the way may go on beyond what the field holds, and
        // a cell no way reached has none in the field.
        const bool rim = x == 0 || z == 0 || x == size - 1 || z == size - 1;
        if (head[i] == Unset || (rim && !field[i].outlet)) {
            budget.lag += own[i];
            continue;
        }
        if (!inside(i)) {
            budget.exported += own[i];
            continue;
        }
        budget.risen += own[i];
        if (all[i] <= 0) continue;
        const size_t mine = size_t(z - RegionalMargin) * TileCells + size_t(x - RegionalMargin);
        tile->upwelling[mine] = float(all[i]);
        tile->rate[mine] = float(rate[i] / all[i]), tile->high[mine] = float(high[i] / all[i]), tile->far[mine] = float(far[i] / all[i]), tile->deep[mine] = float(deep[i] / all[i]);
        budget.arriving += all[i];
    }
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = size_t(z + RegionalMargin) * size_t(size) + size_t(x + RegionalMargin), mine = size_t(z) * TileCells + size_t(x);
            tile->recharge[mine] = field[i].recharge, tile->head[mine] = head[i];
            tile->outlet[mine] = field[i].outlet ? field[i].outlet : tile->upwelling[mine] > 0 ? 4 : 0;
            budget.recharge += field[i].recharge;
        }
    return tile;
}

std::shared_ptr<RegionalTile> buildRegional(const Setting& setting, TileKey key, const Tile* const around[9]) {
    std::vector<RegionalColumn> field;
    readRegionalColumns(setting, key, around, field);
    return solveRegional(key, field);
}

} // namespace derivean::sim

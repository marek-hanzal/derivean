#include "SeamFixture.hpp"

#include "landscape/fn/Build.hpp"
#include "landscape/fn/Forcing.hpp"
#include "material/constant/Materials.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <map>
#include <numeric>

using namespace derivean::sim;

namespace {

// The made parent is the middle course of a large river: 4 m/km down a V-shaped valley 150 m
// deep and 24 km across, so every Local cell of the row lies on its flanks and drains to it.
constexpr double Above = 1500;      // m over the sea where the river enters the made parent
constexpr double Fall = 0.004;      // m/m down the river
constexpr double Depth = 150;       // m from the river to the valley's rims
constexpr double HalfWidth = 12;    // parent cells from the river to a rim
constexpr double Entering = 3e8;    // m³/yr the river brings into the made parent
constexpr float Runoff = 0.4f;      // m/yr every parent cell adds
constexpr float Rain = 0.9f;        // m/yr
constexpr float Uplift = 1e-4f;     // m/yr of rock uplift
constexpr float LoadShare = 2e-4f;  // m³ of sediment per m³ of water, about 500 mg/l
constexpr int Margin = 24;          // parent cells the routed window reaches beyond the row's tiles

struct Valley {
    int64_t alongStart, axis;   // parent cells: upstream end of the window, the river's line across
    bool alongZ;
    double top;                 // m, surface where the river enters

    double height(int64_t gx, int64_t gz) const {
        const double along = double((alongZ ? gz : gx) - alongStart), across = double((alongZ ? gx : gz) - axis);
        return top - Fall * specOf(Level::Region).cell * along + Depth * std::min(1.0, std::abs(across) / HalfWidth);
    }
    uint8_t steepest(int64_t gx, int64_t gz) const {
        uint8_t best = 255;
        double fall = 0;
        for (int k = 0; k < 8; ++k) {
            const double drop = (height(gx, gz) - height(gx + NeighborX[k], gz + NeighborZ[k])) / NeighborReach[k];
            if (drop > fall) fall = drop, best = uint8_t(k);
        }
        return best;
    }
};

// Discharge over the window, routed down the steepest way from the highest cell: the river and
// what its flanks add. Cells outside the window carry their own runoff.
struct Routed {
    int64_t x0, z0, nx, nz;
    std::vector<double> discharge;

    Routed(const Valley& valley, int64_t lowX, int64_t lowZ, int64_t highX, int64_t highZ) : x0(lowX), z0(lowZ), nx(highX - lowX), nz(highZ - lowZ) {
        const double own = Runoff * specOf(Level::Region).cell * specOf(Level::Region).cell;
        discharge.assign(size_t(nx * nz), own);
        std::vector<size_t> order(discharge.size());
        std::iota(order.begin(), order.end(), size_t(0));
        const auto heightOf = [&](size_t i) { return valley.height(x0 + int64_t(i) % nx, z0 + int64_t(i) / nx); };
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return heightOf(a) > heightOf(b) || (heightOf(a) == heightOf(b) && a < b); });
        const int64_t entryX = valley.alongZ ? valley.axis : valley.alongStart, entryZ = valley.alongZ ? valley.alongStart : valley.axis;
        discharge[size_t((entryZ - z0) * nx + (entryX - x0))] += Entering;
        for (const size_t i : order) {
            const int64_t gx = x0 + int64_t(i) % nx, gz = z0 + int64_t(i) / nx;
            const uint8_t to = valley.steepest(gx, gz);
            if (to > 7 || !inside(gx + NeighborX[to], gz + NeighborZ[to])) continue;
            discharge[size_t((gz + NeighborZ[to] - z0) * nx + (gx + NeighborX[to] - x0))] += discharge[i];
        }
    }
    bool inside(int64_t gx, int64_t gz) const { return gx >= x0 && gz >= z0 && gx < x0 + nx && gz < z0 + nz; }
    double at(int64_t gx, int64_t gz, double own) const { return inside(gx, gz) ? discharge[size_t((gz - z0) * nx + (gx - x0))] : own; }
};

std::shared_ptr<Tile> madeParent(const Setting& setting, TileKey key, const Valley& valley, const Routed& routed) {
    auto tile = std::make_shared<Tile>();
    tile->key = key;
    tile->time = setting.age;
    tile->seaLevel = seaLevelAt(setting, setting.age);
    const size_t cells = size_t(TileCells) * TileCells;
    tile->now.resize(cells);
    for (auto* field : {&tile->water, &tile->discharge, &tile->load, &tile->uplift, &tile->rebound, &tile->ice, &tile->iceAge, &tile->iceSpeed, &tile->erosionRate, &tile->depositionRate,
                        &tile->eroded, &tile->deposited, &tile->uplifted, &tile->rain, &tile->warmth, &tile->shed})
        field->assign(cells, 0.0f);
    tile->flowTo.assign(cells, 255);
    tile->rock.assign(cells, uint8_t(Material::Sandstone));
    const double own = Runoff * specOf(Level::Region).cell * specOf(Level::Region).cell;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            const size_t i = size_t(z) * TileCells + size_t(x);
            const int64_t gx = int64_t(key.x) * TileCells + x, gz = int64_t(key.z) * TileCells + z;
            const float top = float(valley.height(gx, gz));
            tile->now.sediment[Sand][i] = 0.5f;
            tile->now.sediment[Fines][i] = 0.4f;
            tile->now.rock[i] = tile->now.datum[i] = top - 0.9f;
            tile->water[i] = top;
            tile->flowTo[i] = valley.steepest(gx, gz);
            tile->discharge[i] = float(routed.at(gx, gz, own));
            tile->load[i] = LoadShare * tile->discharge[i];
            tile->uplift[i] = Uplift;
            tile->rain[i] = Rain;
        }
    return tile;
}

} // namespace

SeamRow buildSeamRow(const Setting& setting, TileKey first, bool alongZ) {
    SeamRow row;
    row.first = first, row.alongZ = alongZ, row.entering = Entering;
    TileKey keys[3];
    for (int k = 0; k < 3; ++k) keys[k] = {Level::Local, first.x + (alongZ ? 0 : k), first.z + (alongZ ? k : 0)};
    const int64_t per = TileCells / Refinement;   // parent cells per Local tile
    const int64_t alongTile = alongZ ? first.z : first.x, acrossTile = alongZ ? first.x : first.z;
    const Valley valley{alongTile * per - Margin, acrossTile * per + per / 2, alongZ, seaLevelAt(setting, setting.age) + Above};
    const int64_t lowAlong = alongTile * per - Margin, highAlong = (alongTile + 3) * per + Margin;
    const int64_t lowAcross = acrossTile * per - Margin, highAcross = (acrossTile + 1) * per + Margin;
    const Routed routed(valley, alongZ ? lowAcross : lowAlong, alongZ ? lowAlong : lowAcross, alongZ ? highAcross : highAlong, alongZ ? highAlong : highAcross);

    std::map<std::pair<int, int>, std::shared_ptr<Tile>> parents;
    std::map<std::pair<int, int>, std::shared_ptr<const Tile>> built;
    for (const TileKey key : keys)
        for (const TileKey need : dependencies(key))
            if (need.level == Level::Region && !parents.count({need.x, need.z})) parents[{need.x, need.z}] = madeParent(setting, need, valley, routed);
    const auto lookup = [&](TileKey key) -> const Tile* {
        if (key.level == Level::Region) {
            const auto found = parents.find({key.x, key.z});
            return found == parents.end() ? nullptr : found->second.get();
        }
        const auto found = built.find({key.x, key.z});
        return key.level == Level::Local && found != built.end() ? found->second.get() : nullptr;
    };
    // Tiles of one pass are built side by side, each pass after the ones before it, as the atlas does.
    for (int pass = 0; pass < 4; ++pass) {
        std::vector<std::pair<int, std::future<std::shared_ptr<Tile>>>> running;
        for (int k = 0; k < 3; ++k)
            if (passOf(keys[k]) == pass) running.push_back({k, std::async(std::launch::async, [&, k] { return replayTile(setting, keys[k], lookup); })});
        for (auto& [k, result] : running) row.tiles[k] = result.get();
        for (auto& [k, result] : running) built[{keys[k].x, keys[k].z}] = row.tiles[k];
    }
    return row;
}

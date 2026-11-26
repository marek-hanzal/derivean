#include "Groundwater.hpp"

#include "Measure.hpp"
#include "groundwater/fn/Caves.hpp"
#include "groundwater/fn/Stages.hpp"
#include "simulation/type/World.hpp"

#include <cstdio>

using namespace derivean::sim;

void timeGroundwater(World& world, TileKey local) {
    std::shared_ptr<const Tile> held[9];
    const Tile* around[9];
    for (int at = 0; at < 9; ++at) {
        held[at] = world.atlas.require({Level::Local, local.x + at % 3 - 1, local.z + at / 3 - 1});
        around[at] = held[at].get();
    }
    for (int pooled = 0; pooled < 2; ++pooled) {
        Setting setting = world.atlas.setting();
        if (!pooled) setting.workers = nullptr;
        double best[7] = {1e9, 1e9, 1e9, 1e9, 1e9, 1e9, 1e9};
        std::shared_ptr<GroundwaterTile> tile;
        WaterField field;
        for (int run = 0; run < 3; ++run) {
            Lap lap;
            field = WaterField{};
            readColumns(setting, local, around, field);
            const double columns = lap();
            const double sinks = 0;   // what sank is the landscape's record (groundwater/fn/Sinking.hpp)
            settleLevels(field);
            const double levels = lap();
            routeGroundWater(field);
            const double route = lap();
            tile = std::make_shared<GroundwaterTile>();
            findVents(field, *tile);
            const double vents = lap();
            growConduits(field, *tile);
            const double caves = lap();
            const double all[7] = {columns, sinks, levels, route, vents, caves, columns + sinks + levels + route + vents + caves};
            for (int stage = 0; stage < 7; ++stage) best[stage] = std::min(best[stage], all[stage]);
        }
        std::printf("local tile %d %d, best of 3, %s: columns %.0f ms, sinking streams %.0f, levels %.0f, routing %.0f, vents and fields %.0f, conduits %.0f (%zu listed) | water table and springs %.0f ms, cave field %.0f ms, all %.0f ms\n",
                    local.x, local.z, pooled ? "with the pool" : "one thread", 1e3 * best[0], 1e3 * best[1], 1e3 * best[2], 1e3 * best[3], 1e3 * best[4], 1e3 * best[5],
                    tile->caves->conduits.size(), 1e3 * (best[6] - best[5]), 1e3 * best[5], 1e3 * best[6]);
        if (!pooled) {
            // Where the water that is followed no farther was when the sweeps ended.
            double lag[4] = {}, entered = 0;
            for (int z = field.margin; z < field.size - field.margin; ++z)
                for (int x = field.margin; x < field.size - field.margin; ++x) {
                    const size_t cell = field.index(x, z);
                    const AquiferColumn& column = field.column[cell];
                    entered += shallowRecharge(column) + column.sunk + column.regional;
                    lag[0] += field.fate[cell * 2 + Upper].lag;
                    lag[column.crush >= 0.2f ? 1 : column.perched ? 2 : 3] += field.fate[cell * 2 + Lower].lag;
                }
            std::printf("  still under way after %d sweeps, of what enters: on tight beds %.1f %%, in fault rock %.1f %%, under tight beds %.1f %%, elsewhere %.1f %%\n", RouteSweeps,
                        100 * lag[0] / entered, 100 * lag[1] / entered, 100 * lag[2] / entered, 100 * lag[3] / entered);
        }
        if (!pooled || tile->caves->conduits.empty()) continue;
        // What a voxel column pays: the spans of vertical lines through the middle of every conduit.
        Lap lap;
        int found = 0, most = 0;
        constexpr int Rounds = 20;
        for (int round = 0; round < Rounds; ++round)
            for (const Conduit& conduit : tile->caves->conduits) {
                CaveSpan spans[MostCaveSpans];
                const int count = caveSpans(*tile->caves, {0.5 * (conduit.ax + conduit.bx) + round * 0.25, 0.5 * (conduit.az + conduit.bz)}, spans, MostCaveSpans);
                found += count, most = std::max(most, count);
            }
        const size_t queries = size_t(Rounds) * tile->caves->conduits.size();
        std::printf("caveSpans: %.3f microseconds a column through a passage (%zu queries, %d spans, at most %d on one line)\n", 1e6 * lap() / double(queries), queries, found, most);
    }
}

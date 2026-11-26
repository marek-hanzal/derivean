#include "icesheet/fn/Parent.hpp"

#include "icesheet/constant/Ice.hpp"

namespace derivean::sim {
namespace {

// The parent tiles a domain depends on, addressed by the parent's global cell index. The same
// square of tiles the landscape seeds the domain from (landscape/fn/Seed.cpp), so no tile is asked
// for that the builder has not been promised.
struct Parents {
    int baseX{}, baseZ{}, countX{}, countZ{};
    double cell{};
    std::vector<const Tile*> tiles;

    Parents(const Domain& domain, const std::function<const Tile*(TileKey)>& lookup) {
        const TileKey child = domain.key;
        const int margin = parentMargin(child.level), span = TileCells / Refinement;
        cell = domain.cell * Refinement;
        baseX = int(std::floor((child.x * span - margin) / double(TileCells)));
        baseZ = int(std::floor((child.z * span - margin) / double(TileCells)));
        countX = int(std::floor(((child.x + 1) * span + margin - 1) / double(TileCells))) - baseX + 1;
        countZ = int(std::floor(((child.z + 1) * span + margin - 1) / double(TileCells))) - baseZ + 1;
        const Level level = Level(int(child.level) - 1);
        for (int z = 0; z < countZ; ++z)
            for (int x = 0; x < countX; ++x) tiles.push_back(lookup({level, baseX + x, baseZ + z}));
    }

    // The tile holding global parent cell (gx, gz), clamped to the square, and the cell inside it.
    const Tile* at(int64_t gx, int64_t gz, size_t& index) const {
        const int tx = std::clamp(int(std::floor(gx / double(TileCells))) - baseX, 0, countX - 1);
        const int tz = std::clamp(int(std::floor(gz / double(TileCells))) - baseZ, 0, countZ - 1);
        const int lx = std::clamp(int(gx - int64_t(tx + baseX) * TileCells), 0, TileCells - 1);
        const int lz = std::clamp(int(gz - int64_t(tz + baseZ) * TileCells), 0, TileCells - 1);
        index = size_t(lz) * TileCells + size_t(lx);
        return tiles[size_t(tz) * size_t(countX) + size_t(tx)];
    }

    bool anyRecord() const {
        for (const Tile* tile : tiles)
            if (tile && tile->sheet) return true;
        return false;
    }

    Vec2 centre(int64_t gx, int64_t gz) const { return {(double(gx) + 0.5) * cell, (double(gz) + 0.5) * cell}; }
};

float groundOf(const Tile& tile, size_t i) { return tile.now.rock[i] + tile.now.sediment[0][i] + tile.now.sediment[1][i] + tile.now.sediment[2][i]; }

} // namespace

ParentSheet readParentSheet(const Domain& domain, const std::function<const Tile*(TileKey)>& tiles) {
    const Parents parents(domain, tiles);
    ParentSheet sheet;
    sheet.any = parents.anyRecord();
    if (!sheet.any) return sheet;
    const size_t cells = domain.cells();
    for (auto* field : {&sheet.iceNow, &sheet.iceThen, &sheet.topNow, &sheet.topThen, &sheet.slideX, &sheet.slideZ, &sheet.worn, &sheet.esker}) field->assign(cells, 0.0f);
    constexpr int Fields = 8;
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const Vec2 here = domain.center(x, z);
            const double fx = here.x / parents.cell - 0.5, fz = here.z / parents.cell - 0.5;
            const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
            const float tx = float(fx - double(ix)), tz = float(fz - double(iz));
            float sum[Fields] = {};
            for (int k = 0; k < 4; ++k) {
                size_t c = 0;
                const Tile* tile = parents.at(ix + (k & 1), iz + (k >> 1), c);
                if (!tile) continue;
                const float weight = ((k & 1) ? tx : 1 - tx) * ((k >> 1) ? tz : 1 - tz);
                const float ground = groundOf(*tile, c);
                const SheetRecord* record = tile->sheet.get();
                // A finer parent's own glaciers are in its iceAge; its record holds the sheet's alone.
                const float then = record && !record->sheetThen.empty() ? record->sheetThen[c] : tile->key.level == Level::Region ? tile->iceAge[c] : 0.0f;
                const float values[Fields] = {tile->ice[c], then, ground + tile->ice[c], ground + then, record ? record->slideX[c] : 0.0f,
                                              record ? record->slideZ[c] : 0.0f, record ? record->worn[c] : 0.0f, record ? record->esker[c] : 0.0f};
                for (int f = 0; f < Fields; ++f) sum[f] += weight * values[f];
            }
            const size_t i = domain.index(x, z);
            sheet.iceNow[i] = sum[0], sheet.iceThen[i] = sum[1], sheet.topNow[i] = sum[2], sheet.topThen[i] = sum[3];
            sheet.slideX[i] = sum[4], sheet.slideZ[i] = sum[5], sheet.worn[i] = sum[6], sheet.esker[i] = sum[7];
        }
    }
    return sheet;
}

std::vector<Tunnel> parentTunnels(const Domain& domain, const std::function<const Tile*(TileKey)>& tiles) {
    const Parents parents(domain, tiles);
    std::vector<Tunnel> found;
    if (!parents.anyRecord()) return found;
    const double extent = domain.size * domain.cell;
    const int64_t lowX = int64_t(std::floor(domain.origin.x / parents.cell)), lowZ = int64_t(std::floor(domain.origin.z / parents.cell));
    const int64_t highX = int64_t(std::floor((domain.origin.x + extent) / parents.cell)), highZ = int64_t(std::floor((domain.origin.z + extent) / parents.cell));
    for (int64_t gz = lowZ; gz <= highZ; ++gz) {
        for (int64_t gx = lowX; gx <= highX; ++gx) {
            size_t c = 0;
            const Tile* tile = parents.at(gx, gz, c);
            if (!tile || !tile->sheet || tile->sheet->esker[c] < 0.05f || tile->sheet->meltTo[c] > 7) continue;
            const SheetRecord& record = *tile->sheet;
            const int to = record.meltTo[c];
            Tunnel tunnel;
            tunnel.from = parents.centre(gx, gz);
            tunnel.to = parents.centre(gx + NeighborX[to], gz + NeighborZ[to]);
            tunnel.fill = double(record.esker[c]) * parents.cell * parents.cell;
            tunnel.supply = std::min(double(record.esker[c]), double(tile->now.sediment[Coarse][c] + tile->now.sediment[Sand][c])) * parents.cell * parents.cell;
            tunnel.discharge = record.meltwater[c];
            found.push_back(tunnel);
        }
    }
    return found;
}

const SheetRecord* finishedSheet(const Domain& domain, int x, int z, size_t& at) {
    const int tx = x < Halo ? -1 : x >= Halo + TileCells ? 1 : 0, tz = z < Halo ? -1 : z >= Halo + TileCells ? 1 : 0;
    const Tile* tile = tx || tz ? domain.finished[(tz + 1) * 3 + tx + 1] : nullptr;
    if (!tile || !tile->sheet) return nullptr;
    at = size_t(z - Halo - tz * TileCells) * TileCells + size_t(x - Halo - tx * TileCells);
    return tile->sheet.get();
}

} // namespace derivean::sim

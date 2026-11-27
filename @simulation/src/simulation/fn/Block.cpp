#include "simulation/fn/Query.hpp"

#include "coast/type/SeaVoid.hpp"
#include "ground/fn/Stamp.hpp"
#include "landscape/fn/Forcing.hpp"
#include "relief/type/Form.hpp"
#include "simulation/fn/Live.hpp"
#include "simulation/fn/Reader.hpp"

#include <span>

namespace derivean::sim {

void sampleBlocks(World& world, const Block& block, const BlockSpan* spans, size_t spanCount) {
    const double maxX = block.originX + block.countX * block.step, maxZ = block.originZ + block.countZ * block.step;
    requireOver(world, Level::Site, block.originX, block.originZ, maxX, maxZ, BlockSiteReach);
    requireOver(world, Level::Ground, block.originX, block.originZ, maxX, maxZ, BlockGroundReach);

    const Setting& setting = world.atlas.setting();
    TileReader site{world.atlas, Level::Site}, fine{world.atlas, Level::Ground};
    GroundLattice ground = readGround(fine, block.originX - FormMargin, block.originZ - FormMargin, maxX + FormMargin, maxZ + FormMargin);
    // What the session dug and built is known by site cell: every node of the ground moves by the
    // change of the cell it lies in, and the stones on it move with it.
    const bool edited = !world.interventions.overlay.tiles.empty();
    if (edited && changeLattice(world, ground)) smoothLie(ground);
    const std::function<float(double, double)> lift = [&](double x, double z) { return changeAt(world, x, z).ground(); };

    // Layering and exposure live on world-anchored lattices, so a point reads the same values
    // whichever block it is asked through. Layering nodes sit at the centres of their lattice cells.
    const auto nodeBelow = [](double position) { return int64_t(std::floor(position / StrataLattice - 0.5)); };
    const int64_t strataX = nodeBelow(block.originX), strataZ = nodeBelow(block.originZ);
    const int strataCountX = int(nodeBelow(maxX) - strataX) + 2, strataCountZ = int(nodeBelow(maxZ) - strataZ) + 2;
    std::vector<FormSetting> strata(size_t(strataCountX) * size_t(strataCountZ));
    std::vector<std::vector<BedMass>> masses(strata.size());
    for (int z = 0; z < strataCountZ; ++z)
        for (int x = 0; x < strataCountX; ++x) {
            const Vec2 centre{(double(strataX + x) + 0.5) * StrataLattice, (double(strataZ + z) + 0.5) * StrataLattice};
            FormSetting& node = strata[size_t(z) * size_t(strataCountX) + size_t(x)];
            node = {setting.seed, strataAt(world.planet, planetPoint(setting.seed, centre)), centre, {}, nullptr, &masses[size_t(z) * size_t(strataCountX) + size_t(x)]};
            node.ore = oreFieldAt(setting.seed, node.strata, node.strata.material);
        }
    const int64_t openX = int64_t(std::floor(block.originX / ExposureLattice)), openZ = int64_t(std::floor(block.originZ / ExposureLattice));
    const int openCountX = int(int64_t(std::floor(maxX / ExposureLattice)) - openX) + 2, openCountZ = int(int64_t(std::floor(maxZ / ExposureLattice)) - openZ) + 2;
    std::vector<float> exposure(size_t(openCountX) * size_t(openCountZ));
    for (int z = 0; z < openCountZ; ++z)
        for (int x = 0; x < openCountX; ++x)
            exposure[size_t(z) * size_t(openCountX) + size_t(x)] = exposureAt(site, {double(openX + x) * ExposureLattice, double(openZ + z) * ExposureLattice});

    // What the sea opened at the waterline, from the site tiles already required above.
    static_assert(BlockSiteReach >= SeaVoidReach + specOf(Level::Site).cell, "sea voids need site tiles beyond the block's reach");
    const SeaVoids sea = seaVoidsOver([&](int32_t tx, int32_t tz) { return site.tile(tx, tz); }, setting, block.originX, block.originZ, maxX, maxZ);
    // A void is a property of a point; each column asks the field of the local tile it stands in.
    TileKey caveKey = tileAt(Level::Local, {block.originX, block.originZ});
    std::shared_ptr<const CaveField> caves = caveFieldAt(world, {block.originX, block.originZ});
    // m/yr the ground sheds at a point (runoffOf the site cells' rain and warmth), interpolated
    // between the cell centres so grikes do not step at the cells' edges.
    const double siteCell = specOf(Level::Site).cell;
    const auto cellRunoff = [&](int64_t gx, int64_t gz, float& runoff) {
        const int32_t tx = int32_t(std::floor(double(gx) / TileCells)), tz = int32_t(std::floor(double(gz) / TileCells));
        const Tile* tile = site.tile(tx, tz);
        if (!tile || tile->rain.empty()) return false;
        const size_t at = size_t(gz - int64_t(tz) * TileCells) * TileCells + size_t(gx - int64_t(tx) * TileCells);
        runoff = runoffOf(tile->rain[at], tile->warmth[at]);
        return true;
    };
    // The block spans a few site cells: their runoff is read once, over the cells the columns'
    // corners fall in, from the first column's to the last's. Columns step forward (step > 0, as
    // every caller passes); a block of no columns reads none.
    const auto siteCellOf = [&](double position) { return int64_t(std::floor(position / siteCell - 0.5)); };
    const int64_t runoffX = siteCellOf(block.originX + 0.5 * block.step), runoffZ = siteCellOf(block.originZ + 0.5 * block.step);
    const int runoffCountX = block.countX > 0 ? int(siteCellOf(block.originX + (block.countX - 0.5) * block.step) - runoffX) + 2 : 0;
    const int runoffCountZ = block.countZ > 0 ? int(siteCellOf(block.originZ + (block.countZ - 0.5) * block.step) - runoffZ) + 2 : 0;
    std::vector<float> runoffs(size_t(runoffCountX) * size_t(runoffCountZ));
    std::vector<uint8_t> runoffKnown(runoffs.size());
    for (int z = 0; z < runoffCountZ; ++z)
        for (int x = 0; x < runoffCountX; ++x) {
            const size_t at = size_t(z) * size_t(runoffCountX) + size_t(x);
            runoffKnown[at] = cellRunoff(runoffX + x, runoffZ + z, runoffs[at]);
        }
    const auto siteRunoff = [&](double x, double z) {
        const double fx = x / siteCell - 0.5, fz = z / siteCell - 0.5;
        const int64_t gx = int64_t(std::floor(fx)), gz = int64_t(std::floor(fz));
        const float tx = float(fx - double(gx)), tz = float(fz - double(gz));
        float sum = 0, weights = 0;
        for (int corner = 0; corner < 4; ++corner) {
            const int dx = corner & 1, dz = corner >> 1;
            const float weight = (dx ? tx : 1 - tx) * (dz ? tz : 1 - tz);
            const size_t at = size_t(gz + dz - runoffZ) * size_t(runoffCountX) + size_t(gx + dx - runoffX);
            if (weight > 0 && runoffKnown[at]) sum += weight * runoffs[at], weights += weight;
        }
        return weights > 0 ? sum / weights : 0.0f;
    };
    TileKey groundKey{Level::Ground, 0, 0};
    const Tile* groundTile = nullptr;
    bool groundRead = false;
    const size_t layer = size_t(block.countX) * size_t(block.countZ);
    for (int iz = 0; iz < block.countZ; ++iz) {
        for (int ix = 0; ix < block.countX; ++ix) {
            const double x = block.originX + (ix + 0.5) * block.step, z = block.originZ + (iz + 0.5) * block.step;
            const double fx = x / StrataLattice - 0.5 - double(strataX), fz = z / StrataLattice - 0.5 - double(strataZ);
            const int sx = std::clamp(int(fx), 0, strataCountX - 2), sz = std::clamp(int(fz), 0, strataCountZ - 2);
            const float ux = float(fx - sx), uz = float(fz - sz);
            const FormSetting* node = &strata[size_t(sz) * size_t(strataCountX) + size_t(sx)];
            const StrataColumn* corner[4] = {&node[0].strata, &node[1].strata, &node[strataCountX].strata, &node[strataCountX + 1].strata};
            const float weight[4] = {(1 - ux) * (1 - uz), ux * (1 - uz), (1 - ux) * uz, ux * uz};
            const FormSetting& nearest = node[(uz > 0.5f ? strataCountX : 0) + (ux > 0.5f ? 1 : 0)];
            FormSetting here{setting.seed, blendThickness(nearest.strata, corner, weight), nearest.strataCentre, nearest.ore, &nearest.strata, nearest.masses};
            // A block lies on one ground tile or a few: the one under the last column is kept.
            if (const TileKey under = tileAt(Level::Ground, {x, z}); !groundRead || !(under == groundKey))
                groundKey = under, groundTile = fine.tile(under.x, under.z), groundRead = true;
            if (groundTile) here.volcanic = groundTile->volcanic.get();
            here.runoff = siteRunoff(x, z);   // bare soluble rock opens by it into grikes (groundwater/fn/Pavement.hpp)
            const double ex = x / ExposureLattice - double(openX), ez = z / ExposureLattice - double(openZ);
            const int ox = std::clamp(int(ex), 0, openCountX - 2), oz = std::clamp(int(ez), 0, openCountZ - 2);
            const float tx = float(ex - ox), tz = float(ez - oz);
            const float* gap = &exposure[size_t(oz) * size_t(openCountX) + size_t(ox)];
            const float open = (gap[0] * (1 - tx) + gap[1] * tx) * (1 - tz) + (gap[openCountX] * (1 - tx) + gap[openCountX + 1] * tx) * tz;
            if (const TileKey local = tileAt(Level::Local, {x, z}); !(local == caveKey)) {
                caveKey = local;
                caves = caveFieldAt(world, {x, z});
            }
            for (const BlockSpan& span : std::span(spans, spanCount)) {
                Material* column = span.materials + size_t(iz) * size_t(block.countX) + size_t(ix);
                formColumn(here, ground, open, caves.get(), x, z, span.originY, block.step, span.countY, column, layer);
                cutSeaVoids(sea, x, z, span.originY, block.step, span.countY, column, layer);
            }
        }
    }

    // Stones lie on and in the ground as solids of their own rock.
    const TileKey low = tileAt(Level::Ground, {block.originX - ClastReach, block.originZ - ClastReach}), high = tileAt(Level::Ground, {maxX + ClastReach, maxZ + ClastReach});
    for (const BlockSpan& span : std::span(spans, spanCount)) {
        const CubeLattice cubes{block.originX, span.originY, block.originZ, block.step, block.countX, span.countY, block.countZ};
        for (int z = low.z; z <= high.z; ++z)
            for (int x = low.x; x <= high.x; ++x)
                if (const Tile* tile = fine.tile(x, z)) stampClasts(*tile, cubes, span.materials, edited ? &lift : nullptr);
    }
}

void sampleBlock(World& world, const Block& block, Material* materials) {
    const BlockSpan span{block.originY, block.countY, materials};
    sampleBlocks(world, block, &span, 1);
}

StoneSample stoneAt(World& world, double x, double y, double z) {
    requireOver(world, Level::Ground, x, z, x, z, ClastReach);
    TileReader fine{world.atlas, Level::Ground};
    const bool edited = !world.interventions.overlay.tiles.empty();
    const std::function<float(double, double)> lift = [&](double px, double pz) { return changeAt(world, px, pz).ground(); };
    StoneSample sample;
    const TileKey low = tileAt(Level::Ground, {x - ClastReach, z - ClastReach}), high = tileAt(Level::Ground, {x + ClastReach, z + ClastReach});
    // In the order sampleBlock stamps them, so the stone named is the one whose rock the point reads as.
    for (int tz = low.z; tz <= high.z; ++tz)
        for (int tx = low.x; tx <= high.x; ++tx) {
            const Tile* tile = fine.tile(tx, tz);
            const Clast* stone = tile ? clastAt(*tile, x, y, z, edited ? &lift : nullptr) : nullptr;
            if (!stone) continue;
            const double span = tileSpan(Level::Ground);
            sample.found = true;
            sample.stone = *stone;
            sample.at = {tx * span + double(stone->x), tz * span + double(stone->z)};
            sample.height = stone->y + (edited ? lift(sample.at.x, sample.at.z) : 0.0f);
        }
    return sample;
}

Material materialAt(World& world, double x, double y, double z) {
    Material material;
    // A cube whose edge is a power of two: its centre is then the point itself, to the last bit.
    constexpr double Edge = 1.0 / 64;
    sampleBlock(world, {x - Edge / 2, y - Edge / 2, z - Edge / 2, Edge, 1, 1, 1}, &material);
    return material;
}

} // namespace derivean::sim

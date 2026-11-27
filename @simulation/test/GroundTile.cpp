#include "GroundFixture.hpp"
#include "LandscapeFixture.hpp"

#include "core/fn/Hash.hpp"
#include "ground/fn/Stamp.hpp"
#include "ground/type/Detail.hpp"
#include "intervention/fn/Intervention.hpp"
#include "intervention/fn/Scenario.hpp"
#include "landscape/fn/Build.hpp"
#include "simulation/fn/Query.hpp"

#include <map>

using namespace derivean::sim;

namespace {

// The point as the centre of one cube of a block whose size, edge and alignment are drawn anew each time.
Material throughBlock(World& world, double x, double y, double z, uint64_t draw) {
    const double step = 0.125 * double(1 << (draw % 3));
    const int count = 4 + int((draw >> 4) % 5);
    const int ix = int((draw >> 8) % uint64_t(count)), iy = int((draw >> 16) % uint64_t(count)), iz = int((draw >> 24) % uint64_t(count));
    std::vector<Material> cubes(size_t(count) * size_t(count) * size_t(count));
    sampleBlock(world, {x - (ix + 0.5) * step, y - (iy + 0.5) * step, z - (iz + 0.5) * step, step, count, count, count}, cubes.data());
    return cubes[(size_t(iy) * size_t(count) + size_t(iz)) * size_t(count) + size_t(ix)];
}

// Stones read through lattices of different edge and alignment must give one answer for one point.
void stampsAgree() {
    Tile tile;
    tile.key = {Level::Ground, 3, -2};
    auto detail = std::make_shared<GroundDetail>();
    for (int n = 0; n < 40; ++n) {
        Clast stone;
        // High ground, where single precision holds an elevation to a quarter of a millimetre only.
        stone.x = float(2 + 60 * unit(hash(1, n))), stone.z = float(2 + 60 * unit(hash(2, n))), stone.y = float(3400 + unit(hash(3, n)));
        stone.b = float(0.15 + 0.8 * unit(hash(4, n))), stone.a = 1.4f * stone.b, stone.c = 0.7f * stone.b;
        stone.yaw = float(6.28 * unit(hash(5, n))), stone.pitch = float(0.6 * unit(hash(6, n)) - 0.3), stone.roll = float(0.6 * unit(hash(7, n)) - 0.3);
        stone.roundness = float(unit(hash(8, n)));
        stone.rock = Material::Granite;
        detail->clasts.push_back(stone);
    }
    const auto binOf = [](const Clast& s) { return size_t(int(s.z / float(ClastBinSpan))) * ClastBins + size_t(int(s.x / float(ClastBinSpan))); };
    std::sort(detail->clasts.begin(), detail->clasts.end(), [&](const Clast& a, const Clast& b) { return binOf(a) < binOf(b); });
    detail->binStart.assign(size_t(ClastBins) * ClastBins + 1, 0);
    for (const Clast& stone : detail->clasts) ++detail->binStart[binOf(stone) + 1];
    for (size_t bin = 0; bin < size_t(ClastBins) * ClastBins; ++bin) detail->binStart[bin + 1] += detail->binStart[bin];
    tile.detail = detail;
    const double cornerX = 3 * 64.0, cornerZ = -2 * 64.0;
    int inside = 0;
    for (int n = 0; n < 400; ++n) {
        const Clast& stone = detail->clasts[size_t(n) % detail->clasts.size()];
        const double x = cornerX + stone.x + (unit(hash(9, n)) - 0.5) * 2.4 * stone.a, y = stone.y + (unit(hash(10, n)) - 0.5) * 2.4 * stone.c,
                     z = cornerZ + stone.z + (unit(hash(11, n)) - 0.5) * 2.4 * stone.a;
        Material answers[2];
        for (int way = 0; way < 2; ++way) {
            const double step = way ? 0.5 : 0.02;
            const int count = way ? 6 : 1, at = way ? int(hash(12, n) % 6) : 0;
            std::vector<Material> cubes(size_t(count) * size_t(count) * size_t(count), Material::Air);
            stampClasts(tile, {x - (at + 0.5) * step, y - (at + 0.5) * step, z - (at + 0.5) * step, step, count, count, count}, cubes.data());
            answers[way] = cubes[(size_t(at) * size_t(count) + size_t(at)) * size_t(count) + size_t(at)];
        }
        expectGround(answers[0] == answers[1], "a stone reads differently through another lattice", n);
        inside += answers[0] != Material::Air;
    }
    expectGround(inside > 80 && inside < 380, "the points of the stone check do not straddle stone surfaces", inside);
}

} // namespace

void testGroundTile() {
    stampsAgree();
    World& world = testWorld();
    const Vec2 focus = testFocus();
    // A tile of the second pass: it waits for its two neighbours of the first, east and west.
    TileKey key = tileAt(Level::Ground, focus);
    key.x |= 1, key.z &= ~1;
    // A query first: it names the world to the atlas, as every ground tile of a world must find it.
    requestAround(world, {(key.x + 1.5) * tileSpan(Level::Ground), (key.z + 0.5) * tileSpan(Level::Ground)}, 1.0, Level::Ground);
    const auto tile = world.atlas.require(key);
    expectGround(tile->detail != nullptr, "a ground tile carries no detail");

    // The same tile on this thread alone, without the pool the atlas spreads a tile's own work
    // over, its neighbours made in the opposite order.
    Setting alone = world.atlas.setting();
    alone.workers = nullptr;
    std::map<std::pair<int, int>, std::shared_ptr<const Tile>> mine;
    std::vector<std::shared_ptr<const Tile>> held;
    const std::function<const Tile*(TileKey)> make = [&](TileKey wanted) -> const Tile* {
        if (wanted.level != Level::Ground) {
            held.push_back(world.atlas.require(wanted));
            return held.back().get();
        }
        auto& slot = mine[{wanted.x, wanted.z}];
        if (!slot) {
            const std::vector<TileKey> needed = dependencies(wanted);
            for (size_t i = needed.size(); i-- > 0;) make(needed[i]);
            slot = replayTile(alone, wanted, make);
        }
        return slot.get();
    };
    expectGround(world.atlas.setting().workers != nullptr, "the atlas gives its tiles no pool to spread their work over");
    expectGround(sameTile(*tile, *make(key)), "a ground tile depends on threads or on the order its neighbours were made in");

    const ClastLedger& ledger = tile->detail->ledger;
    expectGround(std::abs(ledger.coarseBefore + ledger.dug - ledger.matrix - ledger.stones) <= 1e-3 * std::max(1.0, ledger.coarseBefore), "stones and matrix do not add up to the coarse layer",
                 ledger.coarseBefore + ledger.dug - ledger.matrix - ledger.stones);
    double stones = 0;
    for (const Clast& stone : tile->detail->clasts) stones += stone.volume();
    expectGround(std::abs(stones - ledger.stones) <= 1e-3 * std::max(1.0, ledger.stones), "the stones of a tile are not the stones its ledger counts", stones - ledger.stones);

    // One point, one material, whatever block it is asked through: on both sides of the edge to the
    // western neighbour, in the ground, above it, and in and around stones.
    const double span = tileSpan(Level::Ground), edge = key.x * span, north = key.z * span;
    int differing = 0, solid = 0, inStone = 0;
    for (int n = 0; n < 500; ++n) {
        double x = edge + (unit(hash(21, n)) - 0.5) * 12, z = north + 16 + 32 * unit(hash(22, n));
        double y = surveyAt(world, {x, z}, Level::Ground).ground + (unit(hash(23, n)) - 0.5) * 3;
        const std::vector<Clast>& clasts = tile->detail->clasts;
        if (n % 2 && !clasts.empty()) {
            const Clast& stone = clasts[hash(24, n) % clasts.size()];
            // Far enough from the tile's other edges that no block around the point needs a tile beyond them.
            if (stone.x < span - 14 && stone.z > 14 && stone.z < span - 14) {
                x = edge + stone.x + (unit(hash(25, n)) - 0.5) * stone.a, z = north + stone.z + (unit(hash(26, n)) - 0.5) * stone.a, y = stone.y + (unit(hash(27, n)) - 0.5) * stone.c;
                ++inStone;
            }
        }
        const Material direct = materialAt(world, x, y, z);
        differing += direct != throughBlock(world, x, y, z, hash(28, n));
        solid += direct != Material::Air;
    }
    expectGround(differing == 0, "materialAt disagrees with sampleBlock", differing);
    expectGround(solid > 100 && solid < 480, "the points of the block check do not straddle the surface", solid);
    // The tile under the focus may be bare of stones; stampsAgree covers them wherever it is.
    expectGround(tile->detail->clasts.size() < 40 || inStone > 20, "the block check reached no stones in a tile that has them", inStone);

    // A stone names itself where its rock is read.
    for (const Clast& stone : tile->detail->clasts) {
        if (stone.x < 14 || stone.z < 14 || stone.x > span - 14 || stone.z > span - 14) continue;   // one whose reach stays inside the tile
        const StoneSample named = stoneAt(world, edge + stone.x, stone.y, north + stone.z);
        expectGround(named.found && materialAt(world, edge + stone.x, stone.y, north + stone.z) == named.stone.rock, "a stone is not found at its own centre", stone.x);
        break;
    }

    // Close to an edge whose far side nobody has asked for yet: the survey must build what it reads
    // across the edge, and so answer the same before and after more ground around it exists.
    // Earlier checks on the shared world may have finished ground west of the tile (testChart's
    // ground survey at the focus does), so the edge is the first one westward with nothing beyond.
    TileKey west{Level::Ground, key.x - 1, key.z};
    while (world.atlas.find({Level::Ground, west.x - 1, west.z})) --west.x;
    const Vec2 near{west.x * span + 0.3, north + 20.1};
    expectGround(world.atlas.find({Level::Ground, west.x - 1, west.z}) == nullptr, "the tile across the edge was built before the edge was asked for");
    const SurfaceSample first = surveyAt(world, near, Level::Ground);
    expectGround(world.atlas.find({Level::Ground, west.x - 1, west.z}) != nullptr, "a survey beside an edge did not build the tile it reads across it");
    // With every tile it reads in place, the answer that never waits must be the same answer.
    const SurfaceSample second = surfaceAt(world, near, Level::Ground);
    expectGround(std::memcmp(&first.ground, &second.ground, sizeof(float)) == 0 && first.rock == second.rock && first.water == second.water && first.sediment[2] == second.sediment[2],
                 "a survey beside an edge is not what the finished tiles on both sides give", second.ground - first.ground);

    // One edit: the site cell under a point of the tile cut two metres down. Surface and voxels
    // must both show the pit, a point must still read alike through any block, and forgetting the
    // edit must give the untouched ground back.
    const double site = specOf(Level::Site).cell;
    const int64_t cellX = int64_t(std::floor((edge + 30.0) / site)), cellZ = int64_t(std::floor((north + 30.0) / site));
    const Vec2 middle{(double(cellX) + 0.5) * site, (double(cellZ) + 0.5) * site};
    const float before = surveyAt(world, middle, Level::Ground).ground;
    // The top of the solid in the voxel column over the point: rock and stones may stand over the surface.
    const auto solidTop = [&] {
        for (double y = before + 6.0; y > before - 6.0; y -= 0.125)
            if (materialAt(world, middle.x, y, middle.z) != Material::Air) return y;
        return double(before) - 6.0;
    };
    const double topBefore = solidTop();
    intervene(world, siteCell(cellX, cellZ, surveyAt(world, middle).ground - 2.0));
    const float after = surveyAt(world, middle, Level::Ground).ground;
    // The cut takes two metres off the site cell's column; the ground tile's own cover there may be thinner than what was cut of the site's.
    expectGround(after < before - 1.0f && after > before - 2.5f, "a pit two metres deep does not show in the surface", before - after);
    expectGround(surfaceAt(world, middle, Level::Ground).ground == after, "the surface read without waiting differs from the survey over an edit");
    expectGround(std::abs(topBefore - solidTop() - double(before - after)) < 0.3, "a pit does not show in the voxels as it does in the surface", topBefore - solidTop());
    int differingOverEdit = 0;
    for (int n = 0; n < 60; ++n) {
        const double x = middle.x + (unit(hash(31, n)) - 0.5) * 7, z = middle.z + (unit(hash(32, n)) - 0.5) * 7, y = before - 3.5 + 4.5 * unit(hash(33, n));
        differingOverEdit += materialAt(world, x, y, z) != throughBlock(world, x, y, z, hash(34, n));
    }
    expectGround(differingOverEdit == 0, "materialAt disagrees with sampleBlock over an edit", differingOverEdit);
    forgetInterventions(world);
    expectGround(surveyAt(world, middle, Level::Ground).ground == before, "the ground does not come back when the edit is forgotten");
}

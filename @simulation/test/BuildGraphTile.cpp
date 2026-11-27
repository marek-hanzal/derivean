#include "GroundFixture.hpp"

#include "ground/type/Detail.hpp"
#include "measure/fn/Digest.hpp"
#include "simulation/fn/BuildGraph.hpp"
#include "simulation/type/World.hpp"

#include <algorithm>
#include <future>
#include <memory>

namespace derivean::sim {

// Use finished inputs in fresh worlds to exercise the two build paths without replaying the planet's terrain.
struct BuildGraphFixture {
    static void seed(World& world, std::shared_ptr<const Tile> site, std::shared_ptr<const GroundwaterTile> aquifer) {
        const TileKey siteKey = site->key;
        const TileKey aquiferKey = aquifer->key;
        world.atlas.slots_.emplace(siteKey, Atlas::Slot{std::move(site)});
        ++world.atlas.finished_;
        world.groundwater.slots_.emplace(aquiferKey, Aquifers::Slot<GroundwaterTile>{std::move(aquifer)});
    }
};

} // namespace derivean::sim

namespace {

using namespace derivean::sim;

constexpr TileKey GroundKey{Level::Ground, 8, 8};
constexpr BuildKey GroundNode{BuildProduct::Terrain, GroundKey};
constexpr Vec2 Vent{532.0, 544.0};

std::shared_ptr<const Tile> slope() {
    auto site = std::make_shared<Tile>();
    site->key = {Level::Site, 0, 0};
    constexpr size_t Cells = size_t(TileCells) * TileCells;
    site->now.resize(Cells);
    for (size_t i = 0; i < Cells; ++i)
        site->now.rock[i] = site->now.datum[i] = 1000.0f - 0.4f * float(i % TileCells);
    site->now.sediment[Fines].assign(Cells, 0.2f);
    for (auto* field : {&site->discharge, &site->load, &site->ice, &site->iceAge, &site->iceSpeed, &site->uplift, &site->rain, &site->shed})
        field->assign(Cells, 0.0f);
    site->water.assign(Cells, -1e9f);
    site->flowTo.assign(Cells, 255);
    site->rock.assign(Cells, uint8_t(Material::Granite));
    return site;
}

std::shared_ptr<const GroundwaterTile> springAquifer() {
    auto local = std::make_shared<GroundwaterTile>();
    local->key = tileAt(Level::Local, Vent);
    local->springs.push_back({.position = Vent, .discharge = 0.006f, .temperature = 88, .silica = 0.05f, .carbonate = 0.5f, .id = 7});
    const double cell = specOf(Level::Local).cell;
    local->records.push_back({.kind = SpringKind::Karst, .cell = int32_t(Vent.z / cell) * TileCells + int32_t(Vent.x / cell)});
    return local;
}

} // namespace

void testBuildGraphTile() {
    const auto site = slope();
    const auto aquifer = springAquifer();
    World demand({.seed = 42, .age = 0, .threads = 1});
    BuildGraphFixture::seed(demand, site, aquifer);
    const auto expected = demand.atlas.require(GroundKey);
    expectGround(expected && expected->detail, "the Demand build did not publish ground detail");
    expectGround(std::ranges::any_of(expected->detail->crust, [](float crust) { return crust > 0; }) &&
        std::ranges::any_of(expected->discharge, [](float discharge) { return discharge > 0; }),
        "the aquifer fixture's spring did not wet or deposit on the ground");

    for (const unsigned threads : {1u, 4u}) {
        World graph({.seed = 42, .age = 0, .threads = threads});
        BuildGraphFixture::seed(graph, site, aquifer);
        BuildKeys seen;
        std::vector<BuildKey> ready;
        buildFrontier(graph, GroundNode, seen, ready);
        expectGround(ready == std::vector{GroundNode}, "the build graph did not admit the prepared Ground tile", threads);
        const auto build = std::make_shared<std::packaged_task<void()>>([&graph, key = ready.front()] { buildReady(graph, key); });
        auto completed = build->get_future();
        graph.workers.submit([build] { (*build)(); });
        completed.get();
        const auto actual = graph.atlas.find(GroundKey);
        expectGround(actual && actual->detail, "the graph did not publish ground detail", threads);
        expectGround(digestOf(*expected) == digestOf(*actual), "Demand and graph Ground digests differ", threads);
        expectGround(sameTile(*expected, *actual), "Demand and graph Ground tile bits differ", threads);
    }
}

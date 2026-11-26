#include "nature/service/Nature.hpp"

#include "Expect.hpp"

#include <memory>
#include <stdexcept>
#include <string_view>

using namespace derivean;

namespace {

void testReplan() {
    std::promise<void> runningStarted, otherStarted;
    Nature nature(NatureOptions{42, 0, 2});
    std::promise<void> release;
    const auto held = release.get_future().share();
    sim::Workers& workers = nature.world.workers;
    TileRequests& tiles = nature.tiles;
    const auto running = std::make_shared<std::packaged_task<void()>>([&, held] {
        runningStarted.set_value();
        held.wait();
        throw std::runtime_error("retired build failed");
    });
    const sim::BuildKey old{sim::BuildProduct::Terrain, {sim::Level::Region, 100, 0}};
    tiles.flying.push_back({old, workers.submit([running] { (*running)(); }), running->get_future()});
    workers.submit([&, held] {
        otherStarted.set_value();
        held.wait();
    });
    runningStarted.get_future().wait();
    otherStarted.get_future().wait();

    // Both worker threads are occupied. This second old-view request has not started.
    const auto queued = std::make_shared<std::packaged_task<void()>>([] {});
    const uint64_t ticket = workers.submit([queued] { (*queued)(); });
    tiles.flying.push_back({{sim::BuildProduct::Terrain, {sim::Level::Region, 102, 0}}, ticket, queued->get_future()});

    constexpr double Clearance = 10000;
    requestTiles(nature, {0, Clearance, 0}, {}, Clearance);
    expect(tiles.retired.size() == 1 && tiles.retired.front().key == old, "Replanning lost the running request");
    expect(tiles.flying.size() == 1 && tiles.flying.front().key.tile.x == 0, "An old request kept the new queue slot");
    expect(!workers.cancelQueued(ticket), "The obsolete queued request was left in the pool");

    // Returning to the old spot must keep its running build and remove the new queued one.
    requestTiles(nature, {100 * sim::tileSpan(sim::Level::Region), Clearance, 0}, {}, Clearance);
    expect(tiles.flying.empty() && tiles.retired.size() == 1, "Replanning submitted a duplicate of the retired build");

    release.set_value();
    tiles.retired.front().completion.wait();
    bool reported = false;
    try {
        requestTiles(nature, {}, {}, Clearance);
    } catch (const std::runtime_error& error) {
        reported = std::string_view(error.what()) == "retired build failed";
    }
    expect(reported && tiles.retired.empty(), "The retired result or its exception was not collected");
}

} // namespace

// Above the ground's ceiling nothing is the player's own place, so every request comes from the
// shared budget: on a pool of one worker it must still hold that worker, or the view is never built.
void testRequests() {
    for (const size_t workers : {size_t(1), size_t(2), size_t(4), size_t(16)}) {
        const RequestBudget budget = requestBudget(workers);
        expect(budget.own == workers, "The player's own place may use every worker");
        expect(budget.shared >= 1 && budget.shared <= workers, "The rest of the view must be asked for on any pool");
        if (workers > 1) expect(budget.shared < workers, "On a pool of more than one worker the rest of the view leaves one free");
    }
    expect(requestBudget(1).shared == 1 && requestBudget(2).shared == 1 && requestBudget(16).shared == 12, "Shares of the pool kept free changed");
    testReplan();
}

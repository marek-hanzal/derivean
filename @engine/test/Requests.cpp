#include "nature/type/TileRequests.hpp"

#include "Expect.hpp"

using namespace derivean;

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
}

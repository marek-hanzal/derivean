#pragma once

#include "horizon/type/Patch.hpp"
#include "nature/type/Mailbox.hpp"
#include "voxel/type/Grid.hpp"

#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace derivean {

struct Nature;

struct HorizonNode {
    uint32_t shown{};                    // id the renderer holds; 0 while not displayed
    uint32_t detail{};                   // of the shown patch
    uint8_t dataLevel{};                 // of the shown patch
    int below{};                         // shown nodes strictly under this one
    uint64_t ticket{};                   // build in flight; 0 when none
    std::unique_ptr<FarPatch> built;     // finished, waiting to be swapped in
    float low{}, high{};                 // m, valid once `bounded`
    float error{};                       // m the next finer level would add, valid once `bounded`
    bool bounded{};
    bool stale{};                        // finer tiles than the shown patch was built from have finished
    bool wanted{};                       // part of a change the current plan asks for
};

// The far view: a cut through the quadtree of height-field patches, kept near the one the eye's
// position asks for. What the renderer holds only ever changes by whole swaps (a patch for its four
// children, siblings for their parent, a patch for a better build of itself), so it never shows two
// patches over the same ground and never opens a hole in ground it already showed.
struct Horizon {
    std::unordered_map<PatchKey, HorizonNode, PatchKeyHash> nodes;
    std::unordered_set<PatchKey, PatchKeyHash> desired;
    std::unordered_set<PatchKey, PatchKeyHash> roots;   // coarsest nodes in view
    std::shared_ptr<Mailbox<FarPatch>> finished;
    Position planned{};
    bool started{};
    bool settled{};                      // nothing left to change until a build arrives, tiles finish or the eye moves
    size_t tilesCompared{};              // finished simulation tiles at the last comparison with the plan
    size_t tilesSeen{};                  // finished simulation tiles when `audit` was last filled
    std::vector<PatchKey> audit;         // shown patches still to be compared with the tiles now finished
    size_t flying{};
    size_t missing{};                    // patches the plan wants that the renderer does not hold yet
    uint64_t tickets{};
    uint32_t ids{};
    std::array<size_t, sim::LevelCount> shownByData{};   // shown patches by the coarsest data level they were built from
    size_t builds{};                     // patches built so far and the background time they took
    double buildMs{};
};

// One frame of the far view around `eye`, which stands `clearance` metres above the ground; fills
// `delta` with what the renderer must add and drop.
void advanceHorizon(Horizon& horizon, Nature& nature, Position eye, double clearance, FarDelta& delta);

// Chooses the cut the eye's position asks for into `horizon.desired` and `horizon.roots`.
void planHorizon(Horizon& horizon, const sim::World& world, Position eye);

// Distance in metres from the eye to the node's box; the box is flat at the eye's height while unbounded.
double distanceTo(const Horizon& horizon, Position eye, PatchKey key);

} // namespace derivean

#include "horizon/type/Horizon.hpp"
#include "nature/service/Nature.hpp"

#include <algorithm>
#include <cmath>

namespace derivean {
namespace {

constexpr int MaxAddedPerFrame = 24;      // patches handed to the renderer in one frame
constexpr size_t MaxFlying = 64;
constexpr double ReplanDistance = 4.0;    // m the eye moves before the cut is chosen again
constexpr double ReplanShare = 0.03;      // or this share of its height above the ground, from where nothing nearer can be seen
constexpr int AuditSlice = 96;            // shown patches compared with the finished tiles per frame

enum class Change { Show, Refresh, Split, Merge, Remove };

struct Step {
    double distance;
    Change change;
    PatchKey key;
};

struct Pass {
    Horizon& horizon;
    Nature& nature;
    FarDelta& delta;
    size_t tiles;
    int budget{MaxAddedPerFrame};
    bool spent{};      // a swap was ready and had to wait for the next frame
    bool changed{};    // something was shown, hidden or ordered
};

uint32_t fullDetail(PatchKey key) {
    uint32_t detail = 0;
    for (int probe = 0; probe < 5; ++probe) detail |= uint32_t(sourceFor(key.level)) << (3 * probe);
    return detail;
}

void countBelow(Horizon& horizon, PatchKey key, int change) {
    while (key.level < CoarsestLevel) {
        key = parentOf(key);
        horizon.nodes[key].below += change;
    }
}

void hide(Pass& pass, PatchKey key, HorizonNode& node) {
    if (!node.shown) return;
    --pass.horizon.shownByData[node.dataLevel];
    pass.delta.removed.push_back(node.shown);
    node.shown = 0;
    countBelow(pass.horizon, key, -1);
    pass.changed = true;
}

void show(Pass& pass, PatchKey key, HorizonNode& node) {
    FarPatch patch = std::move(*node.built);
    node.built.reset();
    if (node.shown) {
        pass.delta.removed.push_back(node.shown);
        --pass.horizon.shownByData[node.dataLevel];
    } else {
        countBelow(pass.horizon, key, 1);
    }
    patch.id = node.shown = ++pass.horizon.ids;
    node.detail = patch.detail;
    node.dataLevel = patch.dataLevel;
    ++pass.horizon.shownByData[node.dataLevel];
    node.stale = false;
    pass.delta.added.push_back(std::move(patch));
    --pass.budget;
    pass.changed = true;
}

// True once the node's patch is built; orders it otherwise.
bool ensureBuilt(Pass& pass, PatchKey key) {
    Horizon& horizon = pass.horizon;
    HorizonNode& node = horizon.nodes[key];
    node.wanted = true;
    if (node.built) return true;
    if (node.ticket || horizon.flying >= MaxFlying) return false;
    node.ticket = ++horizon.tickets;
    ++horizon.flying;
    pass.changed = true;
    pass.nature.crew->submit([&world = pass.nature.world, box = horizon.finished, key, ticket = node.ticket] {
        FarPatch patch = buildPatch(world, key);
        patch.ticket = ticket;
        box->post(std::move(patch));
    });
    return false;
}

void apply(Pass& pass, const Step& step) {
    Horizon& horizon = pass.horizon;
    const PatchKey key = step.key;
    switch (step.change) {
        case Change::Remove:
            hide(pass, key, horizon.nodes[key]);
            return;
        case Change::Show:
        case Change::Refresh:
            if (!ensureBuilt(pass, key)) return;
            if (pass.budget < 1) break;
            show(pass, key, horizon.nodes[key]);
            return;
        case Change::Split: {
            bool built = true;
            for (int child = 0; child < 4; ++child) built &= ensureBuilt(pass, childOf(key, child));
            if (!built) return;
            if (pass.budget < 4) break;
            hide(pass, key, horizon.nodes[key]);
            for (int child = 0; child < 4; ++child) show(pass, childOf(key, child), horizon.nodes[childOf(key, child)]);
            return;
        }
        case Change::Merge:
            if (!ensureBuilt(pass, key)) return;
            if (pass.budget < 1) break;
            for (int child = 0; child < 4; ++child) hide(pass, childOf(key, child), horizon.nodes[childOf(key, child)]);
            show(pass, key, horizon.nodes[key]);
            return;
    }
    pass.spent = true;
}

std::vector<Step> compare(Pass& pass, Position eye) {
    Horizon& horizon = pass.horizon;
    std::vector<Step> steps;
    std::unordered_set<PatchKey, PatchKeyHash> queued;
    const auto add = [&](Change change, PatchKey key) {
        if (change != Change::Split && change != Change::Merge) steps.push_back({distanceTo(horizon, eye, key), change, key});
        else if (queued.insert(key).second) steps.push_back({distanceTo(horizon, eye, key), change, key});
    };
    const auto shown = [&](PatchKey key) {
        const auto node = horizon.nodes.find(key);
        return node != horizon.nodes.end() && node->second.shown;
    };

    horizon.missing = 0;
    for (const PatchKey key : horizon.desired) {
        HorizonNode& node = horizon.nodes[key];
        node.wanted = true;
        if (node.shown) {
            if (node.built || node.ticket || node.stale) add(Change::Refresh, key);
            continue;
        }
        ++horizon.missing;
        PatchKey above = key;
        bool covered = false;
        while (!covered && above.level < CoarsestLevel) covered = shown(above = parentOf(above));
        if (covered) add(Change::Split, above);
        else if (node.below == 0) add(Change::Show, key);
    }

    for (const auto& [key, node] : horizon.nodes) {
        if (!node.shown || horizon.desired.contains(key)) continue;
        PatchKey above = key;
        bool coarser = false;
        while (!coarser && above.level < CoarsestLevel) coarser = horizon.desired.contains(above = parentOf(above));
        if (!coarser) {
            add(horizon.roots.contains(above) ? Change::Split : Change::Remove, key);
            continue;
        }
        const PatchKey parent = parentOf(key);
        bool leaves = true;
        for (int child = 0; child < 4; ++child) {
            const auto sibling = horizon.nodes.find(childOf(parent, child));
            leaves &= sibling == horizon.nodes.end() || sibling->second.below == 0;
        }
        if (leaves) add(Change::Merge, parent);
    }
    return steps;
}

void collect(Horizon& horizon) {
    for (FarPatch& patch : horizon.finished->collect()) {
        --horizon.flying;
        horizon.settled = false;
        const auto found = horizon.nodes.find(patch.key);
        if (found == horizon.nodes.end() || found->second.ticket != patch.ticket) continue;
        HorizonNode& node = found->second;
        node.ticket = 0;
        ++horizon.builds;
        horizon.buildMs += patch.milliseconds;
        node.low = patch.low;
        node.high = patch.high;
        node.error = patch.error;
        node.bounded = true;
        node.built = std::make_unique<FarPatch>(std::move(patch));
    }
}

// A patch built before finer tiles finished is rebuilt from them. Comparing reads the atlas, so
// only a slice of the shown patches is looked at per frame.
void audit(Horizon& horizon, const sim::World& world, size_t tiles) {
    if (tiles != horizon.tilesSeen && horizon.audit.empty()) {
        horizon.tilesSeen = tiles;
        for (const auto& [key, node] : horizon.nodes)
            if (node.shown && node.detail != fullDetail(key)) horizon.audit.push_back(key);
    }
    for (int looked = 0; looked < AuditSlice && !horizon.audit.empty(); ++looked) {
        const PatchKey key = horizon.audit.back();
        horizon.audit.pop_back();
        const auto node = horizon.nodes.find(key);
        if (node == horizon.nodes.end() || !node->second.shown || detailAt(world, key) == node->second.detail) continue;
        node->second.stale = true;
        horizon.settled = false;
    }
}

} // namespace

void advanceHorizon(Horizon& horizon, Nature& nature, Position eye, double clearance, FarDelta& delta) {
    const size_t tiles = nature.world.atlas.finished();
    collect(horizon);
    audit(horizon, nature.world, tiles);
    const double moved = std::hypot(eye[0] - horizon.planned[0], eye[1] - horizon.planned[1], eye[2] - horizon.planned[2]);
    if (!horizon.started || moved > std::max(ReplanDistance, ReplanShare * clearance)) {
        planHorizon(horizon, nature.world, eye);
        horizon.planned = eye;
        horizon.started = true;
        horizon.settled = false;
    }
    if (horizon.settled && tiles == horizon.tilesCompared) return;
    horizon.tilesCompared = tiles;

    Pass pass{horizon, nature, delta, tiles, MaxAddedPerFrame, false, false};
    for (auto& [key, node] : horizon.nodes) node.wanted = false;
    std::vector<Step> steps = compare(pass, eye);
    std::sort(steps.begin(), steps.end(), [](const Step& a, const Step& b) { return a.distance < b.distance; });
    for (const Step& step : steps) apply(pass, step);
    std::erase_if(horizon.nodes, [](const auto& entry) {
        const HorizonNode& node = entry.second;
        return !node.shown && node.below == 0 && !node.wanted;
    });
    // A pass that changed nothing has nothing to do until a build arrives, tiles finish or the eye moves.
    horizon.settled = !pass.spent && !pass.changed;
}

} // namespace derivean

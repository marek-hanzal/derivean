#include "horizon/type/Horizon.hpp"
#include "nature/type/SurfaceReader.hpp"

#include <cmath>
#include <limits>

namespace derivean {
namespace {

constexpr double KeepFactor = 1.1;        // what is already on screen stays until this much beyond the distance that brought it
constexpr float RootRelief = 1500.0f;     // m assumed around a root's middle until it has been built

double boxDistance(Position eye, PatchKey key, float low, float high) {
    const double size = sizeOf(key.level);
    const double dx = std::max({key.x * size - eye[0], eye[0] - (key.x + 1) * size, 0.0});
    const double dz = std::max({key.z * size - eye[2], eye[2] - (key.z + 1) * size, 0.0});
    const double dy = std::max({double(low) - eye[1], eye[1] - double(high), 0.0});
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void select(Horizon& horizon, Position eye, PatchKey key, float low, float high) {
    // A node that is already split stays split a little longer, so an eye pacing along a split
    // distance does not rebuild the same patches over and over. Finer than asked is always valid:
    // past its far range a patch lies exactly on its parent's surface.
    double reach = SplitCells * cellOf(key.level), tolerance = ErrorShare;
    float error = std::numeric_limits<float>::max();
    if (const auto node = horizon.nodes.find(key); node != horizon.nodes.end()) {
        if (node->second.bounded) {
            low = node->second.low;
            high = node->second.high;
            error = node->second.error;
        }
        if (node->second.below > 0) {
            reach *= KeepFactor;
            tolerance /= KeepFactor;
        }
    }
    // Inside its range a node gives way to its children only where they would show: flat ground and
    // deep water stay coarse, and what borders them differs from them by less than can be seen.
    // Levels further down add about as much again, hence twice the measured error.
    const double distance = boxDistance(eye, key, low, high);
    if (key.level > 0 && distance < reach && 2.0 * error > tolerance * std::max(distance, cellOf(key.level))) {
        for (int child = 0; child < 4; ++child) select(horizon, eye, childOf(key, child), low, high);
    } else {
        horizon.desired.insert(key);
    }
}

} // namespace

double distanceTo(const Horizon& horizon, Position eye, PatchKey key) {
    const auto node = horizon.nodes.find(key);
    if (node != horizon.nodes.end() && node->second.bounded) return boxDistance(eye, key, node->second.low, node->second.high);
    return boxDistance(eye, key, float(eye[1]), float(eye[1]));
}

void planHorizon(Horizon& horizon, const sim::World& world, Position eye) {
    horizon.desired.clear();
    horizon.roots.clear();
    const double size = sizeOf(CoarsestLevel);
    const double reach = ViewDistance * KeepFactor;
    SurfaceReader reader = surfaceReader(world, sim::Level::Region);
    for (int z = int(std::floor((eye[2] - reach) / size)); z <= int(std::floor((eye[2] + reach) / size)); ++z)
        for (int x = int(std::floor((eye[0] - reach) / size)); x <= int(std::floor((eye[0] + reach) / size)); ++x) {
            const PatchKey root{CoarsestLevel, x, z};
            const auto node = horizon.nodes.find(root);
            const bool onScreen = node != horizon.nodes.end() && (node->second.shown || node->second.below > 0);
            const double flat = boxDistance({eye[0], 0, eye[2]}, root, 0, 0);
            if (flat > (onScreen ? reach : ViewDistance)) continue;
            horizon.roots.insert(root);
            const sim::SurfaceSample middle = surfaceAt(reader, {(x + 0.5) * size, (z + 0.5) * size});
            const float ground = middle.found ? middle.ground : float(eye[1]);
            select(horizon, eye, root, ground - RootRelief, ground + RootRelief);
        }
}

} // namespace derivean

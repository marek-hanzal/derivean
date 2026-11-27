#pragma once

#include "simulation/type/BuildKey.hpp"

#include <unordered_set>
#include <vector>

namespace derivean::sim {

struct World;
using BuildKeys = std::unordered_set<BuildKey, BuildKeyHash>;

std::vector<BuildKey> buildReads(BuildKey key);
bool built(const World& world, BuildKey key);

// Visit only unfinished nodes; append tasks whose inputs already exist.
// Sharing visited across requests gives a common dependency the rank of its first request.
void buildFrontier(const World& world, BuildKey key, BuildKeys& visited, std::vector<BuildKey>& ready);

// The worker takes one ready node. Its prerequisites are already in the world's stores.
void buildReady(World& world, BuildKey key);

// Builds the targets and everything they read, one frontier at a time: the ready nodes of a wave
// are spread over the world's pool with parallelFor, then the graph is planned again. Blocks until
// every target exists. The caller must hold no build itself (it must not be inside a tile or ground
// water build), or it may wait for its own claim; a pool task that holds none may call it.
void buildAll(World& world, const std::vector<BuildKey>& targets);

} // namespace derivean::sim

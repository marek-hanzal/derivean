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

} // namespace derivean::sim

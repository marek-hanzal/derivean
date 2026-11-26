#pragma once

#include <vector>

namespace derivean::sim {

// Replaces `field`, a square of `size` cells, by two box passes over `radius` cells: a cheap
// bell-shaped average with clamped edges. `work` is overwritten.
void blur(std::vector<float>& field, std::vector<float>& work, int size, int radius);

// Replaces `value` by its blurred mean over the cells where `mask` is one, and zero where the
// window holds none of them. `mask` and `work` are overwritten.
void meanOver(std::vector<float>& value, std::vector<float>& mask, std::vector<float>& work, int size, int radius);

} // namespace derivean::sim

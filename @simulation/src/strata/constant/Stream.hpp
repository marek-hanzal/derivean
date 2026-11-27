#pragma once

#include <cstdint>

namespace derivean::sim {

// Seed stream of the crust under the cover. Its offsets: +1 the banding of metamorphic basement,
// +2 and +3 the belts of intrusive bodies, +9 to +11 the folds, +31 and +32 the ragged contact of crust kinds.
inline constexpr uint64_t BasementStream = 0x42534d54;

} // namespace derivean::sim

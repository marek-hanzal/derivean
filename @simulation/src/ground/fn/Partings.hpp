#pragma once

#include "ground/type/Parting.hpp"
#include "rockmass/type/RockMass.hpp"
#include "strata/type/Column.hpp"

namespace derivean::sim {

// The layer of a bed at a depth below the datum; the seam at its base where it has one.
Parting partingAt(uint64_t seed, const Bed& bed, float depth);

// Whether the layers of this rock can carry seams of another: a reader that only wants the
// material may skip the partings of a rock that cannot.
bool hasSeams(Material rock);

// m between the bedding planes of a rock.
float partingPitch(const RockTraits& rock);

// The rock mass a bed belongs to (rockmass/type/RockMass.hpp), asked with the bed's thickness alone:
// the planes of a mass are counted from far away, so every reader must hand over the same bed to
// the last bit, and a bed's top and bottom differ from column to column in theirs.
RockMass massOf(uint64_t seed, const StrataColumn& column, const Bed& bed, double laidDown);

// m, the thickness massOf hands over: the bed's own, to a quarter of a metre.
inline float canonicalThickness(const Bed& bed) { return std::round((bed.bottom - bed.top) * 4.0f) / 4.0f; }

// The traits a ground cell meets: the layer's own rock, weakened along joints. `frost` is the share
// of the year the rock spends in the cracking window: ice splits it along its joints into blocks,
// so the more it works the less of the waste comes off as sand and fines.
RockTraits partingTraits(const Parting& parting, float jointWeak, float frost);

// The habit of the pieces a mass of the given rock releases.
BlockHabit blockHabitOf(const RockMass& mass, const RockTraits& rock);

// The same for a rock known only by its kind, as bed material carried from upstream is.
BlockHabit blockHabitOf(const RockTraits& rock);

} // namespace derivean::sim

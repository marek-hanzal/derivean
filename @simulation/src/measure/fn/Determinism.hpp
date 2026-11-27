#pragma once

#include "measure/fn/Digest.hpp"
#include "simulation/type/World.hpp"

namespace derivean::sim {

// One world built two ways: the given one, on its own threads and in whatever order it was asked,
// and a fresh one with a single worker thread that asks for every dependency by hand, last first.
// The fresh world is built alongside while the given one does its part. Its block of voxels needs
// the ground water of the focus, and so the 3 x 3 Local and Region tiles around it, on that one
// thread, longer than the rest of the check together; `freshBlock` false leaves it out (the block
// is still cut in `first`).
struct DeterminismReport {
    struct Entry {
        TileKey key;
        TileDigest digest;           // of the first build
        TileDifference difference;   // against the second
    };
    bool planetSame{};
    uint64_t planetDigest{};
    double ledgerDrift{};            // relative difference of the planet's eroded total between the two builds
    std::vector<Entry> tiles;        // every tile both worlds were asked for
    TileDifference replayed;         // the finest tile under the focus replayed once more inside the first world
    // One sampleBlock across the east edge of a first-pass ground tile at the focus, so that it is
    // cut from two ground tiles solved in different passes.
    size_t blockCells{};             // its voxels; 0 unless `finest` is the site level
    size_t blockFresh{};             // of them, different in the second world; 0 unless `freshBlock`
    bool blockRevisitChecked{};      // the ground tiles around were built and the block read again
    size_t blockRevisited{};         // voxels that changed by that
    // m by which the site-level surveyAt half a metre inside a site tile's edge moved once the tile
    // across that edge existed; 0 when the surveyed surface does not depend on what was built
    // first, negative where it could not be measured. Not part of `identical`.
    double surveyEdgeShift{-1};

    bool identical() const;
};

// `orderLevel` and `orderPass` pick the tile next to the focus that the request-order comparison is
// made on: a pass-1 tile waits for two neighbours of its level, a pass-3 tile for eight. `thorough`
// also rebuilds the surroundings of the sampled block, a dozen ground tiles more.
DeterminismReport checkDeterminism(World& first, Vec2 focus, Level finest, Level orderLevel, int orderPass, bool thorough, bool freshBlock = true);

} // namespace derivean::sim

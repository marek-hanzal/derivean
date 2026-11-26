#pragma once

#include "horizon/type/CubeChunk.hpp"
#include "nature/type/Mailbox.hpp"

#include <array>
#include <memory>
#include <optional>
#include <unordered_map>

namespace derivean {

struct Nature;

inline constexpr double CubeView = 260e3;   // m, how far the coarsest level reaches
inline constexpr int CubeBand = 4;          // cubes of its level that a level's chunks cover beyond its box

// One column of chunks of a level.
struct CubeSlot {
    std::vector<uint32_t> shown;          // ids the renderer holds; empty while it has none
    uint64_t detail{};                    // of the shown chunks
    uint8_t dataLevel{};                  // of the shown chunks
    uint64_t ticket{};                    // build in flight; 0 when none
    std::unique_ptr<CubeColumn> built;    // finished, waiting for its frame
    bool stale{};                         // finer tiles than the shown chunk was cut from have finished
};

struct LevelBox {
    Position min, max;
};

// One level of cubes: a square of columns of chunks that follows the eye.
struct CubeLevel {
    bool active{};
    Cell centre{};                                         // column {x, 0, z} the eye was in when the level was planned
    std::unordered_map<Cell, CubeSlot, CellHash> slots;    // every column the level wants, by {x, 0, z}
    std::vector<Cell> todo;                                // columns to build, the nearest last
    size_t wanted{};                                       // columns within the level's reach and the margin built round it
    size_t shown{};                                        // columns the renderer holds
    size_t held{};                                         // of the wanted columns, those the renderer holds
    size_t settled{};                                      // of those, cut from the data the level wants and not due for a rebuild
    std::optional<LevelBox> box;                           // where the level may be drawn this frame
    size_t builds{};                                       // chunks built so far, the time that took and their faces
    double buildMs{};
    size_t faces{};
};

// The far view: every level is whole inside its box, also under the finer levels, so a level that
// is not there yet or cannot keep up with a fast eye leaves the coarser one showing and never a
// hole. Boxes are centred on the eye and nested; a level is drawn inside its own box and outside
// the next finer one. What the renderer holds of a level only changes a column of chunks at a
// time: it is added, dropped, or swapped for a better build of itself in a single frame.
struct FarCubes {
    int reach{128};                       // cubes from the eye to the side of a level's box
    std::array<CubeLevel, CubeLevelCount> levels;   // from the finest
    std::shared_ptr<Mailbox<CubeColumn>> finished;
    std::vector<CubeKey> ready;           // built columns in the order they are handed to the renderer
    std::vector<CubeKey> audit;           // shown columns still to be compared with the tiles now finished
    size_t tilesSeen{};                   // finished simulation tiles when `audit` was last filled
    size_t flying{};
    size_t missing{};                     // columns the levels want that the renderer does not hold as they will finally be
    bool whole{};                         // nothing is missing and every finished tile has been looked at
    uint64_t tickets{};
    uint32_t ids{};
    std::array<size_t, sim::LevelCount> shownByData{};   // chunks the renderer holds, by their data level
};

struct CubeDelta {
    std::vector<CubeChunk> added;
    std::vector<uint32_t> removed;
};

// One frame of the far view around `eye`, which stands `clearance` metres above the ground and
// moves at `speed` metres a second; fills `delta` with what the renderer must add and drop.
void advanceCubes(FarCubes& far, Nature& nature, Position eye, double clearance, double speed, CubeDelta& delta);

} // namespace derivean

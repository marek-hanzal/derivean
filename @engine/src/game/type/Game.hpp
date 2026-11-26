#pragma once

#include "game/type/Command.hpp"
#include "game/type/Frame.hpp"
#include "granular/type/Sand.hpp"
#include "horizon/type/Horizon.hpp"
#include "nature/service/Nature.hpp"
#include "nature/type/Weather.hpp"
#include "terrain/type/TerrainStream.hpp"

#include <string>
#include <variant>

namespace derivean {

inline constexpr int Visibility = 6;   // chunks around the eye in which water and particles are projected

struct DebrisBody {
    JPH::BodyID id;
    float mass;
    bool suspended{};        // waiting in space whose voxels are not loaded
};

struct ShapeResult {
    Cell key;
    uint64_t ticket;
    TerrainShape shape;
};

// Terrain colliders being built off the game thread for chunks near the player.
struct ColliderQueue {
    std::unordered_map<Cell, uint64_t, CellHash> pending;    // chunk → ticket of the shape in flight
    std::shared_ptr<Mailbox<ShapeResult>> finished;
    uint64_t tickets{};
    bool stale{};            // chunks entered or left the world since colliders were last matched
};

// What the renderer already holds of a streamed world, and what waits to be meshed for it.
struct StreamView {
    std::vector<Cell> queue;                                 // waiting to be meshed on the game thread
    std::unordered_set<Cell, CellHash> provisional;          // meshed beside rock that was not loaded yet
    std::unordered_set<Cell, CellHash> water;                // chunks whose water mesh the renderer holds
    std::optional<VoxelBox> box;
    Cell boxCenter{};
    bool boxStale{true};
};

struct Game {
    // Evolves the planet and finds a place to stand, which blocks for seconds to minutes.
    explicit Game(const PlanetOptions& options = {});

    VoxelWorld world;
    WaterState water;
    SandState sand;
    JoltRuntime physics;
    std::unordered_map<Cell, JPH::BodyID, CellHash> terrain;
    std::vector<Cell> colliderCenters;
    std::unordered_set<Cell, CellHash> visible;
    std::unordered_set<Cell, CellHash> dirtyTerrain;
    std::unordered_set<Cell, CellHash> dirtyColliders;
    std::unordered_set<Cell, CellHash> editedTiles;
    std::vector<DebrisBody> bodies;
    std::vector<Cell> landed;
    double debrisEscapedKg{}, excavatedKg{}, accumulator{};
    uint64_t lastWaterProjection{};
    uint16_t buildStock{128};
    bool jumpQueued{};

    // Declared after the physics runtime: collider shapes are built on nature's workers.
    Nature nature;
    Position spawn{};
    sim::Calendar calendar;  // the world clock: the planet's age and the calendar time played since
    Weather weather;
    double clock{};          // s of play
    bool flying{};
    bool held{};             // the player waits for the voxels around them

    TerrainStream stream;
    EditStore edits;
    Horizon horizon;
    std::optional<FarCubes> cubes;   // set in a game whose far terrain is cubes; `horizon` then rests
    ColliderQueue colliders;
    StreamView view;
};

Frame advance(Game& game, const Command& command);

// Hands over what players changed since the last call, for the simulation to take into account.
std::vector<Edit> takeEdits(Game& game);

// What the simulation knows about the ground and the air over a point, as labelled values in
// reading order. Never blocks: it answers from the tiles and the weather that are finished.
using Inspection = std::vector<std::pair<std::string, std::variant<double, std::string>>>;
Inspection inspect(Game& game, double x, double z);

} // namespace derivean

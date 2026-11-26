#pragma once

#include "physics/service/JoltRuntime.hpp"
#include "nature/type/Air.hpp"
#include "chronology/type/Calendar.hpp"
#include "horizon/type/FarCubes.hpp"
#include "horizon/type/Patch.hpp"
#include "particles/type/Particles.hpp"
#include "voxel/type/Mesh.hpp"
#include "water/type/Water.hpp"

namespace derivean {

struct ChunkMesh {
    Cell key;
    Position origin;
    FaceMesh faces;
};

struct DebrisMesh {
    uint32_t id;
    FaceMesh faces;
};

struct DebrisPose {
    uint32_t id;
    Pose pose;
};

struct Target {
    Cell voxel;
    Material material;
};

// Box in metres inside which voxel chunks, not far patches, are what the renderer must draw.
struct VoxelBox {
    Position min, max;
};

// The weather at the player, from the newest finished state of the atmosphere.
struct LocalWeather {
    double temperature{};    // °C of the air at the ground under the player, with the daily cycle
    double rain{}, snow{};   // mm/h; snow as water equivalent
    double windX{}, windZ{}; // m/s near the ground, direction of travel
    double cloudCover{};     // share of the sky
    double humidity{};       // relative, 0 to 1
    double pressure{};       // Pa at the ground
    double snowpack{};       // kg/m² of snow lying on ground of the player's height there, which is mm of water
    double lag{};            // calendar seconds the atmosphere is behind the calendar
};

// One level of far cubes this frame: what of it the view wants, what the renderer holds, and where it may be drawn.
struct CubeLevelView {
    int level{};
    size_t wanted{}, shown{};
    std::optional<LevelBox> box;
};

struct Frame {
    Position eye;
    std::optional<Target> target;
    std::vector<ChunkMesh> chunks;
    std::vector<ChunkMesh> waterMeshes;
    std::vector<Cell> unloaded;
    std::vector<DebrisMesh> spawned;
    std::vector<DebrisPose> debris;
    std::vector<uint32_t> retired;
    ParticleProjection particles;
    Position particleOrigin{};               // m, what the particle positions are measured from
    WaterBudget water;
    std::optional<float> waterDepth;
    size_t editedTiles{}, visibleChunks{}, bodies{};
    uint64_t sandAirborne{}, sandDeposited{}, sandEscaped{};
    double debrisEscapedKg{}, excavatedKg{}, nativeMs{};
    uint16_t buildStock{};
    bool streamingReady{};

    bool flying{};
    FarDelta far;
    std::array<size_t, sim::LevelCount> farData{};   // far patches or cube chunks the renderer holds, by the coarsest data level they were built from
    CubeDelta cubes;
    std::vector<CubeLevelView> cubeLevels;   // from the finest; empty in a game of patches
    std::optional<VoxelBox> voxelBox;
    std::optional<double> ground;            // m, simulated ground under the eye
    std::optional<double> waterLevel;        // m, natural water standing over that ground
    bool siteReady{true}, localReady{true}, regionReady{true};   // simulation tiles the view wants are finished
    std::array<uint32_t, sim::LevelCount> tilesWanted{}, tilesFinished{};   // by sim::Level: what the view wants and how much of it is built
    size_t pendingChunks{}, pendingPatches{};
    double generationMs{};                   // background time of the voxel columns that entered this frame
    double streamingMs{};                    // game-thread time this frame spent bringing the planet in: tiles, voxels, far patches

    sim::Calendar calendar;                  // the world clock
    double dayOfYear{}, hour{};
    Position sun{};                          // unit vector towards the sun at the player; y below zero at night
    LocalWeather weather;
    std::shared_ptr<const sim::CloudField> clouds;   // set only on a frame that carries a new cloud snapshot
    std::shared_ptr<const sim::Atmosphere> cloudsAir;   // the state of the atmosphere that snapshot was made from
    double cloudsTime{};                     // calendar seconds that snapshot belongs to
};

} // namespace derivean

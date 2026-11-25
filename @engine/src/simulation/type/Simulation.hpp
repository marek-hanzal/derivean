#pragma once

#include "granular/type/Sand.hpp"
#include "simulation/type/JoltRuntime.hpp"
#include "water/type/Water.hpp"

#include <unordered_set>

namespace derivean {

inline constexpr int Visibility = 6;
inline constexpr int MaxBrushRadius = 13;
inline constexpr int DefaultBrushRadius = 7;
inline constexpr Position Spawn{0.5, 0.025, 1.7};

struct Command {
    double elapsed{};
    float x{}, z{};
    bool jump{}, sprint{}, active{};
    Position direction{};
    int radius{DefaultBrushRadius};
    BrushShape shape{BrushShape::Sphere};
    bool dig{}, place{};
};

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

struct DebrisBody {
    JPH::BodyID id;
    float mass;
};

struct Target {
    Cell voxel;
    Material material;
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
    WaterBudget water;
    std::optional<float> waterDepth;
    size_t editedTiles{}, visibleChunks{}, bodies{};
    uint64_t sandAirborne{}, sandDeposited{}, sandEscaped{};
    double debrisEscapedKg{}, excavatedKg{}, nativeMs{};
    uint16_t buildStock{};
    bool streamingReady{};
};

struct Simulation {
    Simulation();

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
};

Frame advance(Simulation& game, const Command& command);

} // namespace derivean

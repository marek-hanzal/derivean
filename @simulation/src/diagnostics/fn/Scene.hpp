#pragma once

#include "diagnostics/type/Scene.hpp"
#include "landscape/type/Terrain.hpp"
#include "measure/fn/Face.hpp"

namespace derivean::sim {

struct World;

// Scene.cpp — reading a scene and putting one together.

// m, the surface at a point: every layer that knows the point, finer over coarser by its weight.
// `cell` receives the cell size of the finest layer that carries most of the answer.
float sceneTop(const Scene& scene, double x, double z, double* cell = nullptr);

// The same blend for everything a cell holds.
SceneCell sceneCell(const Scene& scene, double x, double z);

// The cell a picture draws for a column whose loose ground ends at `ground` metres; `discharge`
// (m³/yr) tints it by the share of the cell a stream of that size covers.
SceneCell paintCell(const Exposed& exposed, float ground, float discharge, double cell);

SceneLayer emptyLayer(double cell, Vec2 firstCentre, int width, int height);

// Fills the cells no tile was built for from the coarser layers, lets the weight of built cells
// fall off towards them and measures the scene's height range. Call once, after every layer is painted.
void finishScene(Scene& scene);

// Gather.cpp — a scene from the world or from one domain.

// Writes a finished tile into the layer of its level. `halfRange` is the seasonal half-range of
// temperature in °C at the tile, from which the lasting snow follows.
void paintTile(SceneLayer& layer, const Tile& tile, double halfRange, bool snow = true);

// One layer from a domain that is being replayed by hand: its whole square, halo included.
SceneLayer layerOf(const Domain& domain);

// Metres from the eye within which each level is built, inside the camera's horizontal field only.
struct SceneReach {
    double site{1600}, local{26e3}, region{150e3}, planet{420e3};
    // Draw lasting snow where the snow balance of the tile's own climate is positive. It is a closed
    // form of that climate, not a deposit the tile holds; without it the ground under it shows.
    bool snow{true};
};

struct SceneCount {
    int site{}, local{}, region{};
};

// Builds the tiles the camera can see and blocks until they are finished. The eye's height is not read.
Scene gatherScene(World& world, const Camera& camera, const SceneReach& reach, SceneCount* count = nullptr);

} // namespace derivean::sim

#include "diagnostics/fn/Scene.hpp"

#include <limits>

namespace derivean::sim {
namespace {

constexpr int Falloff = 16;              // cells over which a built cell's weight rises from an unbuilt one
constexpr float YearSeconds = 3.156e7f;
constexpr float WidthFactor = 4.5f;      // m of channel width per (m³/s)^0.5, as the ground level widens its streams

struct Linear {
    float r{}, g{}, b{};
};

Linear linearOf(Material material) {
    const uint32_t color = definitionOf(material).color;
    const auto channel = [&](int shift) { return std::pow(float((color >> shift) & 255) / 255.0f, 2.2f); };
    return {channel(16), channel(8), channel(0)};
}

Linear mixed(Linear a, Linear b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }

// Position of a point in a layer's cell-centre coordinates; false outside the centres of its outer cells.
bool locate(const SceneLayer& layer, double x, double z, int& ix, int& iz, float& tx, float& tz) {
    const double fx = (x - layer.origin.x) / layer.cell, fz = (z - layer.origin.z) / layer.cell;
    if (fx < 0 || fz < 0 || fx >= layer.width - 1 || fz >= layer.height - 1) return false;
    ix = int(fx), iz = int(fz);
    tx = float(fx - ix), tz = float(fz - iz);
    return true;
}

} // namespace

float sceneTop(const Scene& scene, double x, double z, double* cell) {
    float top = 0, remaining = 1;
    bool named = false;
    for (size_t k = 0; k < scene.layers.size(); ++k) {
        const SceneLayer& layer = scene.layers[k];
        int ix, iz;
        float tx, tz;
        if (!locate(layer, x, z, ix, iz, tx, tz)) continue;
        const size_t a = layer.index(ix, iz), b = a + 1, c = a + size_t(layer.width), d = c + 1;
        const float w00 = (1 - tx) * (1 - tz), w10 = tx * (1 - tz), w01 = (1 - tx) * tz, w11 = tx * tz;
        const float weight = k + 1 == scene.layers.size() ? 1.0f : layer.weight[a] * w00 + layer.weight[b] * w10 + layer.weight[c] * w01 + layer.weight[d] * w11;
        if (weight <= 0) continue;
        top += remaining * weight * (layer.cells[a].top * w00 + layer.cells[b].top * w10 + layer.cells[c].top * w01 + layer.cells[d].top * w11);
        if (cell && !named && weight > 0.5f) *cell = layer.cell, named = true;
        remaining *= 1 - weight;
        if (remaining < 1e-3f) break;
    }
    if (cell && !named) *cell = scene.layers.empty() ? 1.0 : scene.layers.back().cell;
    return top + remaining * float(scene.seaLevel);
}

SceneCell sceneCell(const Scene& scene, double x, double z) {
    SceneCell result;
    float remaining = 1;
    for (size_t k = 0; k < scene.layers.size(); ++k) {
        const SceneLayer& layer = scene.layers[k];
        int ix, iz;
        float tx, tz;
        if (!locate(layer, x, z, ix, iz, tx, tz)) continue;
        const size_t at[4] = {layer.index(ix, iz), layer.index(ix + 1, iz), layer.index(ix, iz + 1), layer.index(ix + 1, iz + 1)};
        const float share[4] = {(1 - tx) * (1 - tz), tx * (1 - tz), (1 - tx) * tz, tx * tz};
        float weight = 0;
        for (int c = 0; c < 4; ++c) weight += layer.weight[at[c]] * share[c];
        if (k + 1 == scene.layers.size()) weight = 1;
        if (weight <= 0) continue;
        for (int c = 0; c < 4; ++c) {
            const SceneCell& cell = layer.cells[at[c]];
            const float part = remaining * weight * share[c];
            result.top += part * cell.top, result.r += part * cell.r, result.g += part * cell.g, result.b += part * cell.b, result.gloss += part * cell.gloss;
        }
        remaining *= 1 - weight;
        if (remaining < 1e-3f) break;
    }
    result.top += remaining * float(scene.seaLevel);
    return result;
}

SceneCell paintCell(const Exposed& exposed, float ground, float discharge, double cell) {
    static const Linear scree = linearOf(Material::Scree), gravel = linearOf(Material::Gravel), sand = linearOf(Material::Sand),
                        silt = linearOf(Material::Silt), snow = linearOf(Material::Snow), ice = linearOf(Material::Ice), water = linearOf(Material::Water);
    SceneCell out;
    out.top = ground;
    const Face face = faceOf(exposed);
    if (face == Face::Water) {
        // Deep water is darker; the bed still shows through the first metre.
        const Linear deep = mixed(water, {0.01f, 0.03f, 0.07f}, std::min(1.0f, exposed.standing / 40.0f));
        out.top = ground + exposed.standing;
        out.r = deep.r, out.g = deep.g, out.b = deep.b, out.gloss = 1;
        return out;
    }
    Linear color = linearOf(exposed.bedrock);
    const float cover = exposed.coarse + exposed.sand + exposed.fines;
    if (cover > 0) {
        const Linear rubble = exposed.slope >= TalusSlope ? scree : gravel;
        const Linear loose{(rubble.r * exposed.coarse + sand.r * exposed.sand + silt.r * exposed.fines) / cover,
                           (rubble.g * exposed.coarse + sand.g * exposed.sand + silt.g * exposed.fines) / cover,
                           (rubble.b * exposed.coarse + sand.b * exposed.sand + silt.b * exposed.fines) / cover};
        color = mixed(color, loose, std::min(1.0f, cover / (2 * BareCover)));
    }
    if (face == Face::Ice) color = ice, out.top = ground + exposed.ice;
    else if (face == Face::Snow) color = snow;
    else if (discharge > 0) {
        const float width = WidthFactor * std::sqrt(discharge / YearSeconds);
        const float share = std::min(1.0f, 2.0f * width / float(cell));
        if (share > 0.05f) color = mixed(color, water, std::pow(share, 0.7f)), out.gloss = share;
    }
    out.r = color.r, out.g = color.g, out.b = color.b;
    return out;
}

SceneLayer emptyLayer(double cell, Vec2 firstCentre, int width, int height) {
    SceneLayer layer;
    layer.cell = cell;
    layer.origin = firstCentre;
    layer.width = width, layer.height = height;
    layer.cells.assign(size_t(width) * size_t(height), SceneCell{});
    layer.weight.assign(layer.cells.size(), 0.0f);
    return layer;
}

void finishScene(Scene& scene) {
    // Coarsest first: a finer layer's gaps are filled from layers that are already complete.
    for (size_t k = scene.layers.size(); k-- > 0;) {
        SceneLayer& layer = scene.layers[k];
        if (k + 1 < scene.layers.size()) {
            Scene coarser;
            coarser.seaLevel = scene.seaLevel;
            coarser.layers.assign(scene.layers.begin() + long(k) + 1, scene.layers.end());
            for (int z = 0; z < layer.height; ++z)
                for (int x = 0; x < layer.width; ++x) {
                    const size_t i = layer.index(x, z);
                    if (layer.weight[i] <= 0) layer.cells[i] = sceneCell(coarser, layer.origin.x + x * layer.cell, layer.origin.z + z * layer.cell);
                }
        }
        // Chamfer distance in cells from every built cell to the nearest unbuilt one or to the edge.
        std::vector<float> distance(layer.cells.size());
        const auto edge = [&](int x, int z) { return x == 0 || z == 0 || x == layer.width - 1 || z == layer.height - 1; };
        for (int z = 0; z < layer.height; ++z)
            for (int x = 0; x < layer.width; ++x) distance[layer.index(x, z)] = layer.weight[layer.index(x, z)] <= 0 || edge(x, z) ? 0.0f : 1e9f;
        const auto relax = [&](int x, int z, int ox, int oz, float cost) {
            if (x + ox < 0 || z + oz < 0 || x + ox >= layer.width || z + oz >= layer.height) return;
            float& here = distance[layer.index(x, z)];
            here = std::min(here, distance[layer.index(x + ox, z + oz)] + cost);
        };
        for (int z = 0; z < layer.height; ++z)
            for (int x = 0; x < layer.width; ++x) relax(x, z, -1, 0, 1), relax(x, z, 0, -1, 1), relax(x, z, -1, -1, 1.414f), relax(x, z, 1, -1, 1.414f);
        for (int z = layer.height - 1; z >= 0; --z)
            for (int x = layer.width - 1; x >= 0; --x) relax(x, z, 1, 0, 1), relax(x, z, 0, 1, 1), relax(x, z, 1, 1, 1.414f), relax(x, z, -1, 1, 1.414f);
        for (size_t i = 0; i < layer.cells.size(); ++i) layer.weight[i] = float(smoothstep(0.0, double(Falloff), distance[i]));
    }
    scene.lowest = std::numeric_limits<float>::max(), scene.highest = std::numeric_limits<float>::lowest();
    for (const SceneLayer& layer : scene.layers)
        for (const SceneCell& cell : layer.cells) scene.lowest = std::min(scene.lowest, cell.top), scene.highest = std::max(scene.highest, cell.top);
}

} // namespace derivean::sim

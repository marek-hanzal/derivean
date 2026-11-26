#include "Coast.hpp"

#include "diagnostics/fn/Png.hpp"
#include "diagnostics/fn/Relief.hpp"
#include "diagnostics/fn/Scene.hpp"
#include "diagnostics/fn/View.hpp"
#include "measure/fn/Face.hpp"

using namespace derivean::sim;

namespace {

Rgb shaded(Rgb color, double shade) {
    return {uint8_t(std::min(255.0, color.r * shade)), uint8_t(std::min(255.0, color.g * shade)), uint8_t(std::min(255.0, color.b * shade))};
}

Rgb blend(Rgb a, Rgb b, double t) { return {uint8_t(a.r + (b.r - a.r) * t), uint8_t(a.g + (b.g - a.g) * t), uint8_t(a.b + (b.b - a.b) * t)}; }

} // namespace

void writeShoreMap(const ShoreGrid& grid, const ShoreSurvey& survey, const std::string& path, int zoom) {
    constexpr Rgb Deep{24, 52, 110}, Shallow{150, 205, 222}, Lagoon{70, 170, 150}, Bench{150, 110, 190};
    constexpr Rgb SandColor{236, 214, 150}, GravelColor{132, 120, 108}, MudColor{128, 96, 66}, RockColor{168, 166, 160};
    constexpr Rgb CliffEdge{214, 46, 40}, StripEdge{240, 140, 30}, Stack{255, 255, 255};
    const int side = grid.side;
    Field<float> ground(side, side);
    for (size_t i = 0; i < grid.rock.size(); ++i) ground.cells[i] = grid.top(i);
    zoom = std::max(1, zoom);
    std::vector<uint8_t> image(size_t(side) * size_t(zoom) * size_t(side) * size_t(zoom) * 3);
    for (int z = 0; z < side; ++z) {
        for (int x = 0; x < side; ++x) {
            const size_t i = grid.index(x, z);
            Rgb color;
            if (grid.wet(i)) {
                const double depth = grid.water[i] - grid.top(i);
                color = blend(Shallow, Deep, std::min(1.0, std::sqrt(depth / 30.0)));
                // A loose bed shows through shallow water.
                const float loose = grid.loose(i);
                if (loose >= BareCover) color = blend(color, grid.cover[Coarse][i] >= CoarseShare * loose ? GravelColor : grid.cover[Sand][i] >= grid.cover[Fines][i] ? SandColor : MudColor, 0.45 * std::exp(-depth / 6.0));
                if (survey.flag[i] & 4) color = blend(color, Lagoon, 0.6);
                if (survey.flag[i] & 1) color = blend(color, Bench, 0.7);
            } else {
                const float loose = grid.loose(i);
                Rgb base = RockColor;
                if (loose >= BareCover) base = grid.cover[Coarse][i] >= CoarseShare * loose ? GravelColor : grid.cover[Sand][i] >= grid.cover[Fines][i] ? SandColor : MudColor;
                color = shaded(base, 0.5 + 0.5 * hillshade(ground, x, z, grid.cell, 1.0));
                if (ShoreKind(survey.kind[i]) == ShoreKind::Cliff) color = CliffEdge;
                if (survey.flag[i] & 2) color = blend(color, StripEdge, 0.6);
                if (survey.flag[i] & 8) color = Stack;
            }
            for (int dz = 0; dz < zoom; ++dz)
                for (int dx = 0; dx < zoom; ++dx) putPixel(image, side * zoom, x * zoom + dx, z * zoom + dz, color);
        }
    }
    writePng(path, side * zoom, side * zoom, image);
}

void writeShoreView(const Domain& domain, Vec2 eye, Vec2 target, double above, const std::string& path) {
    Scene scene;
    scene.seaLevel = domain.seaLevel;
    scene.layers.push_back(layerOf(domain));
    finishScene(scene);
    Camera camera;
    camera.eye = {eye.x, sceneTop(scene, eye.x, eye.z) + above, eye.z};
    camera.heading = std::atan2(target.x - eye.x, -(target.z - eye.z));
    camera.pitch = std::atan2(sceneTop(scene, target.x, target.z) - camera.eye.y, length(target - eye));
    Light light;
    light.azimuth = camera.heading - 1.75;
    light.elevation = 0.56;
    Workers workers(2);
    writePng(path, camera.width, camera.height, renderView(scene, camera, light, workers));
}

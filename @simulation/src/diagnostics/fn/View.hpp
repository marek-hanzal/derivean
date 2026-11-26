#pragma once

#include "core/service/Workers.hpp"
#include "diagnostics/type/Scene.hpp"

namespace derivean::sim {

// Where the ray through the middle of the picture meets the scene.
struct ViewHit {
    bool found{};
    Vec3 at;
    double distance{};   // m
    double cell{};       // m, cell size of the level that is drawn there
};

ViewHit centreHit(const Scene& scene, const Camera& camera);

// A perspective picture of the scene as 8-bit RGB, row-major from the top: the height field is
// marched ray by ray, lit by the sun with the shadows the relief casts, by the sky from above, and
// faded into haze with distance. Nothing is drawn that the scene's cells do not hold.
std::vector<uint8_t> renderView(const Scene& scene, const Camera& camera, const Light& light, Workers& workers);

} // namespace derivean::sim

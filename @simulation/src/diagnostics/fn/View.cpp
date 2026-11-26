#include "diagnostics/fn/View.hpp"

#include "diagnostics/fn/Scene.hpp"

namespace derivean::sim {
namespace {

constexpr double Near = 0.5;             // m
constexpr double StepPerCell = 0.5;      // march step in cells of the level under the ray
constexpr double StepPerRange = 0.0012;  // and in parts of the distance travelled: about one pixel
constexpr double StepPerClearance = 0.25;   // and in parts of the height above the ground
constexpr double ShadowReach = 80e3;     // m
constexpr double ShadowSoftness = 14.0;

struct Color {
    double r{}, g{}, b{};
};

Color operator*(Color a, double s) { return {a.r * s, a.g * s, a.b * s}; }
Color operator+(Color a, Color b) { return {a.r + b.r, a.g + b.g, a.b + b.b}; }
Color blend(Color a, Color b, double t) { return a * (1 - t) + b * t; }

constexpr Color SunLight{1.0, 0.96, 0.88}, SkyLight{0.42, 0.55, 0.78}, Horizon{0.74, 0.82, 0.91}, Zenith{0.20, 0.38, 0.72};

struct Frame {
    Vec3 forward, right, up;
    double tanH{}, tanV{};
};

Frame frameOf(const Camera& camera) {
    Frame frame;
    frame.forward = {std::sin(camera.heading) * std::cos(camera.pitch), std::sin(camera.pitch), -std::cos(camera.heading) * std::cos(camera.pitch)};
    frame.right = {std::cos(camera.heading), 0, std::sin(camera.heading)};
    frame.up = {frame.right.y * frame.forward.z - frame.right.z * frame.forward.y, frame.right.z * frame.forward.x - frame.right.x * frame.forward.z,
                frame.right.x * frame.forward.y - frame.right.y * frame.forward.x};
    frame.tanH = std::tan(0.5 * camera.fov);
    frame.tanV = frame.tanH * camera.height / camera.width;
    return frame;
}

Vec3 sunDirection(const Light& light) {
    return {std::sin(light.azimuth) * std::cos(light.elevation), std::sin(light.elevation), -std::cos(light.azimuth) * std::cos(light.elevation)};
}

Color skyColor(Vec3 direction, Vec3 sun) {
    const Color dome = blend(Horizon, Zenith, std::sqrt(std::max(0.0, direction.y)));
    const double towards = std::max(0.0, dot(direction, sun));
    return dome + SunLight * (0.25 * std::pow(towards, 24) + 4.0 * std::pow(towards, 1800));
}

// Distance along the ray at which it first goes under the surface, or a negative value for the sky.
double march(const Scene& scene, Vec3 eye, Vec3 direction, double start, double reach, double& cell) {
    double t = start, before = start;
    while (t < reach) {
        const Vec3 p = eye + direction * t;
        if (direction.y >= 0 && p.y > scene.highest) return -1;
        const double clearance = p.y - sceneTop(scene, p.x, p.z, &cell);
        if (clearance < 0) {
            double low = before, high = t;
            for (int k = 0; k < 10; ++k) {
                const double middle = 0.5 * (low + high);
                const Vec3 q = eye + direction * middle;
                (q.y < sceneTop(scene, q.x, q.z) ? high : low) = middle;
            }
            return high;
        }
        before = t;
        t += std::max({StepPerCell * cell, StepPerRange * t, StepPerClearance * clearance});
    }
    return -1;
}

// 0 in the shadow the relief casts, 1 in full sun, between the two near a shadow's edge.
double sunlit(const Scene& scene, Vec3 from, Vec3 sun, double cell) {
    double lit = 1, s = 1.5 * cell;
    while (s < ShadowReach) {
        const Vec3 p = from + sun * s;
        if (p.y > scene.highest) break;
        double here;
        const double clearance = p.y - sceneTop(scene, p.x, p.z, &here);
        if (clearance < 0) return 0;
        lit = std::min(lit, ShadowSoftness * clearance / s);
        s += std::max({StepPerCell * here, 0.02 * s, 0.3 * clearance});
    }
    return std::clamp(lit, 0.0, 1.0);
}

Color shade(const Scene& scene, const Light& light, Vec3 eye, Vec3 direction, double t, double cell, Vec3 sun) {
    const Vec3 hit = eye + direction * t;
    // The slope is read over the cell that is drawn or over a pixel's footprint, whichever is wider.
    const double reach = std::max(0.5 * cell, 0.001 * t);
    const double dx = (sceneTop(scene, hit.x + reach, hit.z) - sceneTop(scene, hit.x - reach, hit.z)) / (2 * reach);
    const double dz = (sceneTop(scene, hit.x, hit.z + reach) - sceneTop(scene, hit.x, hit.z - reach)) / (2 * reach);
    const SceneCell surface = sceneCell(scene, hit.x, hit.z);
    const Vec3 ground = normalized({-dx, 1, -dz});
    const Vec3 normal = normalized(ground * (1 - surface.gloss) + Vec3{0, 1, 0} * surface.gloss);
    const double direct = std::max(0.0, dot(normal, sun));
    const double lit = direct > 0 ? sunlit(scene, {hit.x, hit.y + 0.05 * cell, hit.z}, sun, cell) : 0.0;
    // Steep ground sees less sky and more of the lit ground across the valley.
    const double open = 0.5 + 0.5 * normal.y;
    const Color albedo{surface.r, surface.g, surface.b};
    Color color{albedo.r * (SunLight.r * direct * lit * 1.25 + SkyLight.r * 0.34 * open + 0.05 * (1 - open)),
                albedo.g * (SunLight.g * direct * lit * 1.25 + SkyLight.g * 0.34 * open + 0.05 * (1 - open)),
                albedo.b * (SunLight.b * direct * lit * 1.25 + SkyLight.b * 0.34 * open + 0.05 * (1 - open))};
    if (surface.gloss > 0) {
        const Vec3 mirrored = direction - normal * (2 * dot(direction, normal));
        const double fresnel = 0.03 + 0.97 * std::pow(1 - std::max(0.0, -dot(direction, normal)), 5);
        color = blend(color, skyColor({mirrored.x, std::abs(mirrored.y), mirrored.z}, sun), surface.gloss * fresnel);
    }
    const double haze = 1 - std::exp(-t / light.visibility);
    return blend(color, blend(Horizon, SkyLight, 0.25), haze);
}

uint8_t encoded(double linear) { return uint8_t(std::lround(255.0 * std::pow(std::clamp(linear, 0.0, 1.0), 1.0 / 2.2))); }

Vec3 rayThrough(const Camera& camera, const Frame& frame, double px, double py) {
    const double sx = (2 * (px + 0.5) / camera.width - 1) * frame.tanH, sy = (1 - 2 * (py + 0.5) / camera.height) * frame.tanV;
    return normalized(frame.forward + frame.right * sx + frame.up * sy);
}

} // namespace

ViewHit centreHit(const Scene& scene, const Camera& camera) {
    const Frame frame = frameOf(camera);
    ViewHit hit;
    const double reach = scene.layers.empty() ? 0.0 : 0.5 * scene.layers.back().cell * scene.layers.back().width;
    const double t = march(scene, camera.eye, frame.forward, Near, reach, hit.cell);
    if (t < 0) return hit;
    hit.found = true, hit.distance = t, hit.at = camera.eye + frame.forward * t;
    return hit;
}

std::vector<uint8_t> renderView(const Scene& scene, const Camera& camera, const Light& light, Workers& workers) {
    std::vector<uint8_t> image(size_t(camera.width) * size_t(camera.height) * 3);
    if (scene.layers.empty()) return image;
    const Frame frame = frameOf(camera);
    const Vec3 sun = sunDirection(light);
    const double reach = 0.5 * scene.layers.back().cell * scene.layers.back().width;
    workers.parallelFor(camera.width, [&](int begin, int end) {
        for (int px = begin; px < end; ++px) {
            // Bottom row first: without roll a pixel sees farther than the one below it, so its
            // march starts a little short of where that one ended.
            double horizontal = 0;
            for (int py = camera.height - 1; py >= 0; --py) {
                const Vec3 direction = rayThrough(camera, frame, px, py);
                const double flat = std::max(1e-6, std::hypot(direction.x, direction.z));
                double cell = 1;
                const double t = march(scene, camera.eye, direction, std::max(Near, 0.75 * horizontal / flat), reach, cell);
                Color color;
                if (t < 0) color = skyColor(direction, sun);
                else {
                    horizontal = t * flat;
                    color = shade(scene, light, camera.eye, direction, t, cell, sun);
                }
                const size_t at = (size_t(py) * size_t(camera.width) + size_t(px)) * 3;
                image[at] = encoded(color.r), image[at + 1] = encoded(color.g), image[at + 2] = encoded(color.b);
            }
        }
    }, 8);
    return image;
}

} // namespace derivean::sim

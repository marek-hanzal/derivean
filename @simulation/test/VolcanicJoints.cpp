#include "volcanic/fn/Volcanic.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Volcanic joints: " << message << " (" << value << ")\n";
    std::exit(1);
}

void testJoints() {
    const PilePoint point{40.0f, 1.0f, Material::Andesite, 7};
    const PileBed thick{0, 34, Material::Andesite, true}, thin{0, 2, Material::Andesite, true}, ash{0, 34, Material::Tuff, false};
    constexpr int Samples = 4000;
    constexpr double Run = 40.0;
    constexpr float Voxel = 0.125f;
    int open = 0, changes = 0, deep = 0, other = 0, coarse = 0;
    bool was = false;
    for (int k = 0; k < Samples; ++k) {
        const double x = Run * k / Samples;
        const bool in = columnOpen(42, point, thick, x, 3.3, 0.1f, Voxel);
        open += in, changes += k > 0 && in != was, was = in;
        deep += columnOpen(42, point, thick, x, 3.3, 5.0f, Voxel);
        other += columnOpen(42, point, thin, x, 3.3, 0.1f, Voxel) + columnOpen(42, point, ash, x, 3.3, 0.1f, Voxel);
        coarse += columnOpen(42, point, thick, x, 3.3, 0.1f, 1.0f);
    }
    expect(deep == 0 && other == 0, "columns open in sound rock, in a thin sheet or in ash", deep + other);
    expect(coarse == 0, "columns narrower than two points are drawn", coarse);
    expect(open > 0.1 * Samples && open < 0.8 * Samples, "the weathered face of a colonnade is not part open, part rock", double(open) / Samples);
    // Along a line the face turns from rock to air and back about once a column.
    const double width = 0.09 * std::sqrt(34.0), spacing = 2.0 * Run / std::max(1, changes);
    expect(spacing > 0.5 * width && spacing < 3.0 * width, "columns are not as wide as the thickness of their sheet makes them", spacing);
}

// A neck's columns lie across its conduit: on its face they part around the ring and up its height,
// and they close around the ring without a seam.
void testNeckJoints() {
    const Neck neck{Material::Basalt, {100, 200}, 40.0f, 9};
    constexpr int Samples = 4000;
    constexpr float Voxel = 0.125f, Skin = 0.3f;   // m: a point some way into the weathered skin
    const double ring = 0.95 * neck.radius;
    int open = 0, changes = 0, deep = 0, core = 0;
    bool was = false;
    for (int k = 0; k < Samples; ++k) {
        const double bearing = 6.283185307179586 * k / Samples;
        const double x = neck.site.x + ring * std::cos(bearing), z = neck.site.z + ring * std::sin(bearing);
        const bool in = neckOpen(42, neck, x, 12.0, z, Skin, Voxel);
        open += in, changes += k > 0 && in != was, was = in;
        deep += neckOpen(42, neck, x, 12.0, z, 8.0f, Voxel);
        core += neckOpen(42, neck, neck.site.x + 0.5, 12.0, neck.site.z, 0.1f, Voxel);
    }
    expect(deep == 0 && core == 0, "a neck's columns open in sound rock or at its axis", deep + core);
    expect(open > 0.1 * Samples && open < 0.8 * Samples, "the weathered face of a neck is not part open, part rock", double(open) / Samples);
    const double width = 0.09 * std::sqrt(80.0), spacing = 2.0 * 6.283185307179586 * ring / std::max(1, changes);
    expect(spacing > 0.5 * width && spacing < 3.0 * width, "a neck's columns are not as wide as its width makes them", spacing);
    // Either side of the bearing where the ring closes, as on either side of any other.
    int seam = 0, elsewhere = 0;
    for (int k = 0; k < 400; ++k) {
        const double y = 0.05 * k, near = 1e-7;   // rad: a few micrometres apart
        const auto at = [&](double bearing) { return neckOpen(42, neck, neck.site.x + ring * std::cos(bearing), y, neck.site.z + ring * std::sin(bearing), Skin, Voxel); };
        seam += at(3.14159265358979 - near) != at(-3.14159265358979 + near);
        elsewhere += at(1.0 - near) != at(1.0 + near);
    }
    expect(seam <= elsewhere + 1, "a neck's columns break where its ring closes", seam);
    expect(!neckOpen(42, Neck{}, 0, 0, 0, 0.1f, Voxel), "a point outside every conduit is a neck's joint", 1);
}

} // namespace

// The joints of a thick flow and of a neck, as a voxel run draws them.
void testVolcanicJoints() {
    testJoints();
    testNeckJoints();
}

#pragma once

#include <vector>

namespace derivean {

enum class ParticleKind { Water, Sand, Foam };

// World metres: x, y, z, cube size, material kind.
using ParticleProjection = std::vector<float>;

inline void appendParticle(ParticleProjection& pixels, float x, float y, float z, float size, ParticleKind kind) {
    pixels.insert(pixels.end(), {x, y, z, size, float(kind)});
}

} // namespace derivean

#pragma once

#include <vector>

namespace derivean {

enum class ParticleKind { Water, Sand, Foam };

// x, y, z in metres from the projection's origin, cube size, material kind. The origin travels
// beside it in double precision: single precision cannot hold a position measured from the world's origin.
using ParticleProjection = std::vector<float>;

inline void appendParticle(ParticleProjection& pixels, float x, float y, float z, float size, ParticleKind kind) {
    pixels.insert(pixels.end(), {x, y, z, size, float(kind)});
}

} // namespace derivean

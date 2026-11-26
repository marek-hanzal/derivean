#pragma once

#include "material/constant/Materials.hpp"

namespace derivean::sim {

// What an observer meets at the top of a column.
enum class Face : uint8_t { Rock, Talus, Gravel, Sand, Fines, Snow, Ice, Water, Count };

inline constexpr const char* FaceNames[size_t(Face::Count)] = {"bare rock", "talus", "gravel", "sand", "fines", "snow", "ice", "water"};

struct Exposed {
    Material bedrock{Material::Bedrock};
    float coarse{}, sand{}, fines{};   // m of loose cover by class
    float standing{};                  // m of water over the ground; zero or negative where dry
    float ice{};                       // m
    float slope{};                     // rise over run
    bool snow{};                       // snow outlasts the summer here
};

inline constexpr float BareCover = 0.3f;     // m of loose cover under which the rock shows through
inline constexpr float TalusSlope = 0.45f;   // rise over run above which coarse debris lies as a slope of its own
inline constexpr float CoarseShare = 0.3f;   // share of blocks and gravel in a cover from which it reads as rubble
inline constexpr float SnowSlope = 1.4f;     // rise over run above which snow slides off

inline Face faceOf(const Exposed& cell) {
    if (cell.standing > 0) return Face::Water;
    if (cell.ice > 1) return Face::Ice;
    if (cell.snow && cell.slope < SnowSlope) return Face::Snow;
    const float cover = cell.coarse + cell.sand + cell.fines;
    if (cover < BareCover) return Face::Rock;
    // Fallen rock keeps its sand and dust between the blocks: it is the blocks that make it talus.
    if (cell.coarse >= CoarseShare * cover) return cell.slope >= TalusSlope ? Face::Talus : Face::Gravel;
    return cell.sand >= cell.fines ? Face::Sand : Face::Fines;
}

} // namespace derivean::sim

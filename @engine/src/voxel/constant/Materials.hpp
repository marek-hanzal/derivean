#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace derivean {

enum class Material : uint8_t { Air, Moss, Soil, Clay, Shale, Chalk, Basalt, Timber, Leaf, Bedrock, Sand, Water };

enum class Matter { Gas, Solid, Granular, Liquid };

struct MaterialInfo {
    std::string_view name;
    uint32_t color;
    float roughness;
    float density;             // kg / m³
    float support;             // kg carried per face at the current voxel size
    float friction;
    Matter matter{Matter::Solid};
    float angleOfRepose{};       // degrees, for loose grains
    float dynamicViscosity{};    // Pa·s, for liquids
    float surfaceTension{};      // N/m, for liquids
};

inline constexpr std::array Materials{
    MaterialInfo{"Air",     0x000000, 1.00f,    0,     0, 0.00f, Matter::Gas},
    MaterialInfo{"Moss",    0x78875a, 0.96f, 1300,    80, 0.85f},
    MaterialInfo{"Soil",    0x947459, 1.00f, 1550,    95, 0.85f},
    MaterialInfo{"Clay",    0xb26d53, 0.93f, 1900,   130, 0.72f},
    MaterialInfo{"Shale",   0x596c73, 0.87f, 2300,   210, 0.82f},
    MaterialInfo{"Chalk",   0xc9ad84, 0.83f, 2100,   220, 0.75f},
    MaterialInfo{"Basalt",  0x4b6062, 0.79f, 2900,   280, 0.85f},
    MaterialInfo{"Timber",  0xb77949, 0.84f,  650,   160, 0.78f},
    MaterialInfo{"Leaf",    0x9cae62, 0.90f,  100,  0.35f, 0.55f},
    MaterialInfo{"Bedrock", 0x414953, 0.90f, 3100, 10000, 0.92f},
    MaterialInfo{"Sand",    0xc3a478, 0.98f, 1600,    22, 0.67f, Matter::Granular, 33.0f},
    MaterialInfo{"Water",   0x2f939b, 0.16f,  998,     0, 0.00f, Matter::Liquid, 0, 0.001002f, 0.0728f},
};

inline constexpr const MaterialInfo& materialInfo(Material material) {
    return Materials[static_cast<size_t>(material)];
}

} // namespace derivean

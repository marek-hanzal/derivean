#pragma once

#include "material/constant/Materials.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace derivean {

// The simulation module owns every material and its physical definition. The engine keeps only
// the per-voxel view its support, grain and liquid systems read, indexed by the same voxel id.
using Material = sim::Material;

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

constexpr Matter matterOf(sim::Phase phase) {
    switch (phase) {
        case sim::Phase::Gas: return Matter::Gas;
        case sim::Phase::Granular: return Matter::Granular;
        case sim::Phase::Liquid: return Matter::Liquid;
        case sim::Phase::Solid: break;
    }
    return Matter::Solid;
}

constexpr MaterialInfo materialInfoOf(const sim::MaterialDefinition& definition) {
    const Matter matter = matterOf(definition.phase);
    return {
        definition.name,
        definition.color,
        definition.roughness,
        definition.density,
        definition.loadCapacity,
        definition.slidingFriction,
        matter,
        // A solid's friction angle is its internal strength, not a slope loose grains settle to.
        matter == Matter::Granular ? definition.frictionAngle : 0.0f,
        definition.dynamicViscosity,
        definition.surfaceTension,
    };
}

inline constexpr std::array<MaterialInfo, sim::MaterialCount> Materials = [] {
    std::array<MaterialInfo, sim::MaterialCount> materials{};
    for (size_t id = 0; id < materials.size(); ++id) materials[id] = materialInfoOf(*sim::MaterialDefinitions[id]);
    return materials;
}();

inline constexpr const MaterialInfo& materialInfo(Material material) {
    return Materials[static_cast<size_t>(material)];
}

} // namespace derivean

#include "material/constant/Materials.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace derivean::sim;

namespace {

// The first ids are the engine's legacy voxel ids and keep this order.
constexpr std::array<std::string_view, 12> EngineVoxelIds{
    "Air", "Moss", "Soil", "Clay", "Shale", "Chalk", "Basalt", "Timber", "Leaf", "Bedrock", "Sand", "Water",
};

void expect(bool passed, std::string_view material, const char* message) {
    if (passed) return;
    std::cerr << "Material " << material << ": " << message << '\n';
    std::exit(1);
}

float sum(const WeatheringProducts& products) {
    return products.blocks + products.gravel + products.sand + products.silt + products.clay + products.solute;
}

} // namespace

void testMaterials() {
    for (size_t id = 0; id < MaterialDefinitions.size(); ++id) {
        const MaterialDefinition& material = definitionOf(Material(id));
        expect(!material.name.empty(), "<unnamed>", "has no name");
        for (size_t other = 0; other < id; ++other)
            expect(definitionOf(Material(other)).name != material.name, material.name, "name is used twice");
        if (id < EngineVoxelIds.size())
            expect(material.name == EngineVoxelIds[id], material.name, "moved away from its engine voxel id");

        expect(material.porosity >= 0 && material.porosity < 1, material.name, "porosity is outside [0, 1)");
        if (material.phase == Phase::Solid || material.phase == Phase::Granular)
            expect(material.density > 0, material.name, "solid matter has no density");
        const WeatheringProducts& products = material.weathersTo;
        expect(products.blocks >= 0 && products.gravel >= 0 && products.sand >= 0 && products.silt >= 0 &&
               products.clay >= 0 && products.solute >= 0, material.name, "has a negative weathering product");
        if ((material.phase == Phase::Solid && material.weathering > 0) || sum(products) != 0)
            expect(std::abs(sum(products) - 1) <= 1e-3f, material.name, "weathering products do not sum to one");
        expect(material.solidus <= material.liquidus, material.name, "solidus is above its liquidus");
        if (material.phase == Phase::Granular)
            expect(material.frictionAngle > 0 && material.frictionAngle < 60, material.name, "angle of repose is outside (0, 60)");
    }
}

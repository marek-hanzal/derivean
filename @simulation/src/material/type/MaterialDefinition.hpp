#pragma once

#include <cstdint>
#include <string_view>

namespace derivean::sim {

enum class Phase : uint8_t { Gas, Solid, Granular, Liquid };

enum class Origin : uint8_t {
    Void,            // air and other non-rock fillers
    Igneous,
    Metamorphic,
    Sedimentary,
    Unconsolidated,  // loose particles that still remember their source rock
    Fluid,
    Organic,
};

// Volume fractions a rock breaks into when it weathers. They sum to one; transport sorts them later.
struct WeatheringProducts {
    float blocks{};   // boulders and angular scree
    float gravel{};
    float sand{};
    float silt{};     // dust and loess
    float clay{};
    float solute{};   // leaves in solution and never forms a deposit on the slope
};

// One material is one file under material/definition. Each process reads only the group it needs,
// so a new rock is added by writing its definition and listing it in material/constant/Materials.hpp.
struct MaterialDefinition {
    std::string_view name;
    Phase phase{Phase::Solid};
    Origin origin{Origin::Void};

    // Appearance.
    uint32_t color{};               // sRGB 0xRRGGBB of a fresh dry surface
    float roughness{1};             // 0 mirror, 1 fully diffuse

    // Mechanics.
    float density{};                // kg/m³ bulk
    float compressiveStrength{};    // MPa, intact
    float cohesion{};               // kPa
    float frictionAngle{};          // degrees; the angle of repose for granular matter
    float slidingFriction{};        // coefficient against other solids
    float loadCapacity{};           // kg one voxel face carries in the structural check
    float stableSlope{};            // degrees a 10 m face holds before it sheds blocks

    // Surface processes.
    float erodibility{};            // stream power coefficient, m^(1-2m)/yr for m = 0.45
    float abrasion{};               // fraction of clast volume lost per km of bedload travel
    float weathering{};             // m/Myr bare-surface regolith production in a temperate climate
    float solubility{};             // m/Myr of chemical lowering per m/yr of runoff
    float bedding{};                // 0 massive … 1 thinly layered
    float jointSpacing{};           // m between open fractures; 0 means unjointed
    WeatheringProducts weathersTo{};
    float grainSize{};              // m, median particle of loose matter

    // Water. Porosity is the storage volume, permeability the speed; they are independent inputs.
    float porosity{};               // void fraction
    float conductivity{};           // m/s saturated hydraulic conductivity

    // Heat.
    float thermalConductivity{};    // W/(m·K)
    float heatCapacity{};           // J/(kg·K)
    float solidus{};                // K where melting starts; 0 for matter that does not melt here
    float liquidus{};               // K where it is fully molten or vaporised

    // Liquids.
    float dynamicViscosity{};       // Pa·s
    float surfaceTension{};         // N/m

    // The jointed rock mass at the scale of a hillside, as back-analysed from slopes that stand:
    // far below the intact strength above, because a face fails along its joints and beds.
    float massCohesion{};           // kPa for a face 100 m high in rock jointed as usual for its kind; it falls with the cube root of the height
    float massFriction{};           // degrees: the angle a slope of any height holds
};

} // namespace derivean::sim

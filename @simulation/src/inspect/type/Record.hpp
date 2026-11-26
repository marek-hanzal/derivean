#pragma once

#include "landscape/type/Terrain.hpp"
#include "strata/type/Strata.hpp"

#include <string>
#include <vector>

namespace derivean::sim {

// What the simulation knows about one point of the surface, for the player who asks why it looks
// the way it does. Every field carries its unit; rates and totals are kept in separate fields.

struct InspectSurface {
    float ground{};      // m, top of loose or solid ground
    float datum{};       // m, original top of the layering
    float incision{};    // m erosion has cut below that top
    float slope{};       // degrees from horizontal, over the site cells around the point
    float aspect{};      // degrees clockwise from north the ground falls towards; north is -Z
    Level level{Level::Ground};   // finest level the answer comes from
};

// One run of the same material down the column, as the voxels show it.
struct InspectBed {
    float top{}, bottom{};   // m below the ground surface
    Material material{Material::Bedrock};
};

struct InspectRock {
    Material bed{Material::Bedrock};        // first solid rock down the voxel column
    Material modelBed{Material::Bedrock};   // rock the site replay eroded at this cell; differs where the voxel forms add detail
    int unit{};                // strata slot of the bed's unit; OlderCover or BasementUnit below the dated record
    double depositedAgo{};     // yr since that unit began to accumulate; 0 below the dated record
    Facies facies;             // its composition as parts of 255
    Basement basement{Basement::Primordial};   // kind of crust under the cover
    double deformedAgo{};      // yr since the province was last squeezed or intruded
    bool aboveUnconformity{};  // the unit was laid down after that and lies flat across the folded rock below
    float coverTotal{};        // m of sedimentary cover the column had before this replay
    float jointSpacing{};      // m between open fractures of the bed's material; 0 unjointed
    float bedding{};           // 0 massive to 1 thinly layered
    std::vector<InspectBed> column;   // from the ground down to InspectDepth
};

inline constexpr float InspectDepth = 200.0f;   // m of column listed

struct InspectCover {
    float thickness{};                 // m of loose sediment on the rock
    float coarse{}, sand{}, fines{};   // m of each transport class
    Material surface{Material::Air};   // what the top of the ground is made of
};

enum class Regime : uint8_t { Quiet, Compression, Extension, Shear };

// The planet's view of the place: crust, plate and what the nearest contact does to it.
struct InspectCrust {
    Basement basement{Basement::Primordial};
    float elevation{};          // m, planet-scale surface against the datum
    double seaLevel{};          // m
    float thickness{};          // m of felsic crust
    float stock{};              // m of rock erosion can still remove before the column floats level with the datum; negative below it
    float cover{};              // m of dated sedimentary cover
    double lidAge{};            // yr since the lid under the column solidified or left a spreading contact
    double sinceDeformation{};  // yr
    double sinceOrogeny{};      // yr; negative where the crust never stacked in a collision
    float exhumed{};            // m stripped since that stacking
    float erodedTotal{}, depositedTotal{};   // m since the column formed
    float erosionRate{}, depositionRate{};   // m/yr, planet-scale recent mean
    Vec2 drifted;               // m the crust has travelled since it formed
    int plate{}, neighbor{};    // plate under the point and the one across the nearest contact; the same where no contact is within reach
    Vec2 velocity;              // m/yr of the crust under the point
    double contactDistance{};   // m to that contact
    float compression{}, extension{}, shear{};   // m/yr of relative motion at the contact; one of the first two is zero
    double activity{};          // 0..1, how far the current plate arrangement is from its start or end
    Regime regime{Regime::Quiet};
};

// One level's account of vertical motion at the point. `window` is the time its totals cover:
// calendar time for a region, settling time for the finer levels.
struct InspectRates {
    Level level{Level::Site};
    double window{};                           // yr
    float upliftNow{}, upliftTotal{};          // m/yr and m of rock uplift
    float erosionNow{}, erosionTotal{};        // m/yr and m
    float depositionNow{}, depositionTotal{};  // m/yr and m
};

struct InspectWater {
    float rain{};         // m/yr
    float runoff{};       // m/yr of it that runs off
    float discharge{};    // m³/yr through the cell
    float level{};        // m, surface of standing water; meaningful where `standing`
    float depth{};        // m of water over the ground
    bool standing{};
    double seaLevel{};    // m
};

// Annual means from the planet's climate. Weather of the moment (cloud, wind of the hour, rain now)
// belongs here once the atmosphere publishes it per point.
struct InspectAir {
    float warmth{};   // °C at the ground
    Vec2 wind;        // m/s, direction of travel
};

// Heat from below, as the planet's history left it under the point.
struct InspectHeat {
    float gradient{};     // K/m near the surface
    float magmatic{};     // 0..1, share of it owed to melt still cooling below
    double sinceMelt{};   // yr since melt last rose under the point
    float centre{};       // 0..1, closeness to the nearest magmatic centre of the province
    float at100m{};       // °C of the rock 100 m down
};

// Bodies that cut the layering here: sealed fractures and pipes.
struct InspectVeins {
    float veining{};      // 0..1, share of the fracture sets that fluids sealed; 0 for most ground
    double sealedAgo{};   // yr since those fluids moved
    float pipeRadius{};   // m of the pipe that reaches this ground; 0 where there is none
    double pipeAgo{};     // yr since it was punched through
    Material cutting{Material::Air};   // rock that cuts the bed at the top of the solid column; Air where the beds are intact
};

enum class SurfaceOrigin : uint8_t {
    ExposedRock,       // the bed itself, kept bare
    WeatheredInPlace,  // a thin mantle made from the bed underneath
    RiverDeposit,      // laid down by running water
    LakeFloor,         // settled under standing fresh water
    SeaFloor,          // settled under the sea
    Shore,             // within reach of tide and waves
    SlopeDebris,       // fallen from steeper ground above
    SlopeWash,         // soil crept down from the slope above
    Unexplained,       // nothing the simulation recorded accounts for it
};

struct Provenance {
    SurfaceOrigin origin{SurfaceOrigin::Unexplained};
    Material material{Material::Air};   // the surface material that is explained
    std::string reason;                 // one or two sentences built from the measured fields
};

struct InspectRecord {
    Vec2 position;
    bool found{};   // false where no site tile could be read
    InspectSurface surface;
    InspectRock rock;
    InspectCover cover;
    InspectCrust crust;
    InspectRates rates;        // the level the surface was read from
    InspectRates regional;     // region level: the dated history
    float isostaticNow{};      // m/yr of the regional uplift that answers erosion and deposition
    float tectonicNow{};       // m/yr, the rest of it
    InspectWater water;
    InspectAir air;
    InspectHeat heat;
    InspectVeins veins;
    Provenance provenance;
};

} // namespace derivean::sim

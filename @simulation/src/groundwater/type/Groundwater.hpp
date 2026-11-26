#pragma once

#include "groundwater/type/Cave.hpp"
#include "groundwater/type/Spring.hpp"
#include "material/constant/Materials.hpp"
#include "planet/fn/Heat.hpp"

namespace derivean::sim {

enum class SpringKind : uint8_t {
    Table,     // the water table meets the ground
    Contact,   // water held up by a tight bed comes out where the bed crops out
    Fault,     // water that went deep rises along the crushed rock of a fault
    Karst,     // the outlet of a conduit system in soluble rock
};

// Why a spring is where it is and yields what it does. Means are weighted by the water.
struct SpringRecord {
    SpringKind kind{SpringKind::Table};
    float catchment{};       // m² of ground whose recharge it yields
    float recharge{};        // m/yr entering the ground there
    float path{};            // m travelled underground
    float drop{};            // m the ground where the water entered stands above the vent
    float depth{};           // m below the ground the circulation reaches
    float years{};           // yr underground
    Material aquifer{Material::Air};   // rock the water moved in
    Material floor{Material::Air};     // tight bed that turned it out; Air where none did
    Geotherm heat;           // heat from below at the vent
    float reservoir{};       // °C of the rock at that depth
    float dissolvedSilica{}, dissolvedCarbonate{};   // kg/m³ in the water in all; Spring carries only what comes out of it
    float crush{};           // 0..1, closeness of the vent to a domain fault
    float passages{};        // m of listed conduits that feed it
    float grew{};            // yr the largest of them took to open
    int32_t cell{};          // own cell of the tile the water leaves the ground in
    uint8_t storey{};
};

// Why a conduit is where it is.
struct ConduitRecord {
    uint64_t spring{};       // id of the spring it drains to; 0 where its water leaves the tile underground
    Material rock{Material::Limestone};   // soluble rock it is cut in
    Material floor{Material::Air};        // tight bed it runs on; Air where it follows the water table
    float strike{};          // rad, trend of the joint plane it follows; meaningless at order 0
    uint8_t order{};         // 2 a joint zone, 1 a joint corridor, 0 neither: a shaft or a link
    uint8_t level{};         // 0 carries water now; n is the nth abandoned level above the water table
    float catchment{};       // m² that drained through it while it grew
    float grew{};            // yr from open fissure to this size
    float abandoned{};       // yr since the falling water table left it dry; 0 while it carries water
    float cover{};           // m of ground over its roof at its shallower end
};

// m³/yr over a tile's own cells. What enters the ground leaves it again:
// recharge + sunk + lateralIn = springs + seepage + baseflow + deep + underflow + lateralOut.
struct WaterBudget {
    double rain{};          // falling on the tile
    double runoff{};        // of it, left by evaporation: what the landscape routes over the surface
    double recharge{};      // of the runoff, entering the ground where it falls
    double sunk{};          // of routed streams, going underground in soluble rock
    double springs{};       // out at listed vents
    double seepage{};       // out in amounts too small to be a vent
    double baseflow{};      // out into streams, lakes and the sea
    double deep{};          // lost through the base of circulation
    double underflow{};     // followed no farther: paths longer than the model reads, conduits that go on beyond the tile
    double lateralIn{}, lateralOut{};   // across the tile's edge
    double dissolved{};     // m³/yr of rock that leaves in solution with the water that comes out
};

inline double imbalance(const WaterBudget& b) {
    const double in = b.recharge + b.sunk + b.lateralIn, out = b.springs + b.seepage + b.baseflow + b.deep + b.underflow + b.lateralOut;
    return in > 0 ? (in - out) / in : 0.0;
}

// Flags of GroundwaterTile::state.
inline constexpr uint8_t HasPerched = 1, UpperKarst = 2, LowerKarst = 4, Drained = 8, Swallows = 16;

// The ground water of one local tile. Per-cell arrays hold TileCells² entries, row-major. A pure
// function of the world's setting and the tile key.
struct GroundwaterTile {
    TileKey key;
    std::vector<float> table;       // m, the first water met going down: perched where some is, else the lower storey's
    std::vector<float> deepTable;   // m, water level of the lower storey
    std::vector<float> recharge;    // m/yr entering the ground
    std::vector<float> emergence;   // m³/yr the ground returns to the surface in the cell: springs, seeps and baseflow
    std::vector<uint8_t> state;
    std::vector<uint8_t> rock[2];   // Material the water of each storey moves in
    std::vector<uint8_t> from[2];   // neighbour most of a storey's water arrives from; 8 from the storey above; 255 none
    std::vector<Spring> springs;
    std::vector<SpringRecord> records;       // one per spring
    std::shared_ptr<const CaveField> caves;
    std::vector<ConduitRecord> passages;     // one per conduit of `caves`
    WaterBudget budget;
};

struct World;

// The tile's ground water; blocks while it is built, then a lookup.
std::shared_ptr<const GroundwaterTile> groundwaterAt(World& world, TileKey local);

// What an inspector shows for the spring nearest to a point within `reach` metres, and for the
// conduit a point in space lies in. `found` is false where there is none.
struct SpringAccount {
    bool found{};
    Spring spring;
    SpringRecord record;
    std::vector<Material> crossed;   // rock along the main path of its water, from the vent upstream
};
struct CaveAccount {
    bool found{};
    Conduit conduit;
    ConduitRecord record;
    Spring outlet;                   // id 0 where the passage has none in the tile
};
SpringAccount explainSpring(World& world, Vec2 position, double reach = 48.0);
CaveAccount explainCave(World& world, double x, double y, double z);

} // namespace derivean::sim

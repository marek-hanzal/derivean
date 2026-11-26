#include "groundwater/fn/Beds.hpp"

#include "core/fn/Hash.hpp"
#include "ground/fn/Partings.hpp"
#include "groundwater/fn/Conductivity.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t UnitStream = 0x47574154;

// The loose cover as one layer: its classes lie on one another, so water runs along the most
// open of them and has to pass the tightest on its way down.
bool coverLayer(const BedColumn& column, WaterLayer& layer) {
    float thickness = 0, along = 0, across = 0, pores = 0, thickest = 0;
    for (int c = 0; c < SedimentClasses; ++c) {
        const float part = column.cover[c];
        if (part <= 0) continue;
        const MaterialDefinition& rock = definitionOf(CoverRock[c]);
        thickness += part, along += rock.conductivity * part, across += part / rock.conductivity, pores += rock.porosity * part;
        if (part > thickest) thickest = part, layer.rock = CoverRock[c];
    }
    if (thickness < 0.02f) return false;
    layer.top = column.rockTop + thickness, layer.bottom = column.rockTop;
    layer.along = along / thickness * PerYear, layer.through = thickness / across * PerYear;
    layer.porosity = pores / thickness;
    return true;
}

// A rock as one layer between two depths below the rock surface.
WaterLayer rockLayer(const MaterialDefinition& rock, Material kind, float rockTop, float top, float bottom, float openDepth, float crush) {
    const float opened = std::clamp((openDepth - top) / (bottom - top), 0.0f, 1.0f);
    const float joints = jointConductivity(rock, 0.5f * (top + bottom), opened), fault = FaultConductivity * crush * crush;
    WaterLayer layer;
    layer.top = rockTop - top, layer.bottom = rockTop - bottom;
    layer.along = (rock.conductivity + joints + fault) * PerYear;
    layer.through = (rock.conductivity * acrossBedding(rock) + joints + fault) * PerYear;
    layer.porosity = rock.porosity;
    layer.soluble = rock.solubility;
    layer.rock = kind;
    return layer;
}

// Signed metres along a set's normal to its nearest plane of one order.
float planeOffset(uint64_t seed, const RockMass& mass, int set, int order, Vec2 material, float depth) {
    const float here = jointDistance(seed, mass, set, order, material, depth);
    if (here > 1e8f) return 1e9f;
    const Vec2 normal{-std::sin(mass.sets[set].strike), std::cos(mass.sets[set].strike)};
    const float beyond = jointDistance(seed, mass, set, order, material + normal * 0.5, depth);
    return beyond < here ? here : -here;
}

} // namespace

Fissures fissuresAt(uint64_t seed, const RockMass& mass, Vec2 material, float depth) {
    Fissures fissures;
    for (int set = 0; set < JointSets; ++set) {
        fissures.strike[set] = float(mass.sets[set].strike);
        fissures.zone[set] = planeOffset(seed, mass, set, 2, material, depth);
        fissures.corridor[set] = planeOffset(seed, mass, set, 1, material, depth);
    }
    return fissures;
}

void walkBeds(const BedColumn& column, BedWalk& walk) {
    walk.count = 0;
    if (coverLayer(column, walk.layers[0])) walk.count = 1;

    // The layering starts under the pile, at the surface the first lava buried. Its first bed
    // says how the ground is broken, whatever lies on it.
    const float first = std::max(0.0f, column.datum - (column.rockTop - column.pile));   // m below the datum
    const StrataUnit under = unitAt(column.strata, first, column.fold);
    const Bed buried = bedAt(column.seed, column.strata, under, first, column.material);
    walk.top = buried.rock;
    walk.mass = rockMassAt(column.seed, *column.node, buried, under.laidDown, column.material);
    walk.shallowBase = column.rockTop - walk.mass.openDepth;
    walk.base = column.rockTop - (CirculationDepth + walk.mass.crush * walk.mass.crush * FaultDepth);
    const MaterialDefinition* last = &definitionOf(buried.rock);
    Material lastKind = buried.rock;
    if (column.pile > 0.5f) {
        // Lava flows with ash between them: jointed and open along the flows, no more open across them than their ash.
        walk.top = column.lava >= 0.5f ? Material::Basalt : Material::Tuff;
        WaterLayer& pile = walk.layers[walk.count++];
        pile = rockLayer(definitionOf(walk.top), walk.top, column.rockTop, 0, column.pile, walk.mass.openDepth, walk.mass.crush);
        pile.through = std::min(pile.through, 10.0f * definitionOf(Material::Tuff).conductivity * PerYear);
    }

    float at = first;
    for (int beds = 0; beds < WalkBeds && at - first + column.pile < WalkDepth; ++beds) {
        const StrataUnit unit = unitAt(column.strata, at, column.fold);
        const Bed bed = bedAt(column.seed, column.strata, unit, at, column.material);
        const MaterialDefinition& rock = definitionOf(bed.rock);
        // Depths below the rock surface.
        const float top = std::max(bed.top, first) - first + column.pile, bottom = std::min(std::max(bed.bottom, at + 0.25f) - first + column.pile, WalkDepth);
        WaterLayer& layer = walk.layers[walk.count++];
        layer = rockLayer(rock, bed.rock, column.rockTop, top, bottom, walk.mass.openDepth, walk.mass.crush);
        // Basement is one rock all the way down and a pluton has no beds: neither is a sheet that
        // could hold water up along its top.
        const bool bedded = unit.slot != BasementUnit && bed.rock != Material::Granite;
        layer.unit = bedded ? uint32_t(hash(UnitStream, unit.slot, int64_t(unit.laidDown / StrataSlotYears)) | 1) : 0;
        layer.offset = bed.top - unit.top;
        if (!walk.soluble && solubleRock(rock) && bed.bottom - bed.top >= 2.0f) {
            walk.soluble = true;
            walk.solubleMass = massOf(column.seed, *column.node, bed, unit.laidDown);
            walk.solubleDepth = std::min(bed.bottom, first + WalkDepth);
        }
        last = &rock, lastKind = bed.rock;
        at = std::max(bed.bottom, at + 0.25f) + 0.01f;
    }
    // Below what was read bed by bed, the last rock goes on to where circulation ends. In the
    // crushed rock of a fault that is kilometres down, and the fault is what conducts there.
    const float reached = column.rockTop - std::max(column.pile, std::min(WalkDepth, at - first + column.pile));
    if (walk.base < reached) walk.layers[walk.count++] = rockLayer(*last, lastKind, column.rockTop, column.rockTop - reached, column.rockTop - walk.base, 0, walk.mass.crush);
}

} // namespace derivean::sim

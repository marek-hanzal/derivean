#include "groundwater/fn/Beds.hpp"

#include "core/fn/Hash.hpp"
#include "ground/fn/Partings.hpp"
#include "groundwater/fn/Conductivity.hpp"

namespace derivean::sim {
namespace {

constexpr Material CoverRock[SedimentClasses] = {Material::Gravel, Material::Sand, Material::Silt};
constexpr uint64_t UnitStream = 0x47574154;

// The loose cover as one layer: its classes lie on one another, so water runs along the most
// open of them and has to pass the tightest on its way down.
bool coverLayer(const BedColumn& column, Layer& layer) {
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

    const float surface = std::max(0.0f, column.datum - column.rockTop);   // m below the datum
    float at = surface;
    const MaterialDefinition* last = nullptr;
    for (int bedCount = 0; bedCount < WalkBeds && at < surface + WalkDepth; ++bedCount) {
        const StrataUnit unit = unitAt(column.strata, at, column.fold);
        const Bed bed = bedAt(column.seed, column.strata, unit, at, column.material);
        const MaterialDefinition& rock = definitionOf(bed.rock);
        if (bedCount == 0) {
            walk.top = bed.rock;
            walk.mass = rockMassAt(column.seed, *column.node, bed, unit.laidDown, column.material);
            walk.shallowBase = column.rockTop - walk.mass.openDepth;
            walk.base = column.rockTop - (CirculationDepth + walk.mass.crush * walk.mass.crush * FaultDepth);
        }
        const float top = std::max(bed.top, surface), bottom = std::min(std::max(bed.bottom, top + 0.25f), surface + WalkDepth);
        const float opened = std::clamp((walk.mass.openDepth - (top - surface)) / (bottom - top), 0.0f, 1.0f);
        const float joints = jointConductivity(rock, 0.5f * (top + bottom) - surface, opened);
        const float fault = FaultConductivity * walk.mass.crush * walk.mass.crush;
        Layer& layer = walk.layers[walk.count++];
        layer.top = column.datum - top, layer.bottom = column.datum - bottom;
        layer.along = (rock.conductivity + joints + fault) * PerYear;
        layer.through = (rock.conductivity * acrossBedding(rock) + joints + fault) * PerYear;
        layer.porosity = rock.porosity;
        layer.soluble = rock.solubility;
        layer.rock = bed.rock;
        // Basement is one rock all the way down and a pluton has no beds: neither is a sheet that
        // could hold water up along its top.
        const bool bedded = unit.slot != BasementUnit && !(bed.rock == Material::Granite && bed.bottom >= unit.bottom);
        layer.unit = bedded ? uint32_t(hash(UnitStream, unit.slot, int64_t(unit.laidDown / StrataSlotYears)) | 1) : 0;
        layer.offset = bed.top - unit.top;
        if (!walk.soluble && solubleRock(rock) && bed.bottom - bed.top >= 2.0f) {
            walk.soluble = true;
            walk.solubleMass = rockMassAt(column.seed, *column.node, Bed{0, canonicalThickness(bed), bed.rock}, unit.laidDown, column.material);
            walk.solubleDepth = std::min(bed.bottom, surface + WalkDepth);
        }
        last = &rock;
        at = std::max(bed.bottom, at + 0.25f) + 0.01f;
    }
    // Below the beds read one by one, the last rock goes on to where circulation ends. In the
    // crushed rock of a fault that is kilometres down, and the fault is what conducts there.
    const float walked = column.rockTop - WalkDepth;
    if (last && walk.base < walked) {
        const float middle = 0.5f * (column.rockTop - walk.base + WalkDepth);
        const float fault = FaultConductivity * walk.mass.crush * walk.mass.crush;
        Layer& layer = walk.layers[walk.count++];
        layer = Layer{};
        layer.top = walked, layer.bottom = walk.base;
        layer.along = (last->conductivity + jointConductivity(*last, middle, 0) + fault) * PerYear;
        layer.through = (last->conductivity * acrossBedding(*last) + jointConductivity(*last, middle, 0) + fault) * PerYear;
        layer.porosity = last->porosity;
        layer.rock = walk.layers[walk.count - 2].rock;
    }
}

} // namespace derivean::sim

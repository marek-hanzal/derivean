#include "strata/type/Column.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "planet/fn/Sample.hpp"
#include "strata/constant/Stream.hpp"
#include "strata/fn/Window.hpp"

namespace derivean::sim {
namespace {

RockTraits traitsFrom(Material material) {
    const MaterialDefinition& d = definitionOf(material);
    RockTraits traits;
    traits.erodibility = d.erodibility;
    traits.stableTan = std::tan(std::min(d.stableSlope, 85.0f) * 0.0174533f);
    traits.weathering = d.weathering / 1e6f;
    traits.solubility = d.solubility / 1e6f;
    traits.abrasion = d.abrasion / 1e3f;
    traits.coarse = d.weathersTo.blocks + d.weathersTo.gravel;
    traits.sand = d.weathersTo.sand;
    traits.fines = d.weathersTo.silt + d.weathersTo.clay;
    traits.solute = d.weathersTo.solute;
    if (traits.coarse + traits.sand + traits.fines + traits.solute <= 0) traits.sand = 1;
    traits.porosity = d.porosity;
    traits.conductivity = d.conductivity;
    traits.bedding = d.bedding;
    traits.jointSpacing = d.jointSpacing;
    traits.dominant = material;
    return traits;
}

} // namespace

const RockTraits& traitsOf(Material material) {
    static const auto table = [] {
        std::array<RockTraits, size_t(MaterialCount)> entries;
        for (int i = 0; i < MaterialCount; ++i) entries[size_t(i)] = traitsFrom(Material(i));
        return entries;
    }();
    return table[size_t(material)];
}

// Kinds of crust do not blend. Their contact is read through a ragged seeded offset, so it
// interfingers at every scale instead of following the edge of a planet cell.
const CrustColumn& crustOwner(const Planet& planet, Vec2 position) {
    const uint64_t ragged = stream(planet.seed, BasementStream) + 31;
    const Vec2 offset{37e3 * fractalNoise(ragged, position.x / 83e3, position.z / 83e3, 5, 0.6),
                      37e3 * fractalNoise(ragged + 1, position.x / 83e3, position.z / 83e3, 5, 0.6)};
    return planetColumn(planet, position + offset);
}

Vec2 crustFrame(const Planet& planet, Vec2 position) {
    const CrustColumn& owner = crustOwner(planet, position);
    return position - Vec2{double(owner.driftX), double(owner.driftZ)} * PlanetCell;
}

StrataColumn strataAt(const Planet& planet, Vec2 position) {
    const ColumnBlend around = blendColumns(planet, position);
    StrataColumn column;
    column.now = planet.time;
    column.newest = strataSlot(planet.time);
    int heaviest = 0;
    double strikeX = 0, strikeZ = 0;
    for (int corner = 0; corner < 4; ++corner) {
        if (around.weight[corner] > around.weight[heaviest]) heaviest = corner;
        // A strike has no direction, so angles are blended on the doubled circle.
        strikeX += around.weight[corner] * std::cos(2.0 * around.column[corner]->fabric);
        strikeZ += around.weight[corner] * std::sin(2.0 * around.column[corner]->fabric);
        const ColumnRecord& record = planet.records.cells[size_t(around.column[corner] - planet.columns.cells.data())];
        const double weight = around.weight[corner];
        for (int slot = 0; slot < StrataSlots; ++slot) {
            const StrataSlot& from = record.cover.slots[size_t(slot)];
            if (from.thickness <= 0) continue;
            const float added = float(from.thickness * weight);
            column.facies[size_t(slot)] = blend(column.facies[size_t(slot)], column.thickness[size_t(slot)], from.facies, added);
            column.thickness[size_t(slot)] += added;
        }
        if (record.cover.older > 0) {
            const float added = float(record.cover.older * weight);
            column.olderFacies = blend(column.olderFacies, column.older, record.cover.olderFacies, added);
            column.older += added;
        }
    }
    const CrustColumn& owner = crustOwner(planet, position);
    (void)heaviest;
    column.melt = owner.melt, column.meltTime = owner.meltTime, column.orogenTime = owner.orogenTime;
    column.deformed = std::max({0.0, double(owner.meltTime), double(owner.orogenTime)});
    const bool shield = owner.basement == Basement::Orogen && planet.time - column.deformed > ShieldAge;
    column.basement = shield ? Basement::Craton : owner.basement;
    // A cooling lid thickens with the square root of its age (conduction with latent heat).
    if (planet.time < CrustStable && owner.basement == Basement::Primordial) column.lid = 1.2f * std::sqrt(31.5f * std::max(0.0f, owner.oceanAge));
    column.fabric = float(0.5 * std::atan2(strikeZ, strikeX));
    column.material = position - Vec2{double(owner.driftX), double(owner.driftZ)} * PlanetCell;
    column.coverTotal = column.older;
    for (float thickness : column.thickness) column.coverTotal += thickness;
    restoreWindow(planet, around, column);
    // Collision belts fold their cover; quiet crust keeps it nearly flat.
    column.foldSpan = owner.basement == Basement::Orogen ? 520.0f : owner.basement == Basement::Arc ? 180.0f : 45.0f;
    return column;
}

StrataColumn blendThickness(const StrataColumn& base, const StrataColumn* const corner[4], const float weight[4]) {
    StrataColumn column = base;
    column.older = column.coverTotal = 0;
    for (int slot = 0; slot < StrataSlots; ++slot) {
        float thickness = 0;
        for (int c = 0; c < 4; ++c) thickness += weight[c] * corner[c]->thickness[size_t(slot)];
        column.thickness[size_t(slot)] = base.thickness[size_t(slot)] > 0 ? thickness : 0.0f;
        column.coverTotal += column.thickness[size_t(slot)];
    }
    for (int c = 0; c < 4; ++c) column.older += weight[c] * corner[c]->older;
    column.coverTotal += column.older;
    return column;
}

float foldAt(uint64_t seed, const StrataColumn& column, Vec2 material) {
    // Fold axes follow the strike but bend, branch and die out; several wavelengths overlap.
    const uint64_t base = stream(seed, BasementStream) + 9;
    const double bend = 0.7 * fractalNoise(base + 1, material.x / 95e3, material.z / 95e3, 2);
    const double c = std::cos(column.fabric + bend), s = std::sin(column.fabric + bend);
    const double along = (material.x * c + material.z * s) / 1000.0, across = (-material.x * s + material.z * c) / 1000.0;
    const double broad = fractalNoise(base, along / 52.0, across / 17.0, 3, 0.6);
    const double vigor = smoothstep(-0.2, 0.3, fractalNoise(base + 2, material.x / 130e3, material.z / 130e3, 2));
    return column.foldSpan * std::min(1.0f, column.coverTotal / 600.0f) * float(broad * vigor);
}

StrataUnit unitAt(const StrataColumn& column, float depth, float fold) {
    // Strata laid down after the province was last deformed share only its gentle warping. The
    // folded sequence meets them at an unconformity, which truncates whatever the folds lift into it.
    const float gentle = column.foldSpan > QuietFoldSpan ? fold * QuietFoldSpan / column.foldSpan : fold;
    float at = std::max(0.0f, depth + gentle);
    float top = 0;
    bool folded = false;
    const auto enterFolded = [&] {
        folded = true;
        at = std::max(top, depth + fold);
    };
    for (int back = 0; back < StrataSlots; ++back) {
        const int slot = (column.newest - back + StrataSlots) % StrataSlots;
        const float thickness = column.thickness[size_t(slot)];
        if (thickness <= 0) continue;
        const double laidDown = strataSlotStart(slot, column.now);
        if (!folded && laidDown < column.deformed) enterFolded();
        if (at < top + thickness) return {top + depth - at, top + thickness + depth - at, slot, column.facies[size_t(slot)], laidDown};
        top += thickness;
    }
    if (!folded) enterFolded();
    const float shift = depth - at;
    if (at < top + column.older) return {top + shift, top + column.older + shift, OlderCover, column.olderFacies, 0};
    return {top + column.older + shift, 1e9f, BasementUnit, Facies{}, 0};
}

} // namespace derivean::sim

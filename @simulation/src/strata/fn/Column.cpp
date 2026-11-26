#include "strata/type/Column.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t BasementStream = 0x42534d54;
constexpr uint64_t BedStream = 0x42454453;

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

// Strike-aligned coordinates in kilometres: `along` follows the fabric, `across` cuts it.
struct Grain {
    double along, across;
};

Grain grain(const StrataColumn& column, Vec2 material) {
    const double c = std::cos(column.fabric), s = std::sin(column.fabric);
    return {(material.x * c + material.z * s) / 1000.0, (-material.x * s + material.z * c) / 1000.0};
}

// Value of the body pattern above which a province's basement is a pluton.
constexpr double plutonLevel(Basement basement) {
    switch (basement) {
        case Basement::Arc: return -0.08;
        case Basement::Craton:
        case Basement::Orogen: return 0.10;
        default: return 2.0;
    }
}

// Intrusive bodies gather into belts: the broad term decides where a province has plutons at all.
double bodyPattern(uint64_t seed, Vec2 material) {
    const uint64_t base = stream(seed, BasementStream);
    return fractalNoise(base + 2, material.x / 31000.0, material.z / 31000.0, 3) +
           0.3 * fractalNoise(base + 3, material.x / 140e3, material.z / 140e3, 2) - 0.05;
}

Material basementRock(uint64_t seed, const StrataColumn& column, Vec2 material) {
    const uint64_t base = stream(seed, BasementStream);
    const Grain g = grain(column, material);
    const double bands = fractalNoise(base + 1, g.along / 46.0, g.across / 7.5, 3);
    const double bodies = bodyPattern(seed, material);
    if (bodies > plutonLevel(column.basement)) return Material::Granite;
    switch (column.basement) {
        case Basement::Primordial: return bodies > 0.42 ? Material::Peridotite : Material::Basalt;
        case Basement::Oceanic: return Material::Basalt;
        case Basement::Arc: return bands > 0.3 ? Material::Tuff : Material::Andesite;
        case Basement::Craton:
            if (bands > 0.40) return bands > 0.52 ? Material::IronFormation : Material::Quartzite;
            return bands < -0.34 ? Material::Schist : Material::Gneiss;
        case Basement::Orogen:
            if (bands > 0.36) return Material::Quartzite;
            if (bands < -0.30) return Material::Schist;
            return bands < -0.20 ? Material::Marble : Material::Gneiss;
    }
    return Material::Gneiss;
}

} // namespace

double plutonMargin(uint64_t seed, const StrataColumn& column, Vec2 material) {
    return bodyPattern(seed, material) - plutonLevel(column.basement);
}

float plutonRoof(uint64_t seed, const StrataColumn& column, Vec2 material) {
    constexpr double RoofDepth = 2200.0;   // m of cover a pluton keeps over its margin
    constexpr double RoofMargin = 0.10;    // pattern units from the margin to where the dome reaches the datum
    const double margin = plutonMargin(seed, column, material);
    return margin <= 0 ? 1e9f : float(RoofDepth * std::max(0.0, 1.0 - margin / RoofMargin));
}

const RockTraits& traitsOf(Material material) {
    static const auto table = [] {
        std::array<RockTraits, size_t(MaterialCount)> entries;
        for (int i = 0; i < MaterialCount; ++i) entries[size_t(i)] = traitsFrom(Material(i));
        return entries;
    }();
    return table[size_t(material)];
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
    // Kinds of crust do not blend. Their contact is read through a ragged seeded offset, so it
    // interfingers at every scale instead of following the edge of a planet cell.
    const uint64_t ragged = stream(planet.seed, BasementStream) + 31;
    const Vec2 offset{37e3 * fractalNoise(ragged, position.x / 83e3, position.z / 83e3, 5, 0.6),
                      37e3 * fractalNoise(ragged + 1, position.x / 83e3, position.z / 83e3, 5, 0.6)};
    const CrustColumn& owner = planetColumn(planet, position + offset);
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

Bed bedAt(uint64_t seed, const StrataColumn& column, const StrataUnit& layer, float depth, Vec2 material) {
    if (layer.slot == BasementUnit) {
        if (depth >= column.lid) return {std::max(layer.top, column.lid), layer.bottom, Material::Magma};
        return {layer.top, std::min(layer.bottom, column.lid), basementRock(seed, column, material)};
    }
    // A pluton cuts the strata that were there when it rose.
    const float roof = layer.laidDown < column.deformed ? plutonRoof(seed, column, material) : 1e9f;
    if (depth >= roof) return {std::max(layer.top, roof), layer.bottom, Material::Granite};
    // Beds are continuous sheets: thickness and member depend on the unit and the bed number only.
    const uint64_t base = stream(seed, BedStream);
    const int64_t epoch = int64_t(layer.laidDown / StrataSlotYears);
    const float thickness = 4.0f + 28.0f * float(unit(base, layer.slot, epoch));
    const int64_t number = int64_t(std::floor((depth - layer.top) / thickness));
    const double draw = unit(base, layer.slot, epoch, number);
    const float top = layer.top + float(number) * thickness;
    return {top, std::min({layer.bottom, top + thickness, roof}),
            layer.slot == OlderCover ? metamorphosed(layer.facies, draw) : lithified(layer.facies, draw), thickness};
}

RockTraits unitTraits(uint64_t seed, const StrataColumn& column, const StrataUnit& unit, Vec2 material) {
    if (unit.slot == BasementUnit) return traitsOf(basementRock(seed, column, material));
    if (unit.laidDown < column.deformed && plutonRoof(seed, column, material) < 0.5f * (unit.top + std::min(unit.bottom, unit.top + 4000.0f)))
        return traitsOf(Material::Granite);
    // Sample the bed sequence evenly; erodibility averages in log space so a few weak partings do
    // not turn a cliff-forming formation into mud, yet still let it retreat faster than its strongest bed.
    constexpr int Samples = 8;
    RockTraits mean{};
    double logK = 0;
    std::array<int, size_t(MaterialCount)> votes{};
    for (int i = 0; i < Samples; ++i) {
        const double bed = (i + 0.5) / Samples;
        const Material member = unit.slot == OlderCover ? metamorphosed(unit.facies, bed) : lithified(unit.facies, bed);
        const RockTraits& t = traitsOf(member);
        logK += std::log(std::max(1e-9f, t.erodibility));
        mean.stableTan += t.stableTan, mean.weathering += t.weathering, mean.solubility += t.solubility, mean.abrasion += t.abrasion;
        mean.coarse += t.coarse, mean.sand += t.sand, mean.fines += t.fines, mean.solute += t.solute;
        mean.porosity += t.porosity, mean.conductivity += t.conductivity, mean.bedding += t.bedding, mean.jointSpacing += t.jointSpacing;
        if (++votes[size_t(member)] > votes[size_t(mean.dominant)] || i == 0) mean.dominant = member;
    }
    const float scale = 1.0f / Samples;
    for (float* value : {&mean.stableTan, &mean.weathering, &mean.solubility, &mean.abrasion, &mean.coarse, &mean.sand, &mean.fines,
                         &mean.solute, &mean.porosity, &mean.conductivity, &mean.bedding, &mean.jointSpacing}) *value *= scale;
    mean.erodibility = float(std::exp(logK / Samples));
    return mean;
}

} // namespace derivean::sim

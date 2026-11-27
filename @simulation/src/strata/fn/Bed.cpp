#include "strata/type/Column.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"
#include "strata/constant/Stream.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t BedStream = 0x42454453;

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

double bodiesAt(uint64_t seed, Vec2 material, BodyMemo* memo) {
    if (!memo) return bodyPattern(seed, material);
    if (!memo->known) memo->pattern = bodyPattern(seed, material), memo->known = true;
    return memo->pattern;
}

Material basementRock(uint64_t seed, const StrataColumn& column, Vec2 material, BodyMemo* memo) {
    const double bodies = bodiesAt(seed, material, memo);
    if (bodies > plutonLevel(column.basement)) return Material::Granite;
    // The banding of metamorphic and arc basement, read only by the kinds that have it.
    const auto bandsAt = [&] {
        const Grain g = grain(column, material);
        return fractalNoise(stream(seed, BasementStream) + 1, g.along / 46.0, g.across / 7.5, 3);
    };
    switch (column.basement) {
        case Basement::Primordial: return bodies > 0.42 ? Material::Peridotite : Material::Basalt;
        case Basement::Oceanic: return Material::Basalt;
        case Basement::Arc: return bandsAt() > 0.3 ? Material::Tuff : Material::Andesite;
        case Basement::Craton: {
            const double bands = bandsAt();
            if (bands > 0.40) return bands > 0.52 ? Material::IronFormation : Material::Quartzite;
            return bands < -0.34 ? Material::Schist : Material::Gneiss;
        }
        case Basement::Orogen: {
            const double bands = bandsAt();
            if (bands > 0.36) return Material::Quartzite;
            if (bands < -0.30) return Material::Schist;
            return bands < -0.20 ? Material::Marble : Material::Gneiss;
        }
    }
    return Material::Gneiss;
}

float roofOf(uint64_t seed, const StrataColumn& column, Vec2 material, BodyMemo* memo) {
    constexpr double RoofDepth = 2200.0;   // m of cover a pluton keeps over its margin
    constexpr double RoofMargin = 0.10;    // pattern units from the margin to where the dome reaches the datum
    const double margin = bodiesAt(seed, material, memo) - plutonLevel(column.basement);
    // The roof is fixed in the crust, so it lies deeper under a datum the window has raised.
    return margin <= 0 ? 1e9f : float(RoofDepth * std::max(0.0, 1.0 - margin / RoofMargin)) + column.restored;
}

Bed bedIn(uint64_t seed, const StrataColumn& column, const StrataUnit& layer, float depth, Vec2 material, BodyMemo* memo) {
    if (layer.slot == BasementUnit) {
        if (depth >= column.lid) return {std::max(layer.top, column.lid), layer.bottom, Material::Magma};
        return {layer.top, std::min(layer.bottom, column.lid), basementRock(seed, column, material, memo)};
    }
    // A pluton cuts the strata that were there when it rose.
    const float roof = layer.laidDown < column.deformed ? roofOf(seed, column, material, memo) : 1e9f;
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

} // namespace

double plutonMargin(uint64_t seed, const StrataColumn& column, Vec2 material) {
    return bodyPattern(seed, material) - plutonLevel(column.basement);
}

float plutonRoof(uint64_t seed, const StrataColumn& column, Vec2 material) {
    return roofOf(seed, column, material, nullptr);
}

Bed bedAt(uint64_t seed, const StrataColumn& column, const StrataUnit& layer, float depth, Vec2 material) {
    return bedIn(seed, column, layer, depth, material, nullptr);
}

Bed bedAt(uint64_t seed, const StrataColumn& column, const StrataUnit& layer, float depth, Vec2 material, BodyMemo& bodies) {
    return bedIn(seed, column, layer, depth, material, &bodies);
}

RockTraits unitTraits(uint64_t seed, const StrataColumn& column, const StrataUnit& unit, Vec2 material) {
    if (unit.slot == BasementUnit) return traitsOf(basementRock(seed, column, material, nullptr));
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

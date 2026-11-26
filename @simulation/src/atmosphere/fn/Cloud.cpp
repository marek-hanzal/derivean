#include "atmosphere/type/Cloud.hpp"

#include "core/type/Vec.hpp"

namespace derivean::sim {
namespace {

constexpr double LiquidOptics = 150.0;   // optical depth per kg/m² of droplets of 10 µm
constexpr double IceOptics = 55.0;       // per kg/m² of crystals of 30 µm
constexpr double HalfShade = 8.0;        // optical depth that turns half the sunlight away
constexpr double CoverPath[AirBands] = {0.12, 0.08, 0.04};    // kg/m² that covers 63 % of the sky
constexpr double Content[AirBands] = {0.35e-3, 0.2e-3, 0.03e-3};   // kg/m³ inside a cloud of the band
constexpr double ThinnestCloud[AirBands] = {100.0, 200.0, 300.0};  // m
constexpr double FogLevel = 60.0;        // m of condensation level below which the cloud sits on the ground
constexpr double SteadyRain = 0.5 / 3600.0;   // kg/m²/s that marks a rain-bearing layer cloud
constexpr double ThinLayer = 0.15;       // kg/m² in the middle band below which a layer is too thin to be one
constexpr double BroadLift = 400.0;      // m of lifting that smooths a layer into a sheet
constexpr double LidLapse = 1.5e-3;      // °C/m short of the moist lapse at which the air above closes into a lid
constexpr double LidSinking = 400.0;     // m of sinking that does the same

double opticalDepthOf(const AirColumn& column, size_t band) { return LiquidOptics * column.liquid[band] + IceOptics * column.ice[band]; }

} // namespace

const char* genusName(Genus genus) {
    static constexpr const char* names[Genera] = {"cumulus", "cumulonimbus", "stratus", "stratocumulus", "nimbostratus",
                                                  "altocumulus", "altostratus", "cirrus", "cirrocumulus", "cirrostratus"};
    return names[size_t(genus)];
}

double cloudShade(const AirColumn& column) {
    const double depth = opticalDepthOf(column, Low) + opticalDepthOf(column, Middle) + opticalDepthOf(column, High);
    return depth / (depth + HalfShade);
}

CloudReport describeClouds(const AirColumn& column, const ColumnProfile& profile, double precipitation) {
    CloudReport report;
    const double lifts[AirBands] = {bandLift(column, Low), bandLift(column, Middle), bandLift(column, High)};
    double clear = 1.0;
    for (size_t band = 0; band < AirBands; ++band) {
        CloudLayer& layer = report.layer[band];
        layer.liquidPath = column.liquid[band];
        layer.icePath = column.ice[band];
        const double path = layer.liquidPath + layer.icePath;
        layer.cover = 1.0 - std::exp(-path / CoverPath[band]);
        layer.opticalDepth = opticalDepthOf(column, band);
        report.opticalDepth += layer.opticalDepth;
        clear *= 1.0 - layer.cover;

        // A layer starts where its band is moist enough: at the condensation level of surface air
        // in the low band, and the higher in the bands above the drier they are.
        const double floor = profile.height[band], depth = profile.height[band + 1] - floor;
        const double dryness = 1.0 - std::min(1.0, column.vapour[band] * profile.perCapacity[band]);
        const double start = band == Low ? (profile.condensationLevel < FogLevel ? 0.0 : std::min(profile.condensationLevel, LowDepth - ThinnestCloud[Low]))
                                         : floor + depth * std::min(0.6, dryness);
        const double thickness = std::clamp(path / Content[band], ThinnestCloud[band], floor + depth - start);
        layer.base = column.ground + start;
        layer.top = layer.base + thickness;
    }
    report.cover = 1.0 - clear;
    report.shade = report.opticalDepth / (report.opticalDepth + HalfShade);

    // Updrafts stir the low band. Where they are free to grow they build heaped cloud from the
    // condensation level up to where the parcel stops; under a lid of stable or sinking air they
    // spread into a rolled layer instead.
    const double stirred = profile.release;
    const double reach = profile.reach;                    // into the middle band
    const double tower = profile.reach * profile.tower;    // through the high band, where its top is ice
    const double open = saturate((profile.lapse - profile.moistLapse) / LidLapse + 1.0 + std::min(0.0, column.lift) / LidSinking);
    const double heaped = stirred * std::max(reach, open);
    const double shallowTop = profile.lapse < profile.moistLapse && profile.baseBuoyancy > 0
                                  ? profile.condensationLevel + profile.baseBuoyancy / (profile.moistLapse - profile.lapse)
                                  : profile.condensationLevel + 300.0 + 1200.0 * open;
    const double updraftTop = mix(mix(std::min(shallowTop, LowDepth), profile.height[2], reach), profile.height[3], tower);
    report.convectiveTop = column.ground + (heaped > 0 ? updraftTop : 0.0);

    auto& share = report.share;
    const CloudLayer& low = report.layer[Low];
    const CloudLayer& middle = report.layer[Middle];
    const CloudLayer& high = report.layer[High];
    const double storm = tower * saturate(high.icePath / 0.01 + middle.cover);
    share[size_t(Genus::Cumulonimbus)] = std::max(low.cover, middle.cover) * heaped * storm;
    share[size_t(Genus::Cumulus)] = low.cover * heaped * (1.0 - storm);

    // Layer cloud. A thick rain-bearing layer through the low and middle band is nimbostratus; a
    // low layer is stratocumulus where updrafts stir it under a lid and stratus in still air.
    const double layered = 1.0 - heaped;
    const double raining = saturate(precipitation / SteadyRain) * saturate((middle.liquidPath + middle.icePath - ThinLayer) / ThinLayer) *
                           saturate(2.0 * low.cover) * layered;
    share[size_t(Genus::Nimbostratus)] = std::max(low.cover, middle.cover) * raining;
    share[size_t(Genus::Stratocumulus)] = low.cover * (1.0 - raining) * (stirred - heaped);
    share[size_t(Genus::Stratus)] = low.cover * (1.0 - raining) * (1.0 - stirred);

    // Middle band: a sheet where the air was lifted as a whole and holds ice, cells where it was not.
    const double middleOwn = middle.cover * (1.0 - raining) * (1.0 - heaped * reach);
    const double middlePath = middle.liquidPath + middle.icePath;
    const double middleIce = middlePath > 0 ? middle.icePath / middlePath : 0.0;
    const double middleSheet = saturate(3.0 * (middle.cover - 0.45)) * saturate(0.4 + 0.6 * middleIce + lifts[Middle] / BroadLift);
    share[size_t(Genus::Altostratus)] = middleOwn * middleSheet;
    share[size_t(Genus::Altocumulus)] = middleOwn * (1.0 - middleSheet);

    // High band: the anvil belongs to its storm; otherwise a veil where lifted as a whole, ripples
    // where supercooled droplets still form, and streaks of falling crystals in between.
    const double highOwn = high.cover * (1.0 - heaped * storm);
    const double highPath = high.liquidPath + high.icePath;
    const double droplets = highPath > 0 ? saturate(3.0 * high.liquidPath / highPath) : 0.0;
    const double highSheet = saturate(3.0 * (high.cover - 0.5)) * saturate(0.5 + lifts[High] / BroadLift) * (1.0 - droplets);
    share[size_t(Genus::Cirrostratus)] = highOwn * highSheet;
    share[size_t(Genus::Cirrocumulus)] = highOwn * (1.0 - highSheet) * droplets;
    share[size_t(Genus::Cirrus)] = highOwn * (1.0 - highSheet) * (1.0 - droplets);

    const auto leading = [&](std::initializer_list<Genus> candidates) {
        Genus best = *candidates.begin();
        for (Genus genus : candidates)
            if (share[size_t(genus)] > share[size_t(best)]) best = genus;
        return best;
    };
    report.layer[Low].genus = leading({Genus::Stratus, Genus::Stratocumulus, Genus::Cumulus, Genus::Cumulonimbus, Genus::Nimbostratus});
    report.layer[Middle].genus = leading({Genus::Altocumulus, Genus::Altostratus, Genus::Nimbostratus, Genus::Cumulonimbus});
    report.layer[High].genus = leading({Genus::Cirrus, Genus::Cirrocumulus, Genus::Cirrostratus, Genus::Cumulonimbus});
    report.genus = leading({Genus::Cumulus, Genus::Cumulonimbus, Genus::Stratus, Genus::Stratocumulus, Genus::Nimbostratus,
                            Genus::Altocumulus, Genus::Altostratus, Genus::Cirrus, Genus::Cirrocumulus, Genus::Cirrostratus});

    // Heaped cloud stands on the condensation level and ends where its updraft does; a rain layer
    // reaches down to the low cloud under it.
    if (heaped > 0.5 && low.cover > 0) {
        report.layer[Low].base = column.ground + std::min(profile.condensationLevel, LowDepth - ThinnestCloud[Low]);
        report.layer[Low].top = std::max(report.layer[Low].base + ThinnestCloud[Low], std::min(report.convectiveTop, column.ground + profile.height[1]));
        if (reach > 0.5) report.layer[Middle].base = column.ground + profile.height[1];
        if (reach > 0.5) report.layer[Middle].top = std::min(report.convectiveTop, column.ground + profile.height[2]);
        if (tower > 0.5) report.layer[High].base = column.ground + profile.height[2];
        if (tower > 0.5) report.layer[High].top = report.convectiveTop;
    }
    if (raining > 0.5) report.layer[Middle].base = column.ground + profile.height[1];
    return report;
}

} // namespace derivean::sim

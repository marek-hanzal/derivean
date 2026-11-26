#include "atmosphere/type/Column.hpp"

#include "climate/fn/Moisture.hpp"
#include "climate/type/Climate.hpp"

namespace derivean::sim {
namespace {

constexpr double SlackLapse = 2.5e-3, SteepLapse = 9.5e-3;   // °C/m the profile is kept between
constexpr double DewSpread = 2500.0;        // m of condensation level per unit of missing relative humidity
constexpr double SurfaceDrying = 0.065;     // share of its relative humidity air loses per °C it is warmed
constexpr double Inhibition = 1.0;          // °C of negative buoyancy at cloud base that still lets updrafts through
constexpr double FullBuoyancy = 2.0;        // °C at which an updraft keeps all of its air
constexpr double Entrainment = 3.0;         // °C an updraft loses to perfectly dry air around it
constexpr double FogSqueeze = 0.01;         // share of the low band's capacity lost per °C of night cooling
constexpr double HomogeneousFreezing = -38.0;   // °C below which fresh cloud is ice at once
constexpr double ThinLiquid = 0.2;          // kg/m² below which liquid cloud rains more slowly the thinner it is
constexpr double MixedLayerLapse = 1.0e-3;  // °C/m by which the stirred air under a cloud base falls short of the dry lapse
constexpr double ShallowYield = 0.1;        // share of a shallow updraft's condensate that outlives its mixing with dry air
constexpr double Trace = 1e-9;              // kg/m² below which a store is emptied rather than thinned further
constexpr double ThinAir = 0.5;             // share of the vapour exchange that reaches the high band, where the air is thin
constexpr double DescentShare[AirBands] = {0.0, 0.6, 0.7};   // air mass of the low band over that of the band above

// Share of fresh cloud in a band that forms as droplets: the part of the band's vapour that lies
// in air too warm to freeze at once. The rest is ice from the start.
double dropletShare(const ColumnProfile& profile, int band) {
    constexpr double FreezingDensity = saturationDensity(HomogeneousFreezing);
    const double bottom = profile.density[size_t(band)], top = profile.density[size_t(band) + 1];
    return saturate((bottom - std::max(top, FreezingDensity)) / std::max(bottom - top, 1e-12));
}

// °C/m a saturated parcel cools by: latent heat holds it back more the warmer it is.
double moistLapseAt(double celsius) { return std::clamp(6.4e-3 - 8.5e-5 * celsius, 3.5e-3, 9.6e-3); }

} // namespace

double pressureAt(double height) { return 101325.0 * std::exp(-height / 7800.0); }

ColumnProfile profileOf(const AirColumn& column) {
    ColumnProfile profile;
    profile.lapse = std::clamp((column.airTemperature - column.upperTemperature) / UpperLevel, SlackLapse, SteepLapse);
    const double perLapse = 1.0 / profile.lapse;
    // The moisture budget sees the ground no higher than the terrain could lift the air.
    const double floor = column.airTemperature - LapseRate * column.climb;
    profile.height[1] = LowDepth;
    profile.height[2] = std::max(LowDepth + ThinnestBand, (floor - IceCelsius) * perLapse);
    profile.height[3] = std::max(profile.height[2] + ThinnestBand, (floor - TropopauseCelsius) * perLapse);
    const auto& density = profile.density;
    for (int level = 0; level <= AirBands; ++level) {
        profile.celsius[size_t(level)] = floor - profile.lapse * profile.height[size_t(level)];
        profile.density[size_t(level)] = saturationDensity(profile.celsius[size_t(level)]);
    }
    for (int band = 0; band < AirBands; ++band) {
        // Saturation density falls off with VapourDepth, so a band holds the difference of its ends.
        profile.capacity[size_t(band)] = VapourDepth * std::max(density[size_t(band)] - density[size_t(band) + 1], 1e-9);
        profile.perCapacity[size_t(band)] = 1.0 / profile.capacity[size_t(band)];
    }

    // A parcel of surface air: dry to its condensation level, saturated above, thinned by the
    // drier air it mixes with on the way.
    // Air warmed at the ground is relatively drier and condenses higher up; air cooled there may
    // be saturated where it stands.
    const double humidity = std::min(1.0, column.vapour[Low] * profile.perCapacity[Low] * std::max(0.2, 1.0 - SurfaceDrying * column.surfaceWarmth));
    profile.condensationLevel = DewSpread * (1.0 - humidity);
    profile.cloudBaseCelsius = floor + column.surfaceWarmth - DryLapse * profile.condensationLevel;
    profile.moistLapse = moistLapseAt(profile.cloudBaseCelsius);
    profile.baseBuoyancy = column.surfaceWarmth - MixedLayerLapse * profile.condensationLevel;
    const double steepening = profile.lapse - profile.moistLapse;
    const double dryness = Entrainment * (1.0 - std::min(1.0, column.vapour[Middle] * profile.perCapacity[Middle]));
    const double middle = 0.5 * (profile.height[1] + profile.height[2]), high = 0.5 * (profile.height[2] + profile.height[3]);
    profile.middleBuoyancy = profile.baseBuoyancy + steepening * (middle - profile.condensationLevel) - dryness;
    profile.highBuoyancy = profile.baseBuoyancy + steepening * (high - profile.condensationLevel) - dryness;
    profile.release = saturate(profile.baseBuoyancy / Inhibition + 1.0);
    profile.reach = saturate(profile.middleBuoyancy / FullBuoyancy);
    profile.tower = saturate(profile.highBuoyancy / FullBuoyancy);
    profile.convection = profile.release * (1.0 + profile.reach + profile.reach * profile.tower) / 3.0;
    return profile;
}

ColumnRates columnRates(double seconds) {
    const auto share = [&](double time) { return 1.0 - std::exp(-seconds / time); };
    ColumnRates rates;
    rates.seconds = seconds;
    rates.cloudDrying = share(2 * 3600.0);    // liquid cloud in air below its condensation onset
    rates.iceDrying = share(9 * 3600.0);      // ice cloud, which sublimates slowly
    rates.virga = share(2 * 3600.0);          // precipitation falling through such air
    rates.rain = share(1.5 * 3600.0);         // thick liquid cloud raining out
    rates.snow = share(2 * 3600.0);           // ice crystals growing and falling out
    rates.riming = share(1 * 3600.0);         // ice growing at the cost of liquid where both exist
    rates.glaciation = share(1.5 * 3600.0);   // supercooled liquid freezing, at its fastest
    rates.snowFall = share(0.75 * 3600.0);    // snow leaving a band; rain leaves within the step
    rates.exchange = seconds / (LiftTime * 3000.0);
    rates.convective = share(6 * 3600.0);
    return rates;
}

ColumnYield stepColumn(AirColumn& column, const ColumnProfile& profile, const ColumnRates& rates) {
    ColumnYield yield;
    Bands& vapour = column.vapour;
    const double lifts[AirBands] = {bandLift(column, Low), bandLift(column, Middle), bandLift(column, High)};

    // Rising air carries vapour into the band above, sinking air brings the drier air down.
    for (int band = 0; band < AirBands - 1; ++band) {
        const double rise = 0.5 * (lifts[band] + lifts[band + 1]);
        const int from = rise > 0 ? band : band + 1;
        const double carried = vapour[size_t(from)] * std::min(0.5, std::abs(rise) * rates.exchange * (band == Low ? 1.0 : ThinAir));
        vapour[size_t(from)] -= carried;
        vapour[size_t(band + (rise > 0 ? 1 : 0))] += carried;
    }

    // Updrafts lift low air until it stops being buoyant. On the way it sheds as cloud what it can
    // no longer hold, band by band; what is left of its vapour stays where it stops. Deep updrafts
    // rain out part of their cloud at once, and as much drier air sinks back as went up.
    if (profile.convection > 0) {
        const double release = profile.release, reach = profile.reach, tower = profile.tower;
        const double updraft = release * rates.convective * vapour[Low];
        // Share of its vapour the updraft still holds at the top of each band.
        const double perBase = 1.0 / saturationDensity(profile.cloudBaseCelsius);
        const double kept[AirBands + 1] = {1.0, std::min(1.0, profile.density[1] * perBase), std::min(1.0, profile.density[2] * perBase),
                                           std::min(1.0, profile.density[3] * perBase)};
        const double passes[AirBands] = {updraft, updraft * reach, updraft * reach * tower};
        vapour[Low] -= updraft;
        for (int band = 0; band < AirBands; ++band) {
            const size_t at = size_t(band);
            const double formed = passes[band] * std::max(0.0, kept[band] - kept[band + 1]);
            // Shallow cloud mixes with the dry air around it and mostly evaporates again.
            const double condensed = band == Low ? formed * ShallowYield : formed;
            const double shed = band == Low ? 0.0 : condensed * (0.3 + 0.4 * tower);
            const double droplets = (condensed - shed) * dropletShare(profile, band);
            const double stops = passes[band] - (band + 1 < AirBands ? passes[band + 1] : 0.0);
            column.liquid[at] += droplets;
            column.ice[at] += condensed - shed - droplets;
            column.falling[at] += shed;
            vapour[at] += stops * std::min(kept[band], kept[band + 1]) + (formed - condensed);
            if (band > Low) yield.condensed += condensed;
        }
        const double middleDown = vapour[Middle] * std::min(0.5, release * reach * rates.convective * DescentShare[Middle]);
        const double highDown = vapour[High] * std::min(0.5, release * reach * tower * rates.convective * DescentShare[High]);
        vapour[Low] += middleDown;
        vapour[Middle] += highDown - middleDown;
        vapour[High] -= highDown;
    }

    // Band by band from the top, so that what falls out of one band passes through the next.
    double arriving = 0;
    for (int band = AirBands - 1; band >= 0; --band) {
        const size_t at = size_t(band);
        const double celsius = 0.5 * (profile.celsius[at] + profile.celsius[at + 1]);
        double liquid = column.liquid[at], ice = column.ice[at];
        // Night air cooling from the ground squeezes the low band, most where cold air pools.
        const double night = band == Low && column.surfaceWarmth < 0 ? 1.0 + FogSqueeze * column.surfaceWarmth * (1.0 + 2.0 * column.hollow) : 1.0;
        const double hold = profile.capacity[at] * liftedCapacity(lifts[band]) * std::max(0.5, night);
        const double excess = vapour[at] - CondensationOnset * hold;
        if (excess <= 0 && liquid == 0 && ice == 0 && arriving == 0 && column.falling[at] == 0) continue;   // clear, dry and nothing falling through
        if (excess > 0) {
            const double condensed = std::min(excess, condensationOfExcess(excess, 1.0 / hold) * rates.seconds);
            const double droplets = condensed * dropletShare(profile, band);
            vapour[at] -= condensed;
            liquid += droplets;
            ice += condensed - droplets;
            if (band > Low) yield.condensed += condensed;
        } else {
            double room = -excess;
            const double dried = std::min(liquid * rates.cloudDrying, room);
            const double sublimed = std::min(ice * rates.iceDrying, room - dried);
            const double lost = std::min(arriving * rates.virga, room - dried - sublimed);
            liquid -= dried;
            ice -= sublimed;
            arriving -= lost;
            vapour[at] += dried + sublimed + lost;
        }
        if (celsius < -2.0) {
            const double frozen = liquid * rates.glaciation * saturate((-2.0 - celsius) * (1.0 / 30.0));
            liquid -= frozen;
            ice += frozen;
        } else if (celsius > 0.0) {
            const double melted = ice * saturate(0.5 * celsius);
            ice -= melted;
            liquid += melted;
        }
        // A trace falls out whole: stores that only ever shrank by a share would never reach zero.
        const double rained = liquid < Trace ? liquid : liquid * std::min(1.0, liquid * (1.0 / ThinLiquid)) * rates.rain;
        const double snowed = ice < Trace ? ice : ice * rates.snow;
        const double rimed = std::min(liquid - rained, ice - snowed) * rates.riming;
        column.liquid[at] = liquid - rained - rimed;
        column.ice[at] = ice - snowed;
        const double inFlight = column.falling[at] + arriving + rained + snowed + rimed;
        arriving = inFlight * (celsius > 0.0 || inFlight < Trace ? 1.0 : rates.snowFall);
        column.falling[at] = inFlight - arriving;
    }
    yield.precipitation = arriving;
    return yield;
}

} // namespace derivean::sim

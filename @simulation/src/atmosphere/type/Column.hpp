#pragma once

#include <array>

namespace derivean::sim {

// The air above a place is kept in three bands that follow the ground: the low band holds the
// first two kilometres, the high band starts where the column is cold enough for ice cloud, and
// the middle band is what lies between. Their heights come from the column's own temperatures.
inline constexpr int AirBands = 3;
enum Band { Low, Middle, High };
using Bands = std::array<double, AirBands>;

inline constexpr double UpperLevel = 5000.0;      // m above the sea at which the upper temperature is kept
inline constexpr double LowDepth = 2000.0;        // m
inline constexpr double ThinnestBand = 1500.0;    // m the middle and high band keep at least
inline constexpr double IceCelsius = -25.0;       // °C at the base of the high band
inline constexpr double TropopauseCelsius = -58.0;
inline constexpr double DryLapse = 9.8e-3;        // °C/m an unsaturated parcel cools by as it rises

// One column of air: what drives it and the water it holds. The atmosphere steps one of these per
// cell; a hand-built one behaves the same, which is how single cloud types are tested.
struct AirColumn {
    double airTemperature{};     // °C near the surface as a daily mean, referenced to sea level
    double upperTemperature{};   // °C at UpperLevel
    double surfaceWarmth{};      // °C the air at the ground is warmer than that mean right now (sun, warm water); negative at night
    double ground{};             // m above the sea
    double climb{};              // m the moist layer was lifted by the terrain (terrainLift of the ground)
    double hollow{};             // 0 to 1: how far the ground lies below its surroundings, where cold air pools
    double lift{};               // m of lifting by friction under cyclonic flow and by the overturning cells
    double glide{};              // m of lifting by warm air gliding up over cold
    Bands vapour{}, liquid{}, ice{};   // kg/m²
    Bands falling{};                    // kg/m² of rain and snow still in the air of each band
};

// The vertical structure the column's temperatures imply.
struct ColumnProfile {
    double lapse{};                                // °C/m between the surface and UpperLevel
    std::array<double, AirBands + 1> height{};     // m above the ground: band interfaces, the last is the tropopause
    std::array<double, AirBands + 1> celsius{};    // °C at the interfaces
    std::array<double, AirBands + 1> density{};    // kg/m³ of vapour in saturated air at the interfaces
    Bands capacity{};                              // kg/m² of vapour each band holds when saturated
    Bands perCapacity{};
    double condensationLevel{};   // m above the ground at which surface air lifted dry becomes saturated
    double cloudBaseCelsius{};    // °C of that air there
    double moistLapse{};          // °C/m a saturated parcel cools by above it
    double baseBuoyancy{};        // °C a surface parcel is warmer than its surroundings at the condensation level
    double middleBuoyancy{};      // °C in the middle of the middle band
    double highBuoyancy{};        // °C in the middle of the high band
    double release{};             // 0 to 1: share of surface parcels that get past the cloud base
    double reach{};               // 0 to 1: share of those that stay buoyant into the middle band
    double tower{};               // 0 to 1: share of those that stay buoyant through the high band
    double convection{};          // release · (1 + reach + reach · tower) / 3: one figure for how far updrafts develop
};

ColumnProfile profileOf(const AirColumn& column);

// m by which the air of a band is being lifted: friction and overturning act from below, warm air
// gliding up over cold acts from above.
inline double bandLift(const AirColumn& column, int band) {
    return band == Low ? column.lift + 0.3 * column.glide : band == Middle ? 0.7 * column.lift + column.glide : 0.3 * column.lift + column.glide;
}

// Pa at a height above the sea in a standard atmosphere.
double pressureAt(double height);

// Shares of a store that each process moves in one step of the given length.
struct ColumnRates {
    double seconds{};
    double cloudDrying{}, iceDrying{}, virga{};
    double rain{}, snow{}, riming{}, glaciation{}, snowFall{};
    double exchange{};     // per metre of lift: share of a band's vapour carried into the next band
    double convective{};   // share of the low band's vapour a fully active updraft takes
};

ColumnRates columnRates(double seconds);

struct ColumnYield {
    double precipitation{};   // kg/m² that reached the ground
    double condensed{};       // kg/m² condensed above the low band, which warms the upper air
};

// One step of the column's water: vapour is carried up and down, condenses where a band holds
// more than it can, freezes, melts, falls and evaporates on the way. What leaves the column is
// returned; everything else stays in its stores, so vapour + liquid + ice + falling changes by
// exactly −precipitation.
ColumnYield stepColumn(AirColumn& column, const ColumnProfile& profile, const ColumnRates& rates);

} // namespace derivean::sim

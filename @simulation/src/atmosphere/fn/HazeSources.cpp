#include "atmosphere/fn/Kernels.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Regime.hpp"
#include "climate/constant/Moisture.hpp"
#include "climate/type/Climate.hpp"
#include "planet/type/Planet.hpp"

namespace derivean::sim {
namespace {

// The low band's wind is the wind ten metres up (aeolian/constant/Wind.hpp). Gusts the grid does not
// resolve blow around it with Gustiness in each direction, taken over the same quadrature as a sand rose.
constexpr int Reach = 6;

// Wind lifts dust off dry, bare, loose ground in proportion to the sand it moves: the aeolian
// stream's threshold and flux (saturatedFlux) and its ratio of fines to drift. Only the fine class
// stays up longer than it takes to cross a weather cell. A wet surface binds the grains: the
// threshold rises by Fécan et al. (1999) with the gravimetric moisture of a sand, whose field
// capacity the soil store's capacity stands for.
constexpr double SandFieldCapacity = 10.0;   // % water by mass
constexpr double FinesPerDrift = double(DustRise) / double(DustDrift);   // 1/m

// Sea spray: Gong (2003) integrated over wet radii up to 4 µm at 80 % humidity (dry radius 2 µm),
// the part that lives long enough to haze the air, as dry salt of 2160 kg/m³.
constexpr double SaltSpray = 8.36e-14;       // kg/m²/s per (m/s)^3.41
constexpr double SprayPower = 3.41;
constexpr double FrozenSea = -1.8;           // °C: below it the sea is ice and gives neither spray nor gas

// Dimethyl sulphide from the sea's plankton crosses into the air at the Nightingale et al. (2000)
// transfer velocity, scaled to its Schmidt number in sea water (Saltzman et al. 1993). The sea's
// concentration is Earth's climatological mean (Lana et al. 2011, 2.3 nM) until the sea has life
// of its own. Some 0.4 of its sulphur ends as sulphate aerosol: about three quarters oxidises to
// SO₂, a little over half of which becomes sulphate before it is deposited (Chin et al. 1996). It
// is counted as ammonium sulphate.
constexpr double SeaDms = 2.3e-6;            // mol/m³
constexpr double DmsSulphate = 0.4;          // share of the sulphur that ends as aerosol
constexpr double SulphateMolar = 0.13214;    // kg of ammonium sulphate per mol of sulphur

// Plants give off isoprene by day and monoterpenes always (Guenther et al. 1993, 1995); canopy
// emission factors at 30 °C and full light of a mixed cover (MEGAN, Guenther et al. 2006). Part of
// both condenses as secondary organic aerosol: isoprene about 3 % (Kroll et al. 2006), monoterpenes
// about 10 % (Griffin et al. 1999; Hallquist et al. 2009).
constexpr double IsopreneFactor = 5.0e-6 / 3600.0;   // kg/m²/s of plant cover
constexpr double TerpeneFactor = 1.0e-6 / 3600.0;
constexpr double IsopreneYield = 0.03;
constexpr double TerpeneYield = 0.10;
constexpr double TerpeneWarming = 0.09;               // 1/K
constexpr double FullSunlight = 2000.0;               // µmol/m²/s of photosynthetic light under a clear overhead sun

// Mean over the gusts around a wind of `speed` of a function of the speed.
template <typename Of> double gusty(double speed, double spread, Of of) {
    double total = 0, all = 0;
    for (int b = -Reach; b <= Reach; ++b)
        for (int a = -Reach; a <= Reach; ++a) {
            const double share = std::exp(-0.125 * (a * a + b * b));
            all += share;
            total += share * of(std::hypot(speed + 0.5 * spread * a, 0.5 * spread * b));
        }
    return total / all;
}

// A gust mean read linearly between speeds TableStep apart.
constexpr double TableStep = 0.25;   // m/s
constexpr int TableSize = 201;
struct GustTable {
    std::array<double, TableSize> value{};
    double at(double speed) const {
        const double place = std::clamp(speed / TableStep, 0.0, TableSize - 1.001);
        const int low = int(place);
        return mix(value[size_t(low)], value[size_t(low) + 1], place - low);
    }
};

template <typename Of> GustTable tabulate(double spread, Of of) {
    GustTable table;
    for (int k = 0; k < TableSize; ++k) table.value[size_t(k)] = gusty(k * TableStep, spread, of);
    return table;
}

// Damp sand: the threshold rises by `binding`, which for the flux of Lettau and Lettau is the
// dry flux at speeds divided by it, times its cube. Tabulated for bindings from 1 to MostBinding.
constexpr int Bindings = 31;
constexpr double MostBinding = 4.0;
const std::array<GustTable, Bindings>& sandTables() {
    static const std::array<GustTable, Bindings> tables = [] {
        std::array<GustTable, Bindings> made;
        for (int k = 0; k < Bindings; ++k) {
            const double binding = 1.0 + (MostBinding - 1.0) * k / (Bindings - 1);
            made[size_t(k)] = tabulate(Gustiness, [binding](double speed) { return binding * binding * binding * saturatedFlux(speed / binding); });
        }
        return made;
    }();
    return tables;
}

double sandFlux(double speed, double binding) {   // m²/yr per m of width
    if (binding >= MostBinding) return 0;
    const double place = (binding - 1.0) / (MostBinding - 1.0) * (Bindings - 1);
    const int low = std::min(int(place), Bindings - 2);
    return mix(sandTables()[size_t(low)].at(speed), sandTables()[size_t(low) + 1].at(speed), place - low);
}

// Fécan et al. (1999) for a sand without clay: the threshold grows with the water the grains hold.
double bindingOf(double wetness) {
    const double moisture = SandFieldCapacity * saturate(wetness);
    return std::sqrt(1.0 + 1.21 * std::pow(moisture, 0.68));
}

// m/s of the Nightingale transfer velocity of DMS through the sea surface.
double dmsTransfer(double speed, double celsius) {
    static const GustTable spread = tabulate(Gustiness, [](double s) { return 0.222 * s * s + 0.333 * s; });   // cm/h at Schmidt number 600
    const double t = std::clamp(celsius, -2.0, 32.0);
    const double schmidt = 2674.0 - 147.12 * t + 3.726 * t * t - 0.038 * t * t * t;
    return spread.at(speed) * std::sqrt(600.0 / schmidt) / 360000.0;
}

// Guenther et al. (1993): isoprene against leaf temperature and photosynthetic light.
double isopreneResponse(double kelvin, double light) {
    constexpr double R = 8.314, Standard = 303.0, Optimum = 314.0;
    const double warmth = std::exp(95000.0 * (kelvin - Standard) / (R * Standard * kelvin)) / (1.0 + std::exp(230000.0 * (kelvin - Optimum) / (R * Standard * kelvin)));
    const double lit = 0.0027 * 1.066 * light / std::sqrt(1.0 + 0.0027 * 0.0027 * light * light);
    return warmth * lit;
}

} // namespace

void shapeHaze(Atmosphere& atmosphere, const Planet& planet) {
    Haze& haze = atmosphere.haze;
    const Field<double> blank(AirCells, AirCells);
    for (Field<double>& field : haze.mass) field = blank;
    haze.plants = haze.depth = blank;
    for (Field<double>& field : haze.spectrum) field = Field<double>(AirCells, AirCells, 1.0);
    for (Field<double>& field : haze.albedo) field = Field<double>(AirCells, AirCells, 1.0);
    haze.work.assign(2 * HazeKinds, blank);
    // Plants hold the ground as far as the rain keeps up with the air: the aeolian stream's cover,
    // over the land of the cell. The planet's fields have to fit the weather grid as its heights do
    // for setSurface; a planet without a climate leaves the land bare.
    const int perCell = planet.elevation.width / AirCells;
    const bool fits = perCell >= 1 && planet.elevation.width == perCell * AirCells && planet.elevation.height == planet.elevation.width;
    for (const Field<float>* field : {&planet.precipitation, &planet.temperature})
        if (!fits || field->width != planet.elevation.width || field->height != planet.elevation.height) return;
    for (int z = 0; z < AirCells; ++z)
        for (int x = 0; x < AirCells; ++x) {
            double green = 0;
            int land = 0;
            for (int dz = 0; dz < perCell; ++dz)
                for (int dx = 0; dx < perCell; ++dx) {
                    const int px = x * perCell + dx, pz = z * perCell + dz;
                    if (planet.elevation.at(px, pz) <= planet.seaLevel) continue;
                    green += 1.0 - aridityOf(planet.precipitation.at(px, pz), planet.temperature.at(px, pz));
                    ++land;
                }
            green = land > 0 ? green / land : 0.0;
            haze.plants.at(x, z) = green * green;
        }
}

void raiseHaze(Atmosphere& atmosphere, int begin, int end, std::array<double, HazeKinds>* rows) {
    Haze& haze = atmosphere.haze;
    const Calendar now{atmosphere.era, (double(atmosphere.step) + 0.5) * AirStep};
    for (int z = begin; z < end; ++z) {
        const double sunUp = sunDirectionAt({0.0, (z + 0.5) * AirCell}, now).up;   // one hour for the whole flat world
        for (int x = 0; x < AirCells; ++x) {
            const size_t cell = atmosphere.ground.index(x, z);
            const double water = atmosphere.openWater.cells[cell], land = 1.0 - water;
            const double speed = std::hypot(atmosphere.windX.cells[cell], atmosphere.windZ.cells[cell]);
            const double sea = atmosphere.surfaceTemperature.cells[cell];
            std::array<double, HazeKinds> rate{};   // kg/m²/s of cell area
            if (water > 0 && sea > FrozenSea) {
                static const GustTable spray = tabulate(Gustiness, [](double s) { return std::pow(s, SprayPower); });
                rate[SeaSalt] = water * SaltSpray * spray.at(speed);
                rate[Sulphate] = water * dmsTransfer(speed, sea) * SeaDms * DmsSulphate * SulphateMolar;
            }
            if (land > 0) {
                const double open = land - std::min(land, atmosphere.snowCover.cells[cell]);
                const double plants = haze.plants.cells[cell];
                const double wetness = atmosphere.soil.cells[cell] / (SoilCapacity * land);
                const double drift = sandFlux(speed, bindingOf(wetness));
                rate[MineralDust] = open * (1.0 - plants) * DustShare[1] * FinesPerDrift * drift * SandBulkDensity / YearSeconds;
                const AirColumn column = columnAt(atmosphere, x, z);
                const double kelvin = 273.15 + column.airTemperature - LapseRate * column.ground + column.surfaceWarmth;
                const double light = FullSunlight * std::max(0.0, sunUp) * (1.0 - atmosphere.shade.cells[cell]);
                rate[Organic] = open * plants *
                                (IsopreneYield * IsopreneFactor * isopreneResponse(kelvin, light) +
                                 TerpeneYield * TerpeneFactor * std::exp(TerpeneWarming * (kelvin - 303.0)));
            }
            for (int kind = 0; kind < HazeKinds; ++kind) {
                haze.mass[size_t(kind)].cells[cell] += rate[size_t(kind)] * AirStep;
                rows[z][size_t(kind)] += rate[size_t(kind)] * AirStep * AirCellArea;
            }
        }
    }
}

} // namespace derivean::sim

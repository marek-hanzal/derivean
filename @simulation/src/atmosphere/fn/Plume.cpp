#include "atmosphere/fn/Kernels.hpp"

#include "core/service/Workers.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;
constexpr double LevelDepth[PlumeLevels] = {2000, 4000, 4000, 8000};   // m that dust settles through at each level
constexpr double Conversion[PlumeLevels] = {3, 5, 10, 35};             // days for sulphur dioxide to turn into sulphate
// Days sulphate stays at a level. From the three bands it reaches the ground; from above the
// tropopause it only sinks into the high band, which is what makes a high veil last a year or two.
constexpr double VeilLife[PlumeLevels] = {5, 10, 25, 400};
constexpr double Washing = 0.35;        // per kg/m² of precipitation: e-folds of what the low and middle band hold that it takes down
constexpr double AshOptics = 300.0;     // visible optical depth per kg/m² of fine ash
constexpr double VeilOptics = 4300.0;   // per kg/m² of sulphur dioxide turned sulphate
constexpr double AshDim = 0.6;          // share of sunlight a thick ash cloud keeps from the surface
constexpr double VeilDim = 0.105;       // the same for a thick sulphate veil, which scatters most of it onwards
constexpr double SpreadShare = 0.3;     // spread of a blanket of ash over its distance from the source
constexpr double ColumnSpread = 0.15;   // spread at the source over the height the ash fell from
constexpr double ColumnHeat = 1.0e7;    // J/m²/K of the air column that a source's heat warms
constexpr double Trace = 1e-12;         // kg/m² below which a level counts as clean
constexpr int Carriers = 3 * PlumeLevels;

double sum(const Field<double>& field) {
    double all = 0;
    for (double value : field.cells) all += value;
    return all;
}

// kg/m² a patch laid on a point.
double blanket(const AshPatch& patch, Vec2 position) {
    const double dx = wrapDelta(position.x - patch.centre.x, PlanetSpan), dz = wrapDelta(position.z - patch.centre.z, PlanetSpan);
    const double variance = patch.spread * patch.spread;
    return patch.mass * std::exp(-0.5 * (dx * dx + dz * dz) / variance) / (2 * Pi * variance);
}

// What one source gives off in `seconds` of a step.
void emit(Atmosphere& atmosphere, const AirSource& source, double seconds) {
    Plume& plume = atmosphere.plume;
    const int x = wrapAir(floorToInt(source.position.x / AirCell)), z = wrapAir(floorToInt(source.position.z / AirCell));
    const size_t cell = atmosphere.ground.index(x, z);
    const ColumnProfile profile = profileOf(columnAt(atmosphere, x, z));
    const double above = std::max(100.0, source.height - atmosphere.ground.cells[cell]);
    int level = Low;
    while (level + 1 < PlumeLevels && above > profile.height[size_t(level) + 1]) ++level;
    const int band = std::min(level, int(High));
    const Vec2 wind = bandWind(atmosphere, band, x, z);
    const double speed = length(wind);

    // Lapilli and sand-size ash land where the wind at the source's height carries them while they fall.
    for (int grain : {Lapilli, Sand}) {
        const double mass = source.ash[size_t(grain)] * seconds;
        if (mass <= 0) continue;
        const double reach = speed * above / AshSettling[size_t(grain)];
        AshPatch landed{source.id, grain, source.position + wind * (above / AshSettling[size_t(grain)]), SpreadShare * reach + ColumnSpread * above, mass};
        bool merged = false;
        for (size_t back = 0; back < 8 && back < plume.patches.size() && !merged; ++back) {
            AshPatch& patch = plume.patches[plume.patches.size() - 1 - back];
            const double apart = std::hypot(wrapDelta(landed.centre.x - patch.centre.x, PlanetSpan), wrapDelta(landed.centre.z - patch.centre.z, PlanetSpan));
            if (patch.source != landed.source || patch.grain != grain || apart > 0.25 * std::min(patch.spread, landed.spread)) continue;
            const double share = mass / (patch.mass + mass);
            patch.centre = patch.centre + Vec2{wrapDelta(landed.centre.x - patch.centre.x, PlanetSpan), wrapDelta(landed.centre.z - patch.centre.z, PlanetSpan)} * share;
            patch.spread = mix(patch.spread, landed.spread, share);
            patch.mass += mass;
            merged = true;
        }
        if (!merged) plume.patches.push_back(landed);
        const size_t under = atmosphere.ground.index(wrapAir(floorToInt(landed.centre.x / AirCell)), wrapAir(floorToInt(landed.centre.z / AirCell)));
        plume.fallen.cells[under] += mass / AirCellArea;
        plume.fallRate.cells[under] += mass / (AirCellArea * AirStep);
        plume.totals.ashEmitted += mass;
        plume.totals.ashFallen += mass;
    }
    plume.dust[size_t(level)].cells[cell] += source.ash[Dust] * seconds / AirCellArea;
    plume.gas[size_t(level)].cells[cell] += source.sulphur * seconds / AirCellArea;
    plume.totals.ashEmitted += source.ash[Dust] * seconds;
    plume.totals.sulphurEmitted += source.sulphur * seconds;
    atmosphere.vapour[size_t(band)].cells[cell] += source.vapour * seconds / AirCellArea;
    plume.totals.vapourEmitted += source.vapour * seconds;
    atmosphere.airTemperature.cells[cell] += source.heat * seconds / (ColumnHeat * AirCellArea);
    if (source.ash[Dust] > 0 || source.sulphur > 0) plume.aloft = true;
}

// Dust settles from level to level and to the ground, gas turns into veil, the veil thins out,
// and rain takes down what the two lower bands hold. The same pass measures what is left overhead.
void settle(Atmosphere& atmosphere) {
    Plume& plume = atmosphere.plume;
    double falling[PlumeLevels], turning[PlumeLevels], thinning[PlumeLevels];
    for (int level = 0; level < PlumeLevels; ++level) {
        falling[level] = std::min(1.0, AshSettling[Dust] * AirStep / LevelDepth[level]);
        turning[level] = 1.0 - std::exp(-AirStep / (Conversion[level] * DaySeconds));
        thinning[level] = 1.0 - std::exp(-AirStep / (VeilLife[level] * DaySeconds));
    }
    double ashFallen = 0, sulphurFallen = 0, left = 0;
    for (size_t cell = 0; cell < plume.fallen.size(); ++cell) {
        const double rain = (atmosphere.rainRate.cells[cell] + atmosphere.snowRate.cells[cell]) * AirStep;
        const double washed = rain > 0 ? 1.0 - std::exp(-Washing * rain) : 0.0;
        double ash = 0, sulphur = 0, dustOver = 0, veilOver = 0, arriving = 0, sinking = 0;
        for (int level = PlumeLevels - 1; level >= 0; --level) {
            const size_t at = size_t(level);
            double dust = plume.dust[at].cells[cell] + arriving, gas = plume.gas[at].cells[cell], veil = plume.veil[at].cells[cell] + sinking;
            const double out = level <= Middle ? washed : 0.0;
            const double turned = gas * turning[level];
            gas -= turned;
            veil += turned;
            const double gasDown = gas * out, veilGone = veil * (out + (1.0 - out) * thinning[level]);
            const double dustDown = dust * out, dustFalling = (dust - dustDown) * falling[level];
            plume.dust[at].cells[cell] = dust = dust - dustDown - dustFalling;
            plume.gas[at].cells[cell] = gas = gas - gasDown;
            plume.veil[at].cells[cell] = veil = veil - veilGone;
            arriving = level > Low ? dustFalling : 0.0;
            sinking = level == PlumeLevels - 1 ? veilGone : 0.0;
            ash += dustDown + (level == Low ? dustFalling : 0.0);
            sulphur += gasDown + (level == PlumeLevels - 1 ? 0.0 : veilGone);
            dustOver += dust;
            veilOver += veil;
            left += dust + gas + veil;
        }
        plume.dustFallen.cells[cell] += ash;
        plume.fallen.cells[cell] += ash;
        plume.fallRate.cells[cell] += ash / AirStep;
        ashFallen += ash;
        sulphurFallen += sulphur;
        const double ashDepth = AshOptics * dustOver, veilDepth = VeilOptics * veilOver;
        plume.depth.cells[cell] = ashDepth + veilDepth;
        plume.dimming.cells[cell] = 1.0 - (1.0 - AshDim * (1.0 - std::exp(-ashDepth))) * (1.0 - VeilDim * (1.0 - std::exp(-veilDepth)));
    }
    plume.totals.ashFallen += ashFallen * AirCellArea;
    plume.totals.sulphurFallen += sulphurFallen * AirCellArea;
    plume.aloft = left > Trace;
}

} // namespace

void shapePlume(Atmosphere& atmosphere) {
    const Field<double> blank(AirCells, AirCells);
    Plume& plume = atmosphere.plume;
    for (auto* levels : {&plume.dust, &plume.gas, &plume.veil})
        for (Field<double>& field : *levels) field = blank;
    for (Field<double>* field : {&plume.fallen, &plume.dustFallen, &plume.fallRate, &plume.depth, &plume.dimming}) *field = blank;
}

void addSource(Atmosphere& atmosphere, const AirSource& source) { atmosphere.plume.sources.push_back(source); }

void stepPlume(Atmosphere& atmosphere, Workers& workers) {
    Plume& plume = atmosphere.plume;
    if (plume.sources.empty() && !plume.aloft) return;
    std::fill(plume.fallRate.cells.begin(), plume.fallRate.cells.end(), 0.0);
    const double from = double(atmosphere.step) * AirStep, to = from + AirStep;
    for (const AirSource& source : plume.sources) {
        const double seconds = std::min(to, source.end) - std::max(from, source.start);
        if (seconds > 0) emit(atmosphere, source, seconds);
    }
    if (!plume.aloft) {
        std::fill(plume.depth.cells.begin(), plume.depth.cells.end(), 0.0);
        std::fill(plume.dimming.cells.begin(), plume.dimming.cells.end(), 0.0);
        return;
    }

    // Every level of dust, gas and veil rides the wind of its band, the level above the tropopause
    // that of the high band, for the wind steps of this step. Each field is one task with its own
    // work space, and levels that hold nothing are left alone.
    if (plume.work.empty()) plume.work.assign(2 * Carriers, Field<double>(AirCells, AirCells));
    std::array<Field<double>, PlumeLevels>* const kinds[3] = {&plume.dust, &plume.gas, &plume.veil};
    workers.parallelFor(Carriers, [&](int begin, int end) {
        for (int carrier = begin; carrier < end; ++carrier) {
            const int level = carrier % PlumeLevels;
            Field<double>& field = (*kinds[carrier / PlumeLevels])[size_t(level)];
            if (sum(field) <= 0) continue;
            for (int part = 0; part < WindSteps; ++part) {
                advect(atmosphere, field, plume.work[size_t(2 * carrier)], plume.work[size_t(2 * carrier + 1)], std::min(level, int(High)), Carried::Cloud);
                std::swap(field.cells, plume.work[size_t(2 * carrier)].cells);
            }
        }
    });
    settle(atmosphere);
}

AshFall ashFallAt(const Atmosphere& atmosphere, Vec2 position) {
    const Plume& plume = atmosphere.plume;
    const double x = position.x / AirCell - 0.5, z = position.z / AirCell - 0.5;
    AshFall fall;
    fall.mass = sample(plume.dustFallen, x, z, Kernel::Linear, Edge::Wrap);
    fall.rate = sample(plume.fallRate, x, z, Kernel::Linear, Edge::Wrap);
    double most = 0;
    for (const AshPatch& patch : plume.patches) {
        const double here = blanket(patch, position);
        fall.mass += here;
        if (here > most) most = here, fall.source = patch.source;
    }
    // Far from every blanket the ash is the dust of whichever source gave off most of it.
    double largest = 0;
    for (const AirSource& source : plume.sources) {
        const double dust = source.ash[Dust] * (source.end - source.start);
        if (fall.mass > 0 && most < 0.5 * fall.mass && dust > largest) largest = dust, fall.source = source.id;
    }
    for (const AirSource& source : plume.sources)
        if (source.id == fall.source && fall.mass > 0) fall.since = fall.since == 0 ? source.start : std::min(fall.since, source.start);
    if (fall.mass <= 0) fall.source = 0;
    return fall;
}

PlumeLedger plumeLedger(const Atmosphere& atmosphere) {
    PlumeLedger ledger = atmosphere.plume.totals;
    for (size_t level = 0; level < PlumeLevels; ++level) {
        ledger.ashAloft += sum(atmosphere.plume.dust[level]) * AirCellArea;
        ledger.sulphurGas += sum(atmosphere.plume.gas[level]) * AirCellArea;
        ledger.sulphurVeil += sum(atmosphere.plume.veil[level]) * AirCellArea;
    }
    return ledger;
}

} // namespace derivean::sim

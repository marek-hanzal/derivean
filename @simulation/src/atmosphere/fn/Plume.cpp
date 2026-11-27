// 264 lines, kept whole: one plume, from its sources through the air to the ground and the snow.
#include "atmosphere/fn/Kernels.hpp"

#include "core/service/Workers.hpp"

#include <atomic>

namespace derivean::sim {
namespace {

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
constexpr double ClearSky = 1e-7;       // planet-mean optical depth below which what is left aloft is put down and the sky counts as clean
constexpr double BuryingSnow = 5.0;     // kg/m² of fresh snow that hides the ash under it e-fold
// kg/m² of ash on snow below which it is gone: it darkens the snow by under 2e-9 of a full blanket
// (Heat.cpp), and without a floor a trace buried ever deeper would lie on until the exponent underflowed.
constexpr double TraceAsh = 1e-9;
constexpr int Carriers = 3 * PlumeLevels;
constexpr int RowGrain = 16;   // rows per task of settling and soot

double sum(const Field<double>& field) {
    double all = 0;
    for (double value : field.cells) all += value;
    return all;
}

// Books a landed blanket into the cells it covers, by the blanket's own shape, so that the cells
// together receive exactly its mass.
void book(Plume& plume, const AshPatch& landed) {
    const int reach = std::clamp(int(std::ceil(2.5 * landed.spread / AirCell)), 1, 8);
    const int centreX = floorToInt(landed.centre.x / AirCell), centreZ = floorToInt(landed.centre.z / AirCell);
    double weights[17][17], all = 0;
    for (int dz = -reach; dz <= reach; ++dz) {
        for (int dx = -reach; dx <= reach; ++dx) {
            const double offX = (centreX + dx + 0.5) * AirCell - landed.centre.x, offZ = (centreZ + dz + 0.5) * AirCell - landed.centre.z;
            all += weights[dz + reach][dx + reach] = std::exp(-0.5 * (offX * offX + offZ * offZ) / (landed.spread * landed.spread));
        }
    }
    for (int dz = -reach; dz <= reach; ++dz) {
        for (int dx = -reach; dx <= reach; ++dx) {
            // A blanket much narrower than a cell lies in the cell under its centre.
            const double share = all > 0 ? weights[dz + reach][dx + reach] / all : (dx == 0 && dz == 0 ? 1.0 : 0.0);
            const size_t cell = plume.fallen.index(wrapAir(centreX + dx), wrapAir(centreZ + dz));
            plume.fallen.cells[cell] += landed.mass * share / AirCellArea;
            plume.fallRate.cells[cell] += landed.mass * share / (AirCellArea * AirStep);
        }
    }
    plume.falling = true;
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
    for (int grain : {Lapilli, Grit}) {
        const double mass = source.ash[size_t(grain)] * seconds;
        if (mass <= 0) continue;
        const double reach = speed * above / AshSettling[size_t(grain)];
        AshPatch landed{source.id, grain, source.position + wind * (above / AshSettling[size_t(grain)]), SpreadShare * reach + ColumnSpread * above, mass};
        bool merged = false;
        for (size_t back = 0; back < 256 && back < plume.patches.size() && !merged; ++back) {
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
        book(plume, landed);
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
// Rows are worked side by side; the totals are then summed cell by cell in the order of a single sweep.
void settle(Atmosphere& atmosphere, Workers& workers) {
    Plume& plume = atmosphere.plume;
    double falling[PlumeLevels], turning[PlumeLevels], thinning[PlumeLevels];
    for (int level = 0; level < PlumeLevels; ++level) {
        falling[level] = std::min(1.0, AshSettling[Dust] * AirStep / LevelDepth[level]);
        turning[level] = 1.0 - std::exp(-AirStep / (Conversion[level] * DaySeconds));
        thinning[level] = 1.0 - std::exp(-AirStep / (VeilLife[level] * DaySeconds));
    }
    // What each cell put down this step, kept for the sums, in the first carrier's transport space:
    // settling follows the transport, which allocated it, and every transport writes its output
    // whole before reading it and never reads the slope of a cloud field.
    std::vector<double>& ashOf = plume.work[0].cells;
    std::vector<double>& sulphurOf = plume.work[1].cells;
    workers.parallelFor(AirCells, [&](int begin, int end) {
        for (size_t cell = size_t(begin) * AirCells; cell < size_t(end) * AirCells; ++cell) {
            const double rain = (atmosphere.rainRate.cells[cell] + atmosphere.snowRate.cells[cell]) * AirStep;
            const double washed = rain > 0 ? 1.0 - std::exp(-Washing * rain) : 0.0;
            double ash = 0, sulphur = 0, dustOver = 0, veilOver = 0, arriving = 0, sinking = 0, low = 0;
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
                if (level == Low) low = AshOptics * dust + VeilOptics * veil;
            }
            plume.dustFallen.cells[cell] += ash;
            plume.fallen.cells[cell] += ash;
            plume.fallRate.cells[cell] += ash / AirStep;
            ashOf[cell] = ash, sulphurOf[cell] = sulphur;
            const double ashDepth = AshOptics * dustOver, veilDepth = VeilOptics * veilOver;
            plume.depth.cells[cell] = ashDepth + veilDepth;
            plume.lowDepth.cells[cell] = low;
            // 1 − e⁻⁰ is +0 for either zero.
            const double ashShare = ashDepth == 0 ? 0.0 : 1.0 - std::exp(-ashDepth), veilShare = veilDepth == 0 ? 0.0 : 1.0 - std::exp(-veilDepth);
            plume.dimming.cells[cell] = 1.0 - (1.0 - AshDim * ashShare) * (1.0 - VeilDim * veilShare);
        }
    }, RowGrain);
    double ashFallen = 0, sulphurFallen = 0, left = 0, haze = 0, gasOf[PlumeLevels] = {};
    for (size_t cell = 0; cell < plume.fallen.size(); ++cell) {
        for (int level = PlumeLevels - 1; level >= 0; --level) {
            const size_t at = size_t(level);
            const double gas = plume.gas[at].cells[cell];
            left += plume.dust[at].cells[cell] + gas + plume.veil[at].cells[cell];
            gasOf[at] += gas;
        }
        plume.falling = plume.falling || ashOf[cell] > 0;
        ashFallen += ashOf[cell];
        sulphurFallen += sulphurOf[cell];
        haze += plume.depth.cells[cell];
    }
    // Once what is left no longer dims anything, it is put down where it floats and the sky is clean.
    double gas = 0;
    for (const double level : gasOf) gas += level;
    plume.aloft = left > 0;
    if (plume.aloft && (haze + VeilOptics * gas) / double(plume.depth.size()) < ClearSky) {
        for (size_t cell = 0; cell < plume.fallen.size(); ++cell) {
            for (size_t level = 0; level < PlumeLevels; ++level) {
                plume.dustFallen.cells[cell] += plume.dust[level].cells[cell];
                plume.fallen.cells[cell] += plume.dust[level].cells[cell];
                ashFallen += plume.dust[level].cells[cell];
                sulphurFallen += plume.gas[level].cells[cell] + plume.veil[level].cells[cell];
                plume.dust[level].cells[cell] = plume.gas[level].cells[cell] = plume.veil[level].cells[cell] = 0;
            }
            plume.depth.cells[cell] = plume.lowDepth.cells[cell] = plume.dimming.cells[cell] = 0;
        }
        plume.aloft = false;
    }
    plume.totals.ashFallen += ashFallen * AirCellArea;
    plume.totals.sulphurFallen += sulphurFallen * AirCellArea;
}

// Ash that fell this step lies on the snow; snow that fell on it hides it, and it is gone where
// the pack is gone. Where nothing lies or no snow falls the share left is the ash itself.
void soot(Atmosphere& atmosphere, Workers& workers) {
    Plume& plume = atmosphere.plume;
    std::atomic<bool> sooted{false};
    workers.parallelFor(AirCells, [&](int begin, int end) {
        bool any = false;
        for (size_t cell = size_t(begin) * AirCells; cell < size_t(end) * AirCells; ++cell) {
            double lying = 0.0;
            if (atmosphere.snow.cells[cell] > 0) {
                const double ash = plume.onSnow.cells[cell] + plume.fallRate.cells[cell] * AirStep, burying = atmosphere.snowRate.cells[cell];
                lying = ash == 0 || burying == 0 ? ash : ash * std::exp(-burying * AirStep / BuryingSnow);
            }
            plume.onSnow.cells[cell] = lying >= TraceAsh ? lying : 0.0;
            any = any || lying >= TraceAsh;
        }
        if (any) sooted.store(true, std::memory_order_relaxed);
    }, RowGrain);
    plume.sooted = sooted.load(std::memory_order_relaxed);
}

} // namespace

void shapePlume(Atmosphere& atmosphere) {
    const Field<double> blank(AirCells, AirCells);
    Plume& plume = atmosphere.plume;
    for (auto* levels : {&plume.dust, &plume.gas, &plume.veil})
        for (Field<double>& field : *levels) field = blank;
    for (Field<double>* field : {&plume.fallen, &plume.onSnow, &plume.dustFallen, &plume.fallRate, &plume.depth, &plume.lowDepth, &plume.dimming}) *field = blank;
}

void addSource(Atmosphere& atmosphere, const AirSource& source) { atmosphere.plume.sources.push_back(source); }

void stepPlume(Atmosphere& atmosphere, Workers& workers) {
    Plume& plume = atmosphere.plume;
    const double from = double(atmosphere.step) * AirStep, to = from + AirStep;
    bool emitting = false;
    for (const AirSource& source : plume.sources) emitting = emitting || (source.end > from && source.start < to);
    if (!emitting && !plume.aloft && !plume.sooted && !plume.falling) return;
    std::fill(plume.fallRate.cells.begin(), plume.fallRate.cells.end(), 0.0);
    plume.falling = false;
    for (const AirSource& source : plume.sources) {
        const double seconds = std::min(to, source.end) - std::max(from, source.start);
        if (seconds > 0) emit(atmosphere, source, seconds);
    }
    if (!plume.aloft) {
        soot(atmosphere, workers);
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
    settle(atmosphere, workers);
    soot(atmosphere, workers);
}

} // namespace derivean::sim

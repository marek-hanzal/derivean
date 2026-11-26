#include "volcanic/fn/Outbreak.hpp"

#include "chronology/type/Calendar.hpp"
#include "core/fn/Hash.hpp"
#include "landscape/fn/Process.hpp"
#include "volcanic/constant/Rheology.hpp"
#include "volcanic/fn/Emplace.hpp"
#include "volcanic/fn/Minor.hpp"
#include "volcanic/fn/Source.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr uint64_t OutbreakStream = 0x4f555442;
constexpr double RockDensity = 2600.0;   // kg/m³ of melt as dense rock
constexpr double CraterSpread = 150.0;   // m around the conduit within which a crater opens

// CLOSED FORM with assumed coefficients. A larger eruption drains its chamber faster: m³/s of
// fragments for 1e8 m³ and the power of the volume it grows with, so that small eruptions go on
// for a day or two and a Pinatubo empties in about a day. Lava leaves at a rate the conduit holds
// open, fluid basalt fast, viscous andesite as a slow dome.
constexpr double BlastRate = 1.0e3, BlastPower = 1.1;
constexpr double LavaRate[2] = {50.0, 2.0}, LavaPower = 0.5;
// Height of an eruption column against the rate of dense rock through it (Mastin et al. 2009,
// a fit over historical eruptions): H = 2.0 km × Q^0.241.
constexpr double ColumnScale = 2000.0, ColumnPower = 0.241;
// A plume over flowing lava rises on the share of its heat that goes into the air (Settle 1978:
// H = 8.2 m × W^(1/4)).
constexpr double PlumeScale = 8.2, PlumeHeatShare = 0.1;
constexpr double MeltHeat = 1.2e6;       // J/kg given off cooling from eruption to air temperature
// Of the gas in the melt, by Magma: sulphur dioxide (Laki 3e-3, Pinatubo 1.3e-3) and water.
constexpr double SulphurShare[2] = {3e-3, 1.3e-3};
constexpr double WaterShare[2] = {0.007, 0.04};
// Of the hot currents, what rises off them as fine ash.
constexpr double CurrentLoft = 0.3;
// How the ash in the column divides by grain class: little of it stays aloft as dust, most of the
// fine ash aggregates and falls with the coarse.
constexpr double GrainShare[AshClasses] = {0.35, 0.62, 0.03};

} // namespace

double rockMass(double volume) { return volume * RockDensity; }

Outbreak outbreakOf(uint64_t seed, const Vent& vent, int64_t number, double time, double volume) {
    Outbreak outbreak;
    const size_t magma = size_t(vent.magma);
    outbreak.vent = vent.id;
    outbreak.number = number;
    outbreak.magma = vent.magma;
    outbreak.time = time;
    outbreak.volume = volume;
    const uint64_t base = stream(seed, OutbreakStream) ^ vent.id;
    const Vec2 conduit = vent.feeders.empty() ? vent.centre : vent.feeders.back().site;
    outbreak.site = conduit + Vec2{unit(base, number, 1) - 0.5, unit(base, number, 2) - 0.5} * CraterSpread;
    outbreak.empties = volume >= CalderaVolume[magma];
    // An eruption of the size the local level builds on its own is built as it builds them: a cone
    // of fragments and a short flow, with the ash a column of its magma carries off.
    outbreak.small = !outbreak.empties && volume <= minorSize(vent.magma);
    const Style& style = outbreak.empties ? ParoxysmStyle : styleOf(vent.magma);
    outbreak.ash = volume * style.ash;
    if (outbreak.small) {
        outbreak.fall = (volume - outbreak.ash) * minorStyle(vent.magma, true).cone;
        outbreak.lava = volume - outbreak.ash - outbreak.fall;
    } else {
        outbreak.fall = volume * style.cone;
        outbreak.flows = volume * style.flows;
        outbreak.lava = volume * style.lava;
    }
    const double fragments = outbreak.fall + outbreak.flows + outbreak.ash;
    if (fragments > 0) {
        outbreak.blastRate = BlastRate * std::pow(fragments / 1e8, BlastPower);
        outbreak.blast = fragments / outbreak.blastRate;
        outbreak.column = ColumnScale * std::pow(outbreak.blastRate, ColumnPower);
    }
    if (outbreak.lava > 0) {
        outbreak.lavaRate = LavaRate[magma] * std::pow(outbreak.lava / 1e8, LavaPower);
        outbreak.effusion = outbreak.lava / outbreak.lavaRate;
        outbreak.plume = PlumeScale * std::pow(PlumeHeatShare * rockMass(outbreak.lavaRate) * MeltHeat, 0.25);
    }
    return outbreak;
}

void outbreaksBetween(uint64_t seed, const Vent& vent, double from, double to, std::vector<Outbreak>& into) {
    // The record keeps the last few eruptions before its end; their numbers come just before `count`.
    const int64_t firstRemembered = vent.count - int64_t(vent.recent.size());
    for (size_t k = 0; k < vent.recent.size(); ++k) {
        const Eruption& eruption = vent.recent[k];
        if (eruption.time >= from && eruption.time < to) into.push_back(outbreakOf(seed, vent, firstRemembered + int64_t(k), eruption.time, eruption.volume));
    }
    Eruption next;
    if (!nextEruption(seed, vent, next)) return;
    for (int64_t number = vent.count; next.time < to; ++number) {
        if (next.time >= from) into.push_back(outbreakOf(seed, vent, number, next.time, next.volume));
        next.volume = eruptionVolume(seed, vent, number + 1);
        next.time += next.volume / vent.supply;
    }
}

void airSourcesOf(const Outbreak& outbreak, double era, double ground, std::vector<AirSource>& into) {
    const size_t magma = size_t(outbreak.magma);
    const double start = (outbreak.time - era) * YearSeconds;
    if (outbreak.blast > 0) {
        AirSource source;
        source.id = hash(outbreak.vent, outbreak.number, 1);
        source.position = outbreak.site;
        source.height = ground + outbreak.column;
        source.start = start;
        source.end = start + outbreak.blast;
        const double rate = rockMass(outbreak.blastRate);
        const double lofted = rockMass(outbreak.ash + CurrentLoft * outbreak.flows) / outbreak.blast;
        for (int grain = 0; grain < AshClasses; ++grain) source.ash[size_t(grain)] = lofted * GrainShare[grain];
        source.sulphur = rate * SulphurShare[magma];
        source.vapour = rate * WaterShare[magma];
        source.heat = rate * MeltHeat;
        into.push_back(source);
    }
    if (outbreak.effusion > 0) {
        AirSource source;
        source.id = hash(outbreak.vent, outbreak.number, 2);
        source.position = outbreak.site;
        source.height = ground + outbreak.plume;
        source.start = start + outbreak.blast;
        source.end = source.start + outbreak.effusion;
        const double rate = rockMass(outbreak.lavaRate);
        source.sulphur = rate * SulphurShare[magma];
        source.vapour = rate * WaterShare[magma];
        source.heat = PlumeHeatShare * rate * MeltHeat;
        into.push_back(source);
    }
}

NearField layOutbreak(Emplacement& field, const Outbreak& outbreak) {
    NearField near;
    if (field.cells() == 0 || field.cell <= 0) return near;
    near.fragments.assign(field.cells(), 0.0f);
    near.lava.assign(field.cells(), 0.0f);
    std::fill(field.laid.begin(), field.laid.end(), 0.0f);
    const double before = field.lost + field.passed;
    // Moves what the laws laid since the last call into `into`; what lies on cells the grid may not
    // change is booked as left, as the replay books it (volcanic/fn/Lay.cpp).
    const auto harvest = [&](std::vector<float>& into) {
        for (size_t i = 0; i < field.cells(); ++i) {
            if (field.laid[i] <= 0) continue;
            if (field.rest[i] == Rest::Open) into[i] += field.laid[i];
            else field.passed += double(field.laid[i]) * field.cell * field.cell;
            field.laid[i] = 0;
        }
    };
    const int x = int(std::floor((outbreak.site.x - field.origin.x) / field.cell)), z = int(std::floor((outbreak.site.z - field.origin.z) / field.cell));
    if (outbreak.fall > 0) {
        buildCone(field, coneFall(outbreak.site, outbreak.fall), ReposeTan[Sand]);
        harvest(near.fragments);
    }
    if (outbreak.flows > 0) {
        if (field.contains(x, z)) pourLava(field, outbreak.empties ? AshFlow : BlockFlow, x, z, outbreak.flows);
        else field.lost += outbreak.flows;
        harvest(near.fragments);
    }
    if (outbreak.lava > 0) {
        if (field.contains(x, z)) pourLava(field, outbreak.small ? minorStyle(outbreak.magma, true).lava : lavaOf(outbreak.magma), x, z, outbreak.lava);
        else field.lost += outbreak.lava;
        harvest(near.lava);
    }
    near.drop.assign(field.cells(), 0.0f);
    if (outbreak.empties) {
        near.drop = founder(field, outbreak.site, outbreak.volume, outbreak.magma);
        for (size_t i = 0; i < field.cells(); ++i) {
            if (field.rest[i] != Rest::Open) near.drop[i] = 0;
            field.ground[i] -= near.drop[i];
        }
    }
    near.left = field.lost + field.passed - before;
    return near;
}

std::shared_ptr<const Deposit> depositOf(const Emplacement& field, const NearField& near) {
    auto laid = std::make_shared<Deposit>();
    laid->origin = field.origin;
    laid->cell = field.cell;
    laid->width = field.width, laid->height = field.height;
    if (near.fragments.size() != field.cells() || near.lava.size() != field.cells()) return laid;
    laid->drop = near.drop;
    laid->rock.assign(field.cells(), 0.0f);
    for (auto& layer : laid->loose) layer.assign(field.cells(), 0.0f);
    for (size_t i = 0; i < field.cells(); ++i) {
        const float fragments = near.fragments[i], lava = near.lava[i];
        if (fragments + lava >= WeldDepth) {
            laid->rock[i] = fragments + lava;
            continue;
        }
        laid->loose[Coarse][i] = lava + 0.5f * fragments;
        laid->loose[Sand][i] = 0.5f * fragments;
    }
    return laid;
}

} // namespace derivean::sim

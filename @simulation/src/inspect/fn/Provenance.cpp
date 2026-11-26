#include "inspect/fn/Provenance.hpp"

#include "chronology/type/Calendar.hpp"
#include "landscape/fn/Process.hpp"
#include "material/constant/Materials.hpp"
#include "measure/fn/Format.hpp"
#include "strata/type/Column.hpp"


namespace derivean::sim {
namespace {

constexpr float BareCover = 0.05f;       // m below which the rock counts as exposed
constexpr float WaveReach = 1.5f;        // m above spring tide that storm waves rework
constexpr float MantleLimit = 3.0f;      // m beyond which a cover is more than rock weathering under its own waste leaves
constexpr float RecordedShare = 0.25f;   // share of the cover a recorded deposit must reach to explain it
constexpr float ShareTolerance = 0.3f;   // how far a mantle's sand share may lie from what its bed weathers to

const char* rockName(Material material) { return definitionOf(material).name.data(); }

} // namespace

Provenance explainSurface(const SurfaceEvidence& e) {
    Provenance result;
    result.material = e.surface;
    const float thickness = e.cover[Coarse] + e.cover[Sand] + e.cover[Fines];
    const char* surface = rockName(e.surface);

    if (thickness < BareCover) {
        result.origin = SurfaceOrigin::ExposedRock;
        result.reason = e.waterDepth > 0 ? formatText("Bare %s under %.1f m of water: nothing has settled on it.", rockName(e.bed), e.waterDepth)
                                         : formatText("Bare %s: with a slope of %.0f %% and erosion of %.2f mm/yr nothing loose stays on it.", rockName(e.bed),
                                                100 * e.slope, 1000 * e.erosionRate);
        return result;
    }
    if (e.waterDepth > 0) {
        result.origin = e.lake ? SurfaceOrigin::LakeFloor : SurfaceOrigin::SeaFloor;
        result.reason = formatText("%s settled under %.1f m of standing %s water (%.1f m laid down in the replay, %.2f mm/yr now).", surface, e.waterDepth,
                             e.lake ? "fresh" : "sea", e.deposited, 1000 * e.depositionRate);
        return result;
    }
    if (e.aboveSea >= 0 && e.aboveSea <= float(SpringTide) + WaveReach) {
        result.origin = SurfaceOrigin::Shore;
        result.reason = formatText("%s %.1f m above the sea, inside the %.1f m band that spring tides and waves rework.", surface, e.aboveSea,
                             float(SpringTide) + WaveReach);
        return result;
    }
    // Debris lies at or near the angle its blocks hold and has steeper ground above to have come from.
    if (e.cover[Coarse] >= 0.5f * thickness && e.steepestAbove >= ReposeTan[Coarse] && e.slope >= 0.5f * ReposeTan[Coarse]) {
        result.origin = SurfaceOrigin::SlopeDebris;
        result.reason = formatText("%.1f m of mostly coarse debris on a %.0f %% slope below ground that climbs at %.0f %%, steeper than loose blocks hold (%.0f %%).",
                             thickness, 100 * e.slope, 100 * e.steepestAbove, 100 * ReposeTan[Coarse]);
        return result;
    }
    if (e.deposited >= RecordedShare * thickness && e.discharge >= ChannelHead) {
        result.origin = SurfaceOrigin::RiverDeposit;
        result.reason = formatText("%s laid down by running water: %.1f m deposited in the replay under a flow of %.2g m3/yr%s.", surface, e.deposited, double(e.discharge),
                             e.steepestAbove > 2 * e.slope ? ", on a floor at the foot of steeper ground" : "");
        return result;
    }
    const RockTraits& bed = traitsOf(e.bed);
    const float loose = bed.coarse + bed.sand + bed.fines;
    const float expected = loose > 0 ? bed.sand / loose : 0.0f;
    if (thickness <= MantleLimit && e.deposited < RecordedShare * thickness && std::abs(e.cover[Sand] / thickness - expected) <= ShareTolerance) {
        result.origin = SurfaceOrigin::WeatheredInPlace;
        result.reason = formatText("%.1f m of mantle weathered from the %s underneath, which breaks down to %.0f %% sand; the mantle holds %.0f %%.", thickness,
                             rockName(e.bed), 100 * expected, 100 * e.cover[Sand] / thickness);
        return result;
    }
    if (e.deposited >= RecordedShare * thickness) {
        result.origin = SurfaceOrigin::RiverDeposit;
        result.reason = formatText("%s washed in by water too thin to cut a channel: %.1f m deposited in the replay under a flow of %.2g m3/yr.", surface, e.deposited,
                             double(e.discharge));
        return result;
    }
    if (e.steepestAbove > e.slope && e.steepestAbove > 0.02f) {
        result.origin = SurfaceOrigin::SlopeWash;
        result.reason = formatText("%.1f m of soil crept down from ground that climbs at %.0f %% above a %.0f %% slope; no stream deposit is recorded here.", thickness,
                             100 * e.steepestAbove, 100 * e.slope);
        return result;
    }
    result.origin = SurfaceOrigin::Unexplained;
    result.reason = formatText("%.1f m of %s that the simulation did not put here: no deposit recorded, no slope above, and the %s underneath weathers to %.0f %% sand "
                         "against %.0f %% found. Look at the voxel conversion or the far mesh.",
                         thickness, surface, rockName(e.bed), 100 * expected, 100 * e.cover[Sand] / thickness);
    return result;
}

} // namespace derivean::sim

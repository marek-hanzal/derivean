#include "inspect/fn/Inspect.hpp"

#include "material/constant/Materials.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Format.hpp"
#include "strata/type/Column.hpp"


namespace derivean::sim {
namespace {

using Entries = std::vector<std::pair<std::string, std::string>>;

std::string nameOf(Material material) { return std::string(definitionOf(material).name); }

std::string years(double value) {
    return value >= 1e9 ? formatText("%.2f Gyr", value / 1e9) : value >= 1e6 ? formatText("%.1f Myr", value / 1e6) : formatText("%.0f kyr", value / 1e3);
}

// Rates are small numbers; millimetres per year keep them readable next to metres of total.
std::string rate(double metresPerYear) { return formatText("%.4f mm/yr", 1000 * metresPerYear); }

std::string vector(Vec2 value, double scale, const char* unit) {
    const double heading = std::atan2(value.x, -value.z) * DegreesPerRadian;
    return formatText("%.1f %s towards %.0f deg", length(value) * scale, unit, heading < 0 ? heading + 360 : heading);
}

void describeRates(Entries& out, const char* group, const InspectRates& rates) {
    const std::string head = std::string(group) + " / ";
    out.push_back({head + "Window", years(rates.window)});
    out.push_back({head + "Uplift rate now", rate(rates.upliftNow)});
    out.push_back({head + "Uplift total", formatText("%.1f m", rates.upliftTotal)});
    out.push_back({head + "Erosion rate now", rate(rates.erosionNow)});
    out.push_back({head + "Erosion total", formatText("%.1f m", rates.erosionTotal)});
    out.push_back({head + "Deposition rate now", rate(rates.depositionNow)});
    out.push_back({head + "Deposition total", formatText("%.1f m", rates.depositionTotal)});
}

// A value outside the names known here means the enum grew; it is shown as such rather than read past the table.
template<size_t Count>
const char* named(const char* const (&names)[Count], size_t index) { return index < Count ? names[index] : "unnamed"; }

} // namespace

const char* nameOf(Regime regime) {
    static constexpr const char* Names[] = {"quiet", "compression", "extension", "shear"};
    return named(Names, size_t(regime));
}

const char* nameOf(Basement basement) {
    static constexpr const char* Names[] = {"primordial quench crust", "oceanic basalt", "volcanic arc", "craton", "collision belt"};
    return named(Names, size_t(basement));
}

const char* nameOf(SurfaceOrigin origin) {
    static constexpr const char* Names[] = {"exposed rock", "weathered in place", "river deposit", "lake floor", "sea floor", "shore", "slope debris",
                                            "slope wash", "not explained by the simulation"};
    return named(Names, size_t(origin));
}

Entries describe(const InspectRecord& r) {
    Entries out;
    out.push_back({"Position", formatText("x %.1f m, z %.1f m", r.position.x, r.position.z)});
    if (r.found) {
        out.push_back({"Why / Surface material", nameOf(r.provenance.material)});
        out.push_back({"Why / Origin", nameOf(r.provenance.origin)});
        out.push_back({"Why / Explanation", r.provenance.reason});

        out.push_back({"Surface / Ground height", formatText("%.2f m", r.surface.ground)});
        out.push_back({"Surface / Original top of the layering", formatText("%.2f m", r.surface.datum)});
        out.push_back({"Surface / Cut into the layering", formatText("%.2f m", r.surface.incision)});
        out.push_back({"Surface / Slope", formatText("%.1f deg", r.surface.slope)});
        out.push_back({"Surface / Falls towards", formatText("%.0f deg from north", r.surface.aspect)});
        out.push_back({"Surface / Answer from level", formatText("%d (%.0f m cells)", int(r.surface.level), specOf(r.surface.level).cell)});

        out.push_back({"Rock / Exposed bed", nameOf(r.rock.bed)});
        if (r.rock.modelBed != r.rock.bed) out.push_back({"Rock / Bed the erosion worked on", nameOf(r.rock.modelBed)});
        out.push_back({"Rock / Unit", r.rock.unit == BasementUnit ? "basement" : r.rock.unit == OlderCover ? "cover older than the dated record"
                                                                                                           : formatText("strata slot %d", r.rock.unit)});
        if (r.rock.unit >= 0) out.push_back({"Rock / Unit laid down", years(r.rock.depositedAgo) + " ago"});
        if (r.rock.unit != BasementUnit) {
            const Facies& f = r.rock.facies;
            out.push_back({"Rock / Unit facies", formatText("gravel %.0f %%, sand %.0f %%, mud %.0f %%, carbonate %.0f %%, volcanic %.0f %%, evaporite %.0f %%",
                                                      f.gravel / 2.55, f.sand / 2.55, f.mud / 2.55, f.carbonate / 2.55, f.volcanic / 2.55, f.evaporite / 2.55)});
        }
        if (r.rock.unit >= 0)
            out.push_back({"Rock / Place in the sequence", r.rock.deformedAgo >= r.crust.lidAge ? std::string("the province was never deformed: the layering lies as it was laid")
                                                               : (r.rock.aboveUnconformity ? "above the unconformity: laid flat after the last deformation, " : "below the unconformity: folded in the last deformation, ") +
                                                                     years(r.rock.deformedAgo) + " ago"});
        out.push_back({"Rock / Basement", nameOf(r.rock.basement)});
        out.push_back({"Rock / Sedimentary cover before erosion", formatText("%.0f m", r.rock.coverTotal)});
        out.push_back({"Rock / Joint spacing", r.rock.jointSpacing > 0 ? formatText("%.1f m", r.rock.jointSpacing) : std::string("unjointed")});
        out.push_back({"Rock / Bedding", formatText("%.2f (0 massive, 1 thinly layered)", r.rock.bedding)});
        for (const InspectBed& bed : r.rock.column)
            out.push_back({formatText("Column / %.1f to %.1f m down", bed.top, bed.bottom), nameOf(bed.material)});

        out.push_back({"Cover / Thickness", formatText("%.2f m", r.cover.thickness)});
        out.push_back({"Cover / Coarse, sand, fines", formatText("%.2f m, %.2f m, %.2f m", r.cover.coarse, r.cover.sand, r.cover.fines)});
        describeRates(out, formatText("Rates at level %d", int(r.rates.level)).c_str(), r.rates);
        describeRates(out, "Rates of the region", r.regional);
        out.push_back({"Rates of the region / Tectonic part of uplift now", rate(r.tectonicNow)});
        out.push_back({"Rates of the region / Isostatic part of uplift now", rate(r.isostaticNow)});

        out.push_back({"Water / Precipitation", formatText("%.2f m/yr", r.water.rain)});
        out.push_back({"Water / Runoff", formatText("%.2f m/yr", r.water.runoff)});
        out.push_back({"Water / Discharge through the cell", formatText("%.3g m3/yr", double(r.water.discharge))});
        out.push_back({"Water / Standing water", r.water.standing ? formatText("surface at %.2f m, %.2f m deep", r.water.level, r.water.depth) : std::string("none")});
        out.push_back({"Water / Sea level", formatText("%.1f m", r.water.seaLevel)});
        out.push_back({"Heat / From below", formatText("%.0f K/km, %.0f %% of it from melt", 1000.0 * r.heat.gradient, 100.0 * r.heat.magmatic)});
        out.push_back({"Heat / Melt last rose here", r.heat.magmatic <= 0 ? std::string("no melt is cooling under the point")
                                                                          : years(r.heat.sinceMelt) + formatText(" ago (closeness to its centre %.2f)", r.heat.centre)});
        out.push_back({"Heat / Rock 100 m down", formatText("%.0f C", r.heat.at100m)});
        if (r.veins.veining > 0) out.push_back({"Veins / Sealed fractures", formatText("%.0f %% of the sets, by fluids that moved ", 100.0 * r.veins.veining) + years(r.veins.sealedAgo) + " ago"});
        if (r.veins.pipeRadius > 0) out.push_back({"Veins / Pipe", formatText("%.0f m across, punched through ", 2.0 * r.veins.pipeRadius) + years(r.veins.pipeAgo) + " ago"});
        if (r.veins.cutting != Material::Air) out.push_back({"Veins / Cutting the bed here", nameOf(r.veins.cutting)});
        out.push_back({"Air / Annual mean temperature", formatText("%.1f C", r.air.warmth)});
        out.push_back({"Air / Prevailing wind", vector(r.air.wind, 1.0, "m/s")});
    }
    const InspectCrust& c = r.crust;
    out.push_back({"Crust / Planet-scale surface", formatText("%.0f m (sea level %.0f m)", c.elevation, c.seaLevel)});
    out.push_back({"Crust / Thickness", formatText("%.1f km", c.thickness / 1000)});
    out.push_back({"Crust / Rock above the isostatic datum", formatText("%.0f m per unit area", c.stock)});
    out.push_back({"Crust / Dated sedimentary cover", formatText("%.0f m", c.cover)});
    out.push_back({"Crust / Age of the lid", years(c.lidAge)});
    out.push_back({"Crust / Since last deformation", years(c.sinceDeformation)});
    out.push_back({"Crust / Since last collision", c.sinceOrogeny < 0 ? std::string("never stacked") : years(c.sinceOrogeny)});
    out.push_back({"Crust / Stripped since that collision", formatText("%.0f m", c.exhumed)});
    out.push_back({"Crust / Eroded and deposited since it formed", formatText("%.0f m, %.0f m", c.erodedTotal, c.depositedTotal)});
    out.push_back({"Crust / Planet-scale erosion and deposition now", rate(c.erosionRate) + ", " + rate(c.depositionRate)});
    out.push_back({"Crust / Travelled since it formed", vector(c.drifted, 0.001, "km")});
    out.push_back({"Plate / Identity", formatText("plate %d", c.plate)});
    out.push_back({"Plate / Velocity", vector(c.velocity, 1000, "mm/yr")});
    out.push_back({"Plate / Nearest contact", c.neighbor == c.plate ? std::string("none within reach: plate interior")
                                                                    : formatText("with plate %d, %.0f km away", c.neighbor, c.contactDistance / 1000)});
    out.push_back({"Plate / Motion at the contact", formatText("%s: compression %.1f, extension %.1f, shear %.1f mm/yr", nameOf(c.regime), 1000 * c.compression,
                                                         1000 * c.extension, 1000 * c.shear)});
    out.push_back({"Plate / Activity of the arrangement", formatText("%.2f (0 reorganising, 1 steady)", c.activity)});
    return out;
}

} // namespace derivean::sim

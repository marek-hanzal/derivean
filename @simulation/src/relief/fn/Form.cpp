#include "relief/type/Form.hpp"

#include "ground/fn/Partings.hpp"
#include "planet/constant/Scale.hpp"
#include "relief/type/City.hpp"

namespace derivean::sim {
namespace {

constexpr float SoilDepth = 0.4f;       // m of the fine cover that is living soil
constexpr float BareCover = 0.6f;       // m of cover under which rock still shows its beds
constexpr double FaceFoot = 2.0;        // m down the fall line where a face must be bare too

// How far a bed stands back from a steep face, in metres; negative beds stand out.
float recessOf(const RockTraits& rock) {
    return float(mix(2.6, -0.5, smoothstep(0.6, 3.4, rock.stableTan)));
}

constexpr float ThinRoof = 0.3f;        // m of rock over a void that does not hold
constexpr int MostSpans = 6;            // voids one vertical line is taken to cross

// Loose cover in the order it lies: coarse on the rock, sand on it, fines on top.
Material coverAt(const GroundPoint& ground, float aboveRock, float belowGround) {
    if (aboveRock < ground.cover[Coarse]) return ground.coarse;
    if (aboveRock < ground.cover[Coarse] + ground.cover[Sand]) return Material::Sand;
    return belowGround < SoilDepth && !ground.bare ? Material::Soil : ground.fine;
}

// The rock mass of a bed: from the list of the column's layering node if it has met the bed before.
RockMass massFor(const FormSetting& setting, const Bed& bed, double laidDown) {
    if (!setting.masses) return massOf(setting.seed, setting.strata, bed, laidDown);
    const float thickness = canonicalThickness(bed);
    for (const BedMass& known : *setting.masses)
        if (known.rock == bed.rock && known.thickness == thickness && known.laidDown == laidDown) return known.mass;
    setting.masses->push_back({bed.rock, thickness, laidDown, massOf(setting.seed, *setting.node, bed, laidDown)});
    return setting.masses->back().mass;
}

} // namespace

float exposureAt(TileReader& reader, Vec2 position) {
    float low = 1e30f, high = -1e30f;
    for (const Vec2 offset : {Vec2{0, 0}, Vec2{ExposureReach, 0}, Vec2{-ExposureReach, 0}, Vec2{0, ExposureReach}, Vec2{0, -ExposureReach}}) {
        SurfaceSample sample;
        if (!readSurface(reader, position + offset, sample)) continue;
        low = std::min(low, sample.rock);
        high = std::max(high, sample.rock);
    }
    return high > low ? float(smoothstep(0.22, 0.85, (high - low) / (2 * ExposureReach))) : 0.0f;
}

void formColumn(const FormSetting& setting, const GroundLattice& lattice, float exposure, const CaveField* caves, double x, double z,
                double bottom, double step, int count, Material* out, size_t stride) {
    const GroundPoint ground = lattice.at(x, z);
    const Vec2 material = setting.strata.material + (Vec2{x, z} - setting.strataCentre);
    const float fold = foldAt(setting.seed, setting.strata, material);
    const float loose = ground.ground - ground.rock;
    const float top = ground.ground + ground.crust;

    // A void that comes within a thin roof of the rock surface is open: the cover over it has
    // fallen in and lies on its floor, `fallen` thick from `floor` up.
    CaveSpan spans[MostSpans];
    const int voids = caves ? caveSpans(*caves, {x, z}, spans, MostSpans) : 0;
    float floor = 0, fallen = 0;
    bool open = false;
    for (int v = 0; v < voids; ++v) {
        if (spans[v].top < ground.rock - ThinRoof || spans[v].bottom >= top) continue;
        open = true;
        floor = spans[v].bottom;
        fallen = std::min(top - ground.rock, std::max(0.0f, std::min(spans[v].top, ground.rock) - floor));
        break;
    }

    const float middle = std::max(0.0f, ground.datum - float(bottom + 0.5 * count * step));
    const bool cut = (setting.ore.veining != 0 || setting.ore.pipeRadius != 0) &&
                     nearCuttingRock(setting.seed, setting.ore, material, middle, float(0.35 * 0.5 * count * step + 1.0));
    const float bare = float(1.0 - smoothstep(0.5 * BareCover, BareCover, loose));
    // Beds stand back from a face that is open to the weather: bare here and bare at its foot. A
    // steep rock surface buried in its own waste has no face.
    const float foot = lattice.looseAt(x + ground.fall.x * FaceFoot, z + ground.fall.z * FaceFoot);
    const float steep = float(smoothstep(0.7, 1.6, ground.slope)) * bare * float(1.0 - smoothstep(0.5 * BareCover, BareCover, foot));
    const float lie = lattice.lieAt(x, z);

    // A rock city needs massive, widely jointed rock at the surface. Its slots go down through that
    // rock only: they stop at the first bed below that is of another kind.
    const auto massive = [](const RockTraits& rock) { return float(smoothstep(3.5, 5.5, rock.jointSpacing)) * (rock.bedding < 0.7f ? 1.0f : 0.0f); };
    RockCity city;
    city.floor = ground.rock;
    {
        float depth = std::max(0.0f, ground.datum - ground.rock) + 0.1f;
        const StrataUnit unit = unitAt(setting.strata, depth, fold);
        Bed top = bedAt(setting.seed, setting.strata, unit, depth, material);
        city.strength = massive(traitsOf(top.rock)) * exposure * bare;
        if (city.strength > 0.05f) {
            city.seed = setting.seed;
            city.mass = massFor(setting, top, unit.laidDown);
            chooseStreets(city);
        }
        for (int guard = 0; city.strength > 0.05f && guard < 6; ++guard) {
            city.floor = ground.datum - top.bottom;
            depth = top.bottom + 0.05f;
            top = bedAt(setting.seed, setting.strata, unitAt(setting.strata, depth, fold), depth, material);
            if (massive(traitsOf(top.rock)) < 0.5f) break;
        }
    }

    // Where the bed at the surface stands back from the face, the thin cover on it lies that much lower.
    float sunk = 0;
    if (steep > 0.02f && loose > 0) {
        const float depth = std::max(0.0f, ground.datum - ground.rock) + 0.05f;
        const float recess = recessOf(traitsOf(bedAt(setting.seed, setting.strata, unitAt(setting.strata, depth, fold), depth, material).rock)) * steep;
        sunk = std::max(0.0f, lie - lattice.lieAt(x + ground.fall.x * recess, z + ground.fall.z * recess));
    }

    Bed bed{1, 0, Material::Bedrock};
    Parting layer{1, 0};
    bool seams = false;
    double laidDown = 0;
    float face = ground.rock, partingFace = ground.rock;
    for (int k = count - 1; k >= 0; --k) {
        const float y = float(bottom + (k + 0.5) * step);
        Material& result = out[size_t(k) * stride];
        if (y < WorldFloor || y >= top + 1.0f) result = Material::Air;
        else if (y < MeltTop) result = Material::Magma;
        else if (y < MantleTop) result = Material::Peridotite;
        else {
            const float depth = std::max(0.0f, ground.datum - y);
            if (depth < bed.top || depth >= bed.bottom) {
                const StrataUnit unit = unitAt(setting.strata, depth, fold);
                bed = bedAt(setting.seed, setting.strata, unit, depth, material);
                laidDown = unit.laidDown;
                layer = {1, 0};
                seams = hasSeams(bed.rock);
                const RockTraits& rock = traitsOf(bed.rock);
                // The face of this bed: the site surface moved along the fall line by the bed's
                // recess. The base of a bed is a parting that weathers a little deeper.
                face = partingFace = ground.rock;
                if (steep > 0.02f) {
                    const float recess = recessOf(rock) * steep, parting = recess + 0.7f * steep;
                    face = ground.rock + lattice.lieAt(x + ground.fall.x * recess, z + ground.fall.z * recess) - lie;
                    partingFace = ground.rock + lattice.lieAt(x + ground.fall.x * parting, z + ground.fall.z * parting) - lie;
                }
            }
            bool solid = y < (depth > bed.bottom - 0.35f ? partingFace : face);
            bool sand = false;
            if (solid && city.strength > 0.05f && y >= city.floor) solid = !cityOpens(city, material, y, depth, ground.rock, sand);
            bool hollow = false;
            for (int v = 0; v < voids && !hollow; ++v) hollow = y >= spans[v].bottom && y < spans[v].top;
            if (hollow || (open && y >= ground.rock)) {
                result = open && y >= floor && y < floor + fallen ? coverAt(ground, y - floor, floor + fallen - y) : Material::Air;
            } else if (solid) {
                result = bed.rock;
                if (seams) {
                    if (depth < layer.top || depth >= layer.bottom) layer = partingAt(setting.seed, bed, depth);
                    result = layer.rock;
                }
                if (cut) {
                    const Material body = cuttingRock(setting.seed, setting.ore, material, depth, laidDown);
                    if (body != Material::Air) result = body;
                }
            } else if (sand) result = Material::Sand;
            else if (y + sunk < ground.ground && y + sunk >= ground.rock) result = coverAt(ground, y + sunk - ground.rock, ground.ground - y - sunk);
            else if (y + sunk < top && y + sunk >= ground.ground) result = ground.crustKind == Crust::Travertine ? Material::Travertine : Material::Sinter;
            else result = Material::Air;
        }
    }
}

} // namespace derivean::sim

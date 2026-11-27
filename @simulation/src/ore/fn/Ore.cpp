#include "ore/type/Ore.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t OreStream = 0x4f524553;
constexpr double AureoleWidth = 0.07;      // pattern units either side of a pluton's margin that its fluids reach
constexpr double DistrictScale = 5200.0;   // m, size of a veined district inside the aureole
constexpr double VeinSpacing = 140.0;      // m between the fractures of one set
constexpr double SetAngle[2] = {0.32, 1.38};   // rad off the regional strike: a main set and its conjugate
constexpr double SetLean[2] = {0.22, -0.31};   // horizontal shift per metre of depth (steep dips)
constexpr float MeltForFluids = 1500.0f;   // m of melt that saturates a province with fluids
constexpr double PipeSpacing = 70e3;
constexpr double PipeShare = 0.22;         // share of lattice cells in old crust that hold a pipe
constexpr double PipeQuiet = 600 * Myr;    // yr without melt or stacking before the crust is cold enough
constexpr float PipeRoot = 2400.0f;        // m below the datum where a pipe has narrowed to its feeder

struct Vein {
    double offset{};      // m from the point to the vein's mid-plane, across the set
    double half{};        // m, half thickness here
    bool present{};
};

struct SetFrame {
    double along, across;   // m; `across` at the datum
};

SetFrame frameOf(const OreField& field, int set, Vec2 material) {
    const double angle = field.strike + SetAngle[set];
    const double c = std::cos(angle), s = std::sin(angle);
    return {material.x * c + material.z * s, -material.x * s + material.z * c};
}

VeinFracture fractureOf(uint64_t seed, const OreField& field, int set, int64_t number) {
    const uint64_t base = stream(seed, OreStream) + uint64_t(set);
    VeinFracture fracture;
    fracture.number = number;
    // A fracture is sealed only while the ground around it is veined enough, so a vein ends where
    // the district thins rather than at a lattice edge.
    fracture.sealed = !(unit(base, number, 1) >= 0.85 * field.veining);
    if (!fracture.sealed) return fracture;
    fracture.centre = (double(number) + 0.15 + 0.70 * unit(base, number, 2)) * VeinSpacing;
    const double widest = 0.2 + 2.4 * std::pow(unit(base, number, 3), 2.2);
    fracture.half = 0.5 * widest;
    return fracture;
}

// The vein of set `set` at a point `across` the set and `depth` below the datum, in the cell of
// `fracture`. A fracture stays inside its lattice cell, so the neighbours never need to be looked at.
Vein veinOf(uint64_t seed, int set, const VeinFracture& fracture, double along, double across, float depth, bool shaped) {
    Vein vein;
    if (!fracture.sealed) return vein;
    vein.offset = across - fracture.centre;
    vein.half = fracture.half;
    vein.present = true;
    if (!shaped || std::abs(vein.offset) > 4.0 + vein.half) return vein;
    // Pinch and swell along strike and down dip; the mid-plane wanders by a few metres.
    const uint64_t own = hash(stream(seed, OreStream) + uint64_t(set), fracture.number, 4);
    vein.offset -= 3.0 * gradientNoise(own, along / 55.0, depth / 70.0);
    vein.half *= std::max(0.0, 0.45 + 1.4 * fractalNoise(own + 1, along / 75.0, depth / 95.0, 2));
    return vein;
}

double pipeReach(const OreField& field, Vec2 material, float depth) {
    if (field.pipeRadius <= 0) return -1;
    const Vec2 from = material - field.pipe;
    const double taper = std::clamp(1.0 - double(depth) / PipeRoot, 0.12, 1.0);
    const double outline = 1.0 + 0.2 * gradientNoise(field.pipeId, 2.5 * std::atan2(from.z, from.x), depth / 180.0);
    return field.pipeRadius * taper * outline - length(from);
}

} // namespace

OreField oreFieldAt(uint64_t seed, const StrataColumn& column, Vec2 material) {
    OreField field;
    field.strike = column.fabric;
    const uint64_t base = stream(seed, OreStream);
    const bool arc = column.basement == Basement::Arc;
    if (arc || column.basement == Basement::Orogen || column.basement == Basement::Craton) {
        const double aureole = 1.0 - smoothstep(0.0, AureoleWidth, std::abs(plutonMargin(seed, column, material)));
        if (aureole > 0) {
            const double district = smoothstep(0.05, 0.45, fractalNoise(base + 7, material.x / DistrictScale, material.z / DistrictScale, 3));
            // An arc's fluids scale with the melt it received; the plutons of collision belts and
            // old shields made theirs when the crust was stacked and heated.
            const double fluids = arc ? std::min(1.0f, column.melt / MeltForFluids) : 0.55;
            field.veining = float(aureole * district * fluids);
            field.emplaced = arc ? std::max(double(column.meltTime), CrustStable) : std::max(double(column.orogenTime), CrustStable);
        }
    }
    // Pipes need thick, cold crust that nothing has disturbed for a long time.
    const double disturbed = std::max(double(column.meltTime), double(column.orogenTime));
    if (column.basement == Basement::Craton && column.now - disturbed > PipeQuiet) {
        const CellularSite site = cellularNoise(base + 11, material.x / PipeSpacing, material.z / PipeSpacing, 0.8);
        const uint64_t own = hash(base + 12, site.cellX, site.cellZ);
        if (unit(own) < PipeShare) {
            field.pipe = site.site * PipeSpacing;
            field.pipeId = own;
            field.pipeRadius = float(ranged(hash(own, 1), 90.0, 260.0));
            const double since = std::max(disturbed, CrustStable);
            field.pipeEmplaced = since + unit(hash(own, 2)) * std::max(0.0, column.now - since);
        }
    }
    return field;
}

Material cuttingRock(uint64_t seed, const OreField& field, Vec2 material, float depth, double laidDown) {
    VeinColumn column{material};
    return cuttingRock(seed, field, column, depth, laidDown);
}

Material cuttingRock(uint64_t seed, const OreField& field, VeinColumn& column, float depth, double laidDown) {
    if (field.pipeRadius > 0 && laidDown < field.pipeEmplaced && pipeReach(field, column.material, depth) > 0) return Material::Kimberlite;
    if (field.veining <= 0 || laidDown >= field.emplaced) return Material::Air;
    if (!column.framed) {
        column.framed = true;
        for (int set = 0; set < 2; ++set) {
            const SetFrame frame = frameOf(field, set, column.material);
            column.along[set] = frame.along, column.across[set] = frame.across;
        }
    }
    for (int set = 0; set < 2; ++set) {
        const double across = column.across[set] - double(depth) * SetLean[set];
        const int64_t number = int64_t(std::floor(across / VeinSpacing));
        if (!column.asked[set] || number != column.last[set].number) {
            column.last[set] = fractureOf(seed, field, set, number);
            column.asked[set] = true;
        }
        const Vein vein = veinOf(seed, set, column.last[set], column.along[set], across, depth, true);
        if (vein.present && std::abs(vein.offset) < vein.half) return Material::OreVein;
    }
    return Material::Air;
}

bool nearCuttingRock(uint64_t seed, const OreField& field, Vec2 material, float depth, float margin) {
    if (field.pipeRadius > 0 && pipeReach(field, material, depth) > -double(margin) - 0.25 * field.pipeRadius) return true;
    if (field.veining <= 0) return false;
    for (int set = 0; set < 2; ++set) {
        const SetFrame frame = frameOf(field, set, material);
        const double across = frame.across - double(depth) * SetLean[set];
        const Vein vein = veinOf(seed, set, fractureOf(seed, field, set, int64_t(std::floor(across / VeinSpacing))), frame.along, across, depth, false);
        if (vein.present && std::abs(vein.offset) < vein.half * 2.2 + 3.0 + margin) return true;
    }
    return false;
}

} // namespace derivean::sim

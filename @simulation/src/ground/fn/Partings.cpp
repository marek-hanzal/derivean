#include "ground/fn/Partings.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t PartingStream = 0x50415254;
constexpr float SeamStrength = 0.25f;
constexpr float Survives = 0.25f;        // share of a released block's edge that the median piece keeps after the fall
constexpr float LargestPiece = 1.2f;     // m, median piece of the most massive rock

Material seamRockOf(Material rock) {
    const MaterialDefinition& d = definitionOf(rock);
    if (d.origin != Origin::Sedimentary || d.bedding < 0.3f || rock == Material::Shale || rock == Material::Mudstone) return rock;
    return rock == Material::Chalk || rock == Material::Limestone ? Material::Mudstone : Material::Shale;
}

} // namespace

bool hasSeams(Material rock) { return seamRockOf(rock) != rock; }

float partingPitch(const RockTraits& rock) { return 3.0f * std::exp(-3.2f * rock.bedding); }

Parting partingAt(uint64_t seed, const Bed& bed, float depth) {
    const RockTraits& rock = traitsOf(bed.rock);
    // Layers of one rock share their sequence: it is counted from the top of each bed.
    const uint64_t base = stream(seed, PartingStream) + uint64_t(bed.rock);
    const float pitch = partingPitch(rock);
    const float within = std::max(0.0f, depth - bed.top);
    const auto edge = [&](int64_t n) { return n <= 0 ? 0.0f : (float(n) + 0.6f * (float(unit(base, n)) - 0.5f)) * pitch; };
    int64_t number = int64_t(within / pitch);
    if (within < edge(number)) --number;
    else if (within >= edge(number + 1)) ++number;

    Parting parting;
    parting.rock = bed.rock;
    parting.top = bed.top + edge(number);
    parting.bottom = std::min(bed.bottom, bed.top + edge(number + 1));
    parting.strength = std::exp((0.35f + 0.9f * rock.bedding) * float(2.0 * unit(base, number, 1) - 1.0));
    const Material weak = seamRockOf(bed.rock);
    if (weak != bed.rock && unit(base, number, 2) < 0.2 + 0.3 * rock.bedding) {
        const float seam = parting.bottom - (0.14f + 0.12f * float(unit(base, number, 3)));
        if (depth >= seam) return {seam, parting.bottom, SeamStrength, weak, true};
        parting.bottom = std::max(parting.top + 0.01f, seam);
    }
    return parting;
}

RockMass massOf(uint64_t seed, const StrataColumn& column, const Bed& bed, double laidDown) {
    return rockMassAt(seed, column, Bed{0, canonicalThickness(bed), bed.rock, bed.thickness}, laidDown, column.material);
}

RockTraits partingTraits(const Parting& parting, float jointWeak, float frost) {
    RockTraits traits = traitsOf(parting.rock);
    const float split = 0.7f * std::min(1.0f, frost / 0.2f);
    traits.coarse += split * (traits.sand + traits.fines);
    traits.sand *= 1.0f - split;
    traits.fines *= 1.0f - split;
    const float resistance = parting.strength / (1.0f + 3.0f * jointWeak);
    traits.erodibility /= resistance;
    traits.weathering /= resistance;
    traits.stableTan *= std::clamp(0.5f + 0.5f * resistance, 0.6f, 1.25f);
    return traits;
}

BlockHabit blockHabitOf(const RockMass& mass, const RockTraits& rock) {
    BlockHabit habit;
    habit.size = std::clamp(Survives * mass.block, 0.04f, LargestPiece);
    habit.flatness = std::clamp(1.1f - 0.7f * rock.bedding, 0.35f, 1.0f);
    return habit;
}

BlockHabit blockHabitOf(const RockTraits& rock) {
    const float joints = std::max(0.05f, rock.jointSpacing), layers = partingPitch(rock);
    BlockHabit habit;
    habit.size = std::clamp(0.45f * std::min(joints, 2.0f * layers), 0.04f, LargestPiece);
    habit.flatness = std::clamp(1.1f - 0.7f * rock.bedding, 0.35f, 1.0f);
    return habit;
}

} // namespace derivean::sim

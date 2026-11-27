#include "rockmass/type/RockMass.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t DomainStream = 0x524d4153;
constexpr double DomainSize = 22e3;            // m across a structural domain
constexpr double FaultWidth = 0.045;           // lattice units of crushed rock either side of a bounding fault
constexpr double StrikeStep = 0.2617993877991494;   // rad; the regional strike is taken in steps so planes stay planes
constexpr double DipBase = 150.0;              // m over which the attitude of the layering is measured
constexpr float SegmentLength = 6.0f;          // spacings a joint runs before it may end
constexpr float CorridorShare = 0.04f;         // share of its spacing a plane's broken rock is wide
constexpr float MassShare = 0.03f;             // share of intact cohesion a jointed mass keeps where a material states none
constexpr float Unjointed = 60.0f;             // m between the rare breaks of rock that has no joints of its own

struct Frame {
    double along, across;
};

Frame frameOf(const JointSet& set, Vec2 material, float depth) {
    const double c = std::cos(set.strike), s = std::sin(set.strike);
    return {material.x * c + material.z * s, -material.x * s + material.z * c - double(depth) * set.lean};
}

// jointDistance at a point already in the frame of the set.
float distanceIn(uint64_t seed, const RockMass& mass, int set, int order, const Frame& frame, float depth) {
    const JointSet& joints = mass.sets[set];
    float spacing = joints.spacing;
    for (int k = 0; k < order; ++k) spacing *= OrderStep;
    const uint64_t base = hash(stream(seed, DomainStream) + 17, int64_t(mass.domain), set * JointOrders + order);
    // Wider orders are broken through more completely: a zone runs on where single joints end.
    const double through = 1.0 - (1.0 - joints.persistence) * std::pow(0.45, order);
    const int64_t nearest = int64_t(std::floor(frame.across / spacing));
    float best = 1e9f;
    for (int64_t plane = nearest - 1; plane <= nearest + 1; ++plane) {
        const int64_t segment = int64_t(std::floor(frame.along / (spacing * SegmentLength) + unit(base, plane, 1)));
        if (unit(base, plane, segment + 2) >= through) continue;
        const double wander = 0.08 * spacing * gradientNoise(hash(base, plane, 0), frame.along / (3.0 * spacing), depth / (4.0 * spacing));
        const double at = (double(plane) + 0.2 + 0.6 * unit(base, plane, 0)) * spacing + wander;
        best = std::min(best, float(std::abs(frame.across - at)));
    }
    // The three planes looked at are every plane within one spacing, so the answer is whole up to there.
    return best <= spacing ? best : 1e9f;
}

} // namespace

RockMass rockMassAt(uint64_t seed, const StrataColumn& column, const Bed& bed, double laidDown, Vec2 material) {
    const uint64_t base = stream(seed, DomainStream);
    const MaterialDefinition& rock = definitionOf(bed.rock);
    RockMass mass;
    const CellularSite site = cellularNoise(base, material.x / DomainSize, material.z / DomainSize, 0.9);
    const int64_t step = std::llround(column.fabric / StrikeStep);
    mass.domain = hash(base, site.cellX, site.cellZ, step);
    mass.crush = float(1.0 - smoothstep(0.0, FaultWidth, site.second - site.nearest));

    // Layering takes the fold's slope where it was folded and only the gentle warping where it was not.
    const bool folded = laidDown < column.deformed;
    const float share = folded || column.foldSpan <= QuietFoldSpan ? 1.0f : QuietFoldSpan / column.foldSpan;
    const double slopeX = share * (foldAt(seed, column, material + Vec2{DipBase, 0}) - foldAt(seed, column, material - Vec2{DipBase, 0})) / (2 * DipBase);
    const double slopeZ = share * (foldAt(seed, column, material + Vec2{0, DipBase}) - foldAt(seed, column, material - Vec2{0, DipBase})) / (2 * DipBase);
    mass.dip = float(std::atan(std::sqrt(slopeX * slopeX + slopeZ * slopeZ)));
    mass.dipDirection = std::atan2(-slopeZ, -slopeX);

    // Joints of layered rock stand about as far apart as its beds are thick; massive rock keeps its own.
    // Readers reach a bed through different depths and clippings; the planes they get must not
    // depend on that, so the thickness that sets the spacing is the bed's own, in whole quarter metres.
    const float whole = bed.thickness > 0 ? bed.thickness : bed.bottom - bed.top;
    const float thickness = std::clamp(std::round(whole * 4.0f) * 0.25f, 1.0f, 40.0f);
    const float own = rock.jointSpacing > 0 ? rock.jointSpacing : Unjointed;
    const float layered = own * float(mix(1.0, thickness / 12.0, std::clamp(rock.bedding, 0.0f, 1.0f)));
    const double strike = double(step) * StrikeStep + ranged(hash(mass.domain, 1), -0.17, 0.17);
    for (int set = 0; set < JointSets; ++set) {
        JointSet& joints = mass.sets[set];
        joints.strike = strike + (set ? 1.5707963 + ranged(hash(mass.domain, 2), -0.2, 0.2) : 0.0);
        joints.lean = float(ranged(hash(mass.domain, 3 + set), -1.0, 1.0)) * (folded ? 0.3f : 0.1f);
        joints.spacing = std::max(0.05f, layered * float(ranged(hash(mass.domain, 5 + set), 0.6, 1.7)) * (1.0f - 0.7f * mass.crush));
        joints.persistence = rock.jointSpacing > 0 ? float(ranged(hash(mass.domain, 7 + set), 0.55, 0.9)) : 0.15f;
    }
    mass.parting = rock.bedding > 0.3f ? thickness * (1.3f - rock.bedding) : 2.0f * std::max(mass.sets[0].spacing, mass.sets[1].spacing);
    mass.block = std::cbrt(mass.sets[0].spacing * mass.sets[1].spacing * mass.parting);
    // The material's mass strength is that of its usual jointing; wider joints here leave more of
    // the intact rock to hold a face, closer ones less.
    const float narrow = std::min(mass.sets[0].spacing, mass.sets[1].spacing);
    const float jointing = std::clamp(std::pow(narrow / own, 0.6f), 0.3f, 3.0f) * (1.0f - 0.8f * mass.crush);
    mass.cohesion = (rock.massCohesion > 0 ? rock.massCohesion : rock.cohesion * MassShare) * jointing;
    const float angle = rock.massFriction > 0 ? rock.massFriction : std::clamp(rock.frictionAngle, 10.0f, 45.0f);
    mass.friction = std::tan(angle * 0.0174533f) * (1.0f - 0.25f * mass.crush);
    mass.openDepth = (6.0f + 1.2f * 0.5f * (mass.sets[0].spacing + mass.sets[1].spacing)) * (1.0f + mass.crush);
    return mass;
}

float jointDistance(uint64_t seed, const RockMass& mass, int set, int order, Vec2 material, float depth) {
    return distanceIn(seed, mass, set, order, frameOf(mass.sets[set], material, depth), depth);
}

float columnJointDistance(uint64_t seed, uint64_t body, Vec2 across, float width) {
    // Rows of the lattice are offset by half a cell so its cells come out six-sided, as columns do.
    const double row = across.z / (0.866 * width);
    const double shifted = across.x / width + 0.5 * (int64_t(std::floor(row)) & 1);
    const CellularSite site = cellularNoise(hash(stream(seed, DomainStream) + 29, int64_t(body)), shifted, row, 0.3);
    return float(0.5 * (site.second - site.nearest) * width);
}

float fractureShare(uint64_t seed, const RockMass& mass, Vec2 material, float depth, float width) {
    double intact = 1.0 - mass.crush;
    for (int set = 0; set < JointSets; ++set) {
        // Orders only widen, so where the widest is too fine to see, every order is.
        static_assert(OrderStep >= 1.0f);
        float spacing = mass.sets[set].spacing, widest = spacing;
        for (int k = 1; k < JointOrders; ++k) widest *= OrderStep;
        if (widest < 3.0f * width) continue;
        const Frame frame = frameOf(mass.sets[set], material, depth);
        for (int order = 0; order < JointOrders; ++order, spacing *= OrderStep) {
            if (spacing < 3.0f * width) continue;
            const float half = 0.5f * CorridorShare * spacing * float(order + 1);
            const float covered = std::clamp((half + 0.5f * width - distanceIn(seed, mass, set, order, frame, depth)) / width, 0.0f, 1.0f);
            intact *= 1.0 - covered * std::min(1.0f, 2.0f * half / width + 0.25f);
        }
    }
    return float(1.0 - intact);
}

} // namespace derivean::sim

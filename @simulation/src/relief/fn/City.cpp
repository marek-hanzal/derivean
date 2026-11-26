#include "relief/type/City.hpp"

#include "core/fn/Hash.hpp"
#include "core/fn/Noise.hpp"

namespace derivean::sim {
namespace {

constexpr float SlotSand = 0.7f;      // m of sand lying on the floor of a slot
constexpr double ArchShare = 0.10;    // share of fins between slots that are cut through

struct Street {
    float spacing{};
    double along{}, across{};   // m in the set's frame: along its strike, and across it at this depth
};

Street streetOf(const RockCity& city, int set, Vec2 material, float depth) {
    const JointSet& joints = city.mass.sets[set];
    Street street;
    street.spacing = joints.spacing;
    for (int k = 0; k < city.order[set]; ++k) street.spacing *= OrderStep;
    const double c = std::cos(joints.strike), s = std::sin(joints.strike);
    street.along = material.x * c + material.z * s;
    street.across = -material.x * s + material.z * c - double(depth) * joints.lean;
    return street;
}

} // namespace

void chooseStreets(RockCity& city) {
    for (int set = 0; set < JointSets; ++set) {
        float spacing = city.mass.sets[set].spacing;
        city.order[set] = 0;
        while (city.order[set] < JointOrders && spacing < StreetSpacing) spacing *= OrderStep, ++city.order[set];
    }
}

bool cityOpens(const RockCity& city, Vec2 material, float y, float depth, float rockTop, bool& sand) {
    bool open = false;
    const float gape = city.strength;
    const uint64_t vary = hash(city.mass.domain, 0x43495459);
    for (int set = 0; set < JointSets; ++set) {
        if (city.order[set] >= JointOrders) continue;
        const Street street = streetOf(city, set, material, depth);
        // A street is wider along some of its length and deeper along some than along the rest.
        const double place = street.along / (4.0 * street.spacing), row = std::floor(street.across / street.spacing + 0.5);
        const float wide = float(0.5 + 0.5 * gradientNoise(vary + uint64_t(set), place, row * 0.37));
        const float deep = float(0.5 + 0.5 * gradientNoise(vary + 7 + uint64_t(set), place * 0.6, row * 0.53));
        const float halfWidth = gape * (0.5f + 1.7f * wide), cut = gape * (8.0f + 30.0f * deep);
        const float height = y - std::max(city.floor, rockTop - cut);
        if (height < 0) continue;
        const float distance = jointDistance(city.seed, city.mass, set, city.order[set], material, depth);
        // Slots widen a little upwards; the tops of the towers beside them are rounded.
        if (distance < halfWidth * (1.0f + 0.3f * height / std::max(1.0f, cut))) {
            open = true;
            sand = sand || height < SlotSand * gape;
        } else if (y > rockTop - 1.4f * gape * std::exp(-(distance - halfWidth) / 1.6f)) {
            open = true;
        }
    }
    if (open || city.order[0] >= JointOrders || city.order[1] >= JointOrders) return open;
    // A fin of the first set, beside a slot that is open, cut through between two streets of the second.
    const Street fin = streetOf(city, 0, material, depth), cross = streetOf(city, 1, material, depth);
    const int64_t wall = int64_t(std::floor(fin.across / fin.spacing)), bay = int64_t(std::floor(fin.along / cross.spacing));
    if (unit(hash(vary, wall, bay, 5)) >= ArchShare || jointDistance(city.seed, city.mass, 0, city.order[0], material, depth) > fin.spacing) return false;
    const float cut = gape * (8.0f + 30.0f * float(unit(hash(vary, wall, 4))));
    const float rise = std::min(0.6f * cut, 9.0f), height = y - std::max(city.floor, rockTop - cut);
    const float along = float(fin.along - (double(bay) + 0.5) * cross.spacing), reach = 0.28f * cross.spacing;
    if (height < 0 || height >= rise || std::abs(along) >= reach * std::sqrt(1.0f - (height / rise) * (height / rise))) return false;
    sand = sand || height < 0.4f;
    return true;
}

} // namespace derivean::sim

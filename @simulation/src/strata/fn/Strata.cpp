#include "strata/type/Strata.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

std::array<double, FaciesParts> parts(Facies f) {
    return {double(f.gravel), double(f.sand), double(f.mud), double(f.carbonate), double(f.volcanic), double(f.evaporite)};
}

Facies packed(const std::array<double, FaciesParts>& values) {
    double total = 0;
    for (double value : values) total += value;
    if (total <= 0) return Facies{0, 128, 127, 0, 0, 0};
    std::array<uint8_t, FaciesParts> bytes{};
    int assigned = 0, largest = 0;
    for (int i = 0; i < FaciesParts; ++i) {
        bytes[size_t(i)] = uint8_t(std::floor(values[size_t(i)] / total * 255.0));
        assigned += bytes[size_t(i)];
        if (values[size_t(i)] > values[size_t(largest)]) largest = i;
    }
    bytes[size_t(largest)] = uint8_t(bytes[size_t(largest)] + (255 - assigned));
    return {bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]};
}

// Picks one member of an interbedded sequence in proportion to its share. Soft members are placed
// first so a low `bed` value selects the recessive partings of a mostly resistant formation.
int member(Facies facies, double bed) {
    static constexpr int order[FaciesParts] = {5, 2, 4, 3, 0, 1};
    const auto share = parts(facies);
    double cursor = bed * 255.0;
    for (int index : order) {
        if (cursor < share[size_t(index)]) return index;
        cursor -= share[size_t(index)];
    }
    return 1;
}

} // namespace

Facies blend(Facies a, double weightA, Facies b, double weightB) {
    const auto first = parts(a), second = parts(b);
    std::array<double, FaciesParts> mixed{};
    for (int i = 0; i < FaciesParts; ++i) mixed[size_t(i)] = first[size_t(i)] * weightA + second[size_t(i)] * weightB;
    return packed(mixed);
}

Material lithified(Facies facies, double bed) {
    switch (member(facies, bed)) {
        case 0: return Material::Conglomerate;
        case 1: return Material::Sandstone;
        case 2: return facies.mud > 150 ? Material::Shale : Material::Mudstone;
        case 3: return facies.carbonate > 200 && facies.mud < 25 ? Material::Chalk : Material::Limestone;
        case 4: return facies.volcanic > 170 ? Material::Basalt : Material::Tuff;
        default: return Material::RockSalt;
    }
}

Material metamorphosed(Facies facies, double bed) {
    switch (member(facies, bed)) {
        case 0:
        case 1: return Material::Quartzite;
        case 2: return Material::Schist;
        case 3: return Material::Marble;
        case 4: return Material::Gneiss;
        default: return Material::Schist;
    }
}

Material unconsolidated(Facies facies, double bed) {
    switch (member(facies, bed)) {
        case 0: return Material::Gravel;
        case 1: return Material::Sand;
        case 2: return facies.mud > 150 ? Material::Clay : Material::Silt;
        case 3: return Material::Silt;
        case 4: return Material::Gravel;
        default: return Material::Silt;
    }
}

float addDeposit(Cover& cover, double time, float thickness, Facies facies) {
    if (thickness <= 0) return 0;
    StrataSlot& slot = cover.slots[size_t(strataSlot(time))];
    slot.facies = blend(slot.facies, slot.thickness, facies, thickness);
    slot.thickness += thickness;
    return thickness;
}

float stripCover(Cover& cover, double time, float thickness) {
    float left = thickness;
    const int newest = strataSlot(time);
    for (int back = 0; back < StrataSlots && left > 0; ++back) {
        StrataSlot& slot = cover.slots[size_t((newest - back + StrataSlots) % StrataSlots)];
        const float taken = std::min(left, slot.thickness);
        slot.thickness -= taken;
        left -= taken;
    }
    const float deep = std::min(left, cover.older);
    cover.older -= deep;
    return thickness - (left - deep);
}

float coverThickness(const Cover& cover) {
    float total = cover.older;
    for (const StrataSlot& slot : cover.slots) total += slot.thickness;
    return total;
}

void retireOldest(Cover& cover, double time) {
    StrataSlot& slot = cover.slots[size_t(strataSlot(time))];
    if (slot.thickness > 0) {
        cover.olderFacies = blend(cover.olderFacies, cover.older, slot.facies, slot.thickness);
        cover.older += slot.thickness;
    }
    slot = StrataSlot{};
}

void buryCover(Cover& cover, double time) {
    const int newest = strataSlot(time);
    float above = 0;
    for (int back = 0; back < StrataSlots; ++back) {
        StrataSlot& slot = cover.slots[size_t((newest - back + StrataSlots) % StrataSlots)];
        if (slot.thickness <= 0) continue;
        const float kept = std::clamp(MetamorphicDepth - above, 0.0f, slot.thickness);
        const float sunk = slot.thickness - kept;
        if (sunk > 0) {
            cover.olderFacies = blend(cover.olderFacies, cover.older, slot.facies, sunk);
            cover.older += sunk;
            slot.thickness = kept;
        }
        above += kept;
    }
    cover.older = std::min(cover.older, OlderLimit);
}

void scaleCover(Cover& cover, float factor) {
    for (StrataSlot& slot : cover.slots) slot.thickness *= factor;
    cover.older *= factor;
}

void stackCover(Cover& onto, const Cover& from) {
    for (int i = 0; i < StrataSlots; ++i) {
        StrataSlot& slot = onto.slots[size_t(i)];
        const StrataSlot& added = from.slots[size_t(i)];
        if (added.thickness <= 0) continue;
        slot.facies = blend(slot.facies, slot.thickness, added.facies, added.thickness);
        slot.thickness += added.thickness;
    }
    if (from.older > 0) {
        onto.olderFacies = blend(onto.olderFacies, onto.older, from.olderFacies, from.older);
        onto.older += from.older;
    }
}

} // namespace derivean::sim

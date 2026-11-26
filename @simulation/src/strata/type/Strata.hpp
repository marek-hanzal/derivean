#pragma once

#include "chronology/constant/Timeline.hpp"
#include "material/constant/Materials.hpp"

#include <array>
#include <cstdint>

namespace derivean::sim {

// Composition of a deposit as parts of 255. Neighbouring columns interpolate these parts, so a
// formation changes facies gradually instead of along the edges of a planet cell.
struct Facies {
    uint8_t gravel{}, sand{}, mud{}, carbonate{}, volcanic{}, evaporite{};
};

inline constexpr int FaciesParts = 6;

// The sedimentary cover is filed by the time it was laid down. Slot k of every column covers the
// same StrataSlotYears, which is what lets two columns with different histories be interpolated.
inline constexpr int StrataSlots = 24;
inline constexpr double StrataSlotYears = 25 * Myr;

struct StrataSlot {
    float thickness{};   // m still preserved
    Facies facies;
};

// What lies under the dated cover. The kind follows from how the column was made and how deep
// erosion has cut into it since.
enum class Basement : uint8_t {
    Primordial,   // mafic quench crust of the cooling body
    Oceanic,      // basalt made at a spreading contact
    Arc,          // andesite and granite added above a sinking plate
    Craton,       // old felsic gneiss and granite
    Orogen,       // rocks squeezed and heated in a collision
};

struct Cover {
    std::array<StrataSlot, StrataSlots> slots;
    float older{};        // m of cover that outlived the slot window; buried deep enough to be metamorphic
    Facies olderFacies;
};

inline int strataSlot(double time) {
    return int(int64_t(time / StrataSlotYears) % StrataSlots);
}

// Absolute time at which a slot was last opened, looking back from `now`.
inline double strataSlotStart(int slot, double now) {
    const int64_t current = int64_t(now / StrataSlotYears);
    const int64_t back = ((current % StrataSlots) - slot + StrataSlots) % StrataSlots;
    return double(current - back) * StrataSlotYears;
}

Facies blend(Facies a, double weightA, Facies b, double weightB);

// The rock a deposit of this composition becomes once buried; `bed` picks among interbedded members.
Material lithified(Facies facies, double bed);

// The same deposit after deep burial and heating.
Material metamorphosed(Facies facies, double bed);

// The loose sediment a deposit of this composition is before it lithifies.
Material unconsolidated(Facies facies, double bed);

float addDeposit(Cover& cover, double time, float thickness, Facies facies);

// Removes up to `thickness` from the youngest strata down and returns what the cover could give.
float stripCover(Cover& cover, double time, float thickness);

float coverThickness(const Cover& cover);
void retireOldest(Cover& cover, double time);

// Deep burial ends a deposit's time as cover: dated strata under more than MetamorphicDepth of
// younger ones join the metamorphic older cover, and older cover beyond OlderLimit is taken up
// by the crystalline basement.
inline constexpr float MetamorphicDepth = 6000.0f;   // m
inline constexpr float OlderLimit = 6000.0f;         // m
void buryCover(Cover& cover, double time);
void scaleCover(Cover& cover, float factor);
void stackCover(Cover& onto, const Cover& from);

} // namespace derivean::sim

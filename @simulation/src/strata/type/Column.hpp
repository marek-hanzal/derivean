#pragma once

#include "core/type/Vec.hpp"
#include "strata/type/Strata.hpp"

namespace derivean::sim {

struct Planet;

// What a process needs to know about the rock it is working on.
struct RockTraits {
    float erodibility{};   // stream power coefficient
    float stableTan{};     // tangent of the slope a 10 m face holds
    float weathering{};    // m/yr bare-surface regolith production
    float solubility{};    // m/yr of chemical lowering per m/yr of runoff
    float abrasion{};      // share of bedload volume lost per metre of travel
    float coarse{}, sand{}, fines{}, solute{};   // shares of the weathered rock by transport class
    float porosity{}, conductivity{};
    float bedding{}, jointSpacing{};
    Material dominant{Material::Bedrock};
};

// Everything under one point of the surface, blended from the four planet columns around it.
// Depths are measured down from the datum: the top the cover had before the current replay eroded it.
struct StrataColumn {
    std::array<float, StrataSlots> thickness{};
    std::array<Facies, StrataSlots> facies{};
    float older{};
    Facies olderFacies;
    Basement basement{Basement::Primordial};
    int newest{};        // slot being filled at `now`
    float fabric{};      // rad, strike of the last deformation
    float foldSpan{};    // m, amplitude with which folding displaces the layering in this province
    double deformed{};   // yr the province was last squeezed or intruded; strata laid down later lie across the folds
    float coverTotal{};  // m, all dated strata plus the older cover
    float lid{1e9f};     // m of solid crust over melt while the body is still cooling; beyond reach afterwards
    float melt{};        // m of crust that melt added to the owning column
    float meltTime{-1};  // yr melt last rose into it; negative if never
    float orogenTime{-1};// yr of its last collision stacking; negative if never
    Vec2 material;       // m, crust-attached coordinates: where this rock was when its column formed
    double now{};        // yr
};

// m, the warping that even undisturbed cover shares; the rest of `foldSpan` only reaches strata older than `deformed`.
inline constexpr float QuietFoldSpan = 45.0f;
// yr without stacking or melt after which a collision belt has worn down and cooled into a shield.
inline constexpr double ShieldAge = 800 * Myr;

inline constexpr int OlderCover = -1;
inline constexpr int BasementUnit = -2;

struct StrataUnit {
    float top{}, bottom{};   // m below the datum
    int slot{BasementUnit};
    Facies facies;
    double laidDown{};       // yr the unit started to accumulate; 0 for basement
};

StrataColumn strataAt(const Planet& planet, Vec2 position);

// The column whose rock a position stands on, and the position in the frame of that crust: where
// the rock was when its column formed. Every pattern attached to the crust is read in this frame.
// It is whole inside one column's ragged footprint and steps where crust of different travel meets;
// drifts are never blended, which would stretch or fold the frame between two columns.
struct CrustColumn;
const CrustColumn& crustOwner(const Planet& planet, Vec2 position);
Vec2 crustFrame(const Planet& planet, Vec2 position);

// `base` with the thicknesses of four columns around a point blended by weight. A slot `base` lacks
// stays absent, since it has no facies of its own to give it. Layering read through this never
// steps where the nearest column changes.
StrataColumn blendThickness(const StrataColumn& base, const StrataColumn* const corner[4], const float weight[4]);
// m by which folding displaces the layering at a point of this column's province.
float foldAt(uint64_t seed, const StrataColumn& column, Vec2 material);

StrataUnit unitAt(const StrataColumn& column, float depth, float fold);

// One bed, pluton or band: what a voxel or a metre-scale process meets at this depth.
struct Bed {
    float top{}, bottom{};   // m below the datum
    Material rock{Material::Bedrock};
    float thickness{};       // m, the whole bed before a unit boundary or a pluton cut it; 0 for a body without bedding
};

// `material` is the crust-attached position of the point itself; a column only knows its own.
Bed bedAt(uint64_t seed, const StrataColumn& column, const StrataUnit& unit, float depth, Vec2 material);

// The whole unit averaged over its members: what a kilometre-scale process meets.
RockTraits unitTraits(uint64_t seed, const StrataColumn& column, const StrataUnit& unit, Vec2 material);

const RockTraits& traitsOf(Material material);

// Signed distance, in units of the seeded pattern, from the margin of the nearest intrusive body of
// the column's province: positive inside the pluton, negative in its host rock. Provinces without
// plutons answer far outside.
double plutonMargin(uint64_t seed, const StrataColumn& column, Vec2 material);

// m below the datum of the roof of the pluton under a point: a dome that stands highest over the
// middle of the body. It cuts every unit older than `deformed`; beyond reach outside a pluton.
float plutonRoof(uint64_t seed, const StrataColumn& column, Vec2 material);

} // namespace derivean::sim

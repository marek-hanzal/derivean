#pragma once

#include "core/type/Field.hpp"
#include "measure/fn/Periodicity.hpp"
#include "measure/type/Distribution.hpp"

namespace derivean::sim {

// A profile repeats when its autocorrelation peak reaches RepeatStrength and at least four marks
// stand RegularSpacing apart or closer in relative scatter. Seeded fractal relief, which has no
// preferred spacing, stays below both on nineteen profiles out of twenty; a row of equal gullies
// with a tenth of their depth in noise passes both (test/LandscapeRegularity.cpp holds the two cases).
inline constexpr double RepeatStrength = 0.6;
inline constexpr double RegularSpacing = 0.25;

inline bool repeats(const Periodicity& found) { return found.strength >= RepeatStrength && found.marks >= 4 && found.spacingCv <= RegularSpacing; }

// A valley with two flanks needs more than four cells from crest to crest. A pattern that repeats
// faster is finer than the grid can carry as a landform and can only come from the solver.
inline constexpr double GridSpacing = 4.0;   // cells

// A notch is a cut one or two cells wide whose sides rise NotchDepth cells' width within two cells:
// steeper than 7° on both sides at the finest scale the grid has. Slopes dissected that sharply at
// every fourth cell or closer are a comb whether or not its teeth are evenly spaced.
inline constexpr double NotchDepth = 0.25;   // of a cell width

// Gullies along contour-parallel lines. The surface is cut into square windows; each window is
// read along three lines across its mean fall line.
struct CombReport {
    int profiles{};
    Distribution strength;    // autocorrelation peak per profile
    double regularShare{};    // share of profiles that repeat
    double gridShare{};       // share that repeat every GridSpacing cells or closer
    double spacing{};         // cells, median trough spacing of the repeating profiles
    double depth{};           // m, median trough depth of the repeating profiles
    Distribution roughness;   // cellRoughness per profile
    double notchedShare{};    // share of profiles with a notch every GridSpacing cells or closer
    double notchSpacing{};    // cells of profile per notch over all profiles; 0 without notches
};

// `water` marks flooded cells (water above ground) and may be null; windows that touch them are
// skipped. `cell` is the cell width in metres; troughs shallower than a five-hundredth of it do
// not count as a pattern.
CombReport combOf(const Field<float>& surface, double cell, const Field<float>* water = nullptr, int window = 64);

// Ridge crests along straight lines on both axes and both diagonals: the teeth of a horizon.
struct SawtoothReport {
    int lines{};             // lines with at least five crests
    Distribution spacingCv;  // scatter of crest spacing per line
    double regularShare{};   // share with spacing scatter ≤ SawSpacing and crest height scatter ≤ SawHeight
    double spacing{};        // cells, median crest spacing of the regular lines
    double height{};         // m, median crest height of the regular lines
};

// Crests of ordinary relief stand 0.4 to 0.7 apart in relative scatter and differ in height by as
// much; teeth of one size at one distance are what the eye picks out on a skyline.
inline constexpr double SawSpacing = 0.2;
inline constexpr double SawHeight = 0.35;

// Share of shore cells (land with water on one of its four sides) that lie in a row or column of
// at least `run` cells with the water on the same side: a shore drawn along a grid axis. A straight
// shore within 11° of an axis makes such runs as well, so a coast with no preferred direction reads
// up to about a quarter; the fractal coast of test/LandscapeRegularity.cpp reads well under that
// and a coast of whole cells' edges reads close to one. `wraps` joins opposite edges of the field.
inline constexpr double StraightShoreLimit = 0.35;

double straightShoreShare(const Field<uint8_t>& land, bool wraps, int run = 5);

SawtoothReport sawtoothOf(const Field<float>& surface, double cell, const Field<float>* water = nullptr, int window = 128);

} // namespace derivean::sim

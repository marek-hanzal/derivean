#pragma once

#include "landscape/type/Terrain.hpp"
#include "measure/fn/Periodicity.hpp"
#include "measure/type/Distribution.hpp"

namespace derivean::sim {

// An inclined plane of one rock under uniform rain: the surface on which every gully is the
// solver's own doing. With a swale the same plane gathers its water, and a valley is the answer.
struct PlaneSpec {
    Level level{Level::Local};
    double slope{0.03};        // m/m along the fall line
    double azimuth{0};         // rad, direction of fall from +X towards +Z
    double swale{0};           // m the middle fall line lies below the plane's edges
    double swalePower{2};      // 2 for a rounded trough, 1 for two planes meeting in a crease
    double roughness{0.03};    // m, amplitude of the seeded unevenness
    double rain{0.9};          // m/yr
    Material rock{Material::Sandstone};
    int steps{0};              // 0 runs the settling time of the level
    uint64_t seed{42};
};

// Sizes every per-cell array of a hand-built domain and gives it empty layering on a node lattice.
void allocateBare(Domain& domain);

// The plane as a domain of its level, ready for stepDomain: open along its foot, held along the
// rest of its rim. A finer level is led by its trend exactly as a tile under a real parent is.
Domain planeDomain(const PlaneSpec& spec);

// Replays the plane and returns the domain at the end; flow fields describe the last step.
Domain replayPlane(const PlaneSpec& spec);

struct PlaneReport {
    double fall{};               // m the plane drops across the tile
    double lowered{};            // m, mean ground below the starting surface
    Distribution gully;          // m, crest to trough within 16 cells along contour-parallel lines
    Distribution strength;       // autocorrelation peak of those lines
    double regularShare{};       // share of the lines that repeat (measure/fn/Regularity.hpp)
    double gridShare{};          // share that repeat every four cells or closer
    Distribution roughness;      // cellRoughness of those lines
    double notchSpacing{};       // cells of contour per notch; 0 without notches
    std::vector<float> cross;    // m, mean ground against its contour by position across the fall line, one entry per cell width
    Periodicity crossPattern;    // of `cross`; marks are troughs
    double crossRange{};         // m between the 5th and 95th percentile of `cross`
    double footShare{};          // share of the water crossing the foot carried by its wettest tenth of cells; 0.1 for a sheet
    double incisionPerDoubling{};   // m a cell lies lower per doubling of its discharge against its contour
    double correlation{};        // between that doubling and the lowering, -1..1
    double gathered{};           // 0 when the water crosses the foot evenly, 1 when all of it does on the middle fall line
    double axisShare{};          // share of the water crossing the foot within eight cells of the middle fall line; 0.07 for a sheet
    double axisDepth{};          // m that band lies below its flanks, on top of the built swale; negative where it has filled
    double headingError{};       // degrees, mean angle between the paths of gathered water and the fall line
    int paths{};                 // paths behind `headingError`
};

PlaneReport measurePlane(const PlaneSpec& spec, const Domain& domain);

} // namespace derivean::sim

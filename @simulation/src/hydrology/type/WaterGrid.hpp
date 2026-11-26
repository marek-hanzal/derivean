#pragma once

#include "landscape/type/Terrain.hpp"

namespace derivean::sim {

// Open cells hold water and move it. Rim cells are the fixed edge of a grid: water that reaches
// them leaves, and where they keep standing water of their own they feed the cells beside them.
enum class CellKind : uint8_t { Open, Rim, Outside };

struct WaterSource {
    int32_t cell{};
    double flow{};   // m³/yr of water entering at the cell
    double load{};   // m³/yr of sediment it carries
};

struct Tally {
    double water{};   // m³
    double salt{};    // kg
    Tally& operator+=(const Tally& other) {
        water += other.water;
        salt += other.salt;
        return *this;
    }
};

// Every transfer across the edge of a grid is added to exactly one of these.
struct WaterFlows {
    Tally inflow;        // rivers entering across the rim
    Tally rain;          // precipitation on open water and the runoff dry ground sheds
    Tally evaporation;
    Tally outflow;       // left across the rim
    Tally fromBody;      // drawn from standing water the rim keeps
    // A solver sums one call's transfers apart and adds them here once: a running total that is
    // thousands of times a single transfer would round every one of them.
    WaterFlows& operator+=(const WaterFlows& other) {
        inflow += other.inflow, rain += other.rain, evaporation += other.evaporation, outflow += other.outflow, fromBody += other.fromBody;
        return *this;
    }
};

// m³ of solid. What water picks up is carried on until it settles or leaves across the rim.
struct SolidFlows {
    double carriedIn{}, eroded{}, deposited{}, exported{};
};

// Working arrays of the event solver, kept between calls so that a tick allocates nothing.
struct SurgeScratch {
    std::vector<double> surface, sent, gain, saltGain, before;
    std::vector<float> fresh[4];
    std::vector<uint8_t> moves;
};

// One body of water as a grid: a group of site tiles with its rim, or the whole planet. Water is
// the same material on both; only the setting differs. Neighbours are the eight of NeighborX/Z.
struct WaterGrid {
    int width{}, height{};
    double cell{};                      // m
    bool periodic{};
    Ground solid;                       // empty where the solid never changes
    std::vector<float> ground;          // m, rock plus cover
    std::vector<float> held;            // m, water surface a Rim cell keeps; its ground where it is dry
    std::vector<float> salinity;        // kg/m³ of the water a Rim cell keeps
    std::vector<double> depth;          // m of water
    std::vector<double> salt;           // kg/m² dissolved in it, or lying as a crust where it dried up
    std::vector<uint8_t> kind;
    std::vector<float> rain, evaporation, runoff;   // m/yr: on open water, from open water, shed by dry ground
    std::vector<float> erodibility;     // stream power coefficient of the rock
    std::vector<float> balance;         // stream power the untouched bed carries, as streamPower gives it
    std::vector<float> discharge;       // m³/yr
    std::vector<float> flux[4];         // m²/s through the faces towards neighbours 0 to 3
    std::vector<WaterSource> sources;
    double sourceSalinity{};            // kg/m³ of entering rivers and of the runoff dry ground sheds

    size_t cells() const { return size_t(width) * size_t(height); }
    CellKind kindOf(int32_t at) const { return CellKind(kind[size_t(at)]); }

    // Index of neighbour `k`, or -1 beyond the edge of a grid that does not wrap.
    int32_t neighbour(int32_t at, int k) const {
        int x = at % width + NeighborX[k], z = at / width + NeighborZ[k];
        if (periodic) {
            x = wrapIndex(x, width);
            z = wrapIndex(z, height);
        } else if (x < 0 || z < 0 || x >= width || z >= height) {
            return -1;
        }
        return z * width + x;
    }
};

} // namespace derivean::sim

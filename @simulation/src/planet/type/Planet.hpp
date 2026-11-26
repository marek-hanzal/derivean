#pragma once

#include "core/service/Workers.hpp"
#include "core/type/Field.hpp"
#include "planet/constant/Scale.hpp"
#include "strata/type/Strata.hpp"
#include "tectonics/type/Plates.hpp"

namespace derivean::sim {

// Regional tiles restart from the planet only at these absolute times, and every column keeps what
// it looked like at the last few of them. The record travels with the crust, so a region can replay
// its recent history in the frame of the rock it stands on.
inline constexpr double RegionAnchor = 8 * Myr;
inline constexpr int PastMarks = 5;
inline constexpr float ContactReach = 305e3f;   // m from a plate contact within which magmatism acts

// Where melt comes from decides what erupts: wet, viscous and explosive above a sinking plate,
// fluid basalt where crust is stretched or a plume rises.
enum class MeltSource : uint8_t { Arc, Rift, Plume };

struct PastMark {
    float elevation{};   // m against the datum
    float crust{};       // m
    float gain{};        // m, CrustColumn::tectonicGain at the mark
    float eroded{};      // m, CrustColumn::eroded at the mark
    float deposited{};   // m, CrustColumn::deposited at the mark
    float melt{};        // m, CrustColumn::melt at the mark: what a later mark holds more is the melt that rose in between
    float erupted{};     // m, CrustColumn::erupted at the mark
};

// Material of one planet cell: the scalars every step reads. It travels with its plate together
// with its ColumnRecord, so the rock record of a place is that of the crust that sits there now.
struct CrustColumn {
    float crust{};          // m of buoyant felsic crust; the conserved stock mountains are built from
    float oceanAge{};       // yr since the lid under the column solidified or left a spreading contact
    float load{};           // m of loose sediment in transit across the cell
    float tectonicGain{};   // m, signed total of stacking, stretching, magmatism and gravitational flow
    float orogenTime{-1};   // yr of the last collision stacking; negative if never
    float quietSince{};     // yr of the last deformation of any kind
    float exhumed{};        // m of rock stripped since the last stacking
    float meltTime{-1};     // yr melt last rose into the column from a sinking plate, a rift or a plume; negative if never
    float melt{};           // m of crust that melt has added to the column
    float erupted{};        // m of that which reached the surface as lava and ash
    MeltSource meltSource{MeltSource::Arc};   // where the last melt came from
    float eroded{};         // m removed from this column since it formed
    float deposited{};      // m laid onto this column since it formed
    float fabric{};         // rad, strike left by the last deformation
    int16_t driftX{}, driftZ{};   // planet cells travelled since the column formed
    Basement basement{Basement::Primordial};
};

// The bulky, rarely read half of a column: its dated cover and its recent past.
struct ColumnRecord {
    std::array<PastMark, PastMarks> past;
    Cover cover;
};

// Kinematic state of one plate inside the current epoch.
struct PlateMotion {
    Vec2 travelled;        // cells of crust displacement not yet applied to the grid
    double resistance{};   // m·cells of continental crust recently jammed at the leading contacts
    int shifts{};          // whole-cell moves made this epoch
};

// Contact kinematics of a planet cell: the nearest cell of another plate and how the two close.
struct ContactCell {
    float distance{1e9f};  // m to the nearest plate contact; beyond reach in a plate's interior
    float convergence{};   // m/yr closing speed at full plate speed
    float shear{};         // m/yr
    float normalX{}, normalZ{};   // unit, towards the other plate
};

// Totals in m³. Every transfer the planet makes is added to exactly one of these.
struct PlanetLedger {
    double eroded{}, deposited{};
    double subducted{}, accreted{};
    double erupted{};        // arc, rift and hotspot additions
    double precipitated{};   // the part of `deposited` that came out of solution as carbonate
    double waterInventory{}; // all water the body holds
    double waterEscaped{};   // lost through the void
};

struct Planet {
    uint64_t seed{};
    double time{};                 // yr; always a whole number of planet steps
    Field<CrustColumn> columns;
    Field<ColumnRecord> records;   // parallel to `columns`
    Field<float> elevation;        // m, solid surface against the datum
    Field<float> swell;            // m, dynamic support by the mantle of the current epoch
    Field<float> meltHeat;         // 0..1, melt still cooling under the cell, spread over the width of a magmatic belt
    Field<float> erosion;          // m/yr, recent mean at the cell
    Field<float> deposition;       // m/yr, recent mean at the cell
    Field<float> temperature;      // °C, annual mean at the surface
    Field<float> precipitation;    // m/yr
    Field<float> windX, windZ;     // m/s, direction the air travels
    Field<uint16_t> owner;         // plate each cell's crust rides on; changes hands only where one plate overrides another
    Field<ContactCell> contact;    // follows `owner`
    std::vector<uint32_t> contactZone;   // cells close enough to a contact to feel it
    Epoch epoch;
    std::vector<PlateMotion> motion;
    std::vector<float> seaLevels;  // m at every planet step since TectonicsStart
    double seaLevel{};             // m against the datum, solved from volume and basin shape
    double surfaceWater{};         // m³ liquid at the surface
    double surfaceKelvin{};        // K, global mean
    PlanetLedger ledger;
};

// Runs the whole-planet history from the hot initial body to the last planet step at or before `time`.
Planet evolvePlanet(uint64_t seed, double time, Workers& workers);

// planet/fn/Primordial.cpp — closed-form cooling of the initial body.
double quenchTime(uint64_t seed, Vec2 position);
CrustColumn primordialColumn(uint64_t seed, Vec2 position, double time);

// planet/fn/Motion.cpp
void enterEpoch(Planet& planet, Workers& workers);
void movePlates(Planet& planet, double dt);

// planet/fn/Outline.cpp
void traceContacts(Planet& planet);

// planet/fn/Contacts.cpp
void actOnContacts(Planet& planet, double dt);
void relaxThickCrust(Planet& planet, double dt);

// planet/fn/Surface.cpp
float columnElevation(const CrustColumn& column);
void refreshElevation(Planet& planet);
void solveSeaLevel(Planet& planet);
void shapeSurface(Planet& planet, double dt, Workers& workers);
void stampMarks(Planet& planet);

// planet/fn/Heat.cpp
void refreshMeltHeat(Planet& planet);

inline Vec2 planetCellCenter(int x, int z) { return {(x + 0.5) * PlanetCell, (z + 0.5) * PlanetCell}; }

inline int pastMarkIndex(double anchorTime) { return int(int64_t(anchorTime / RegionAnchor) % PastMarks); }

} // namespace derivean::sim

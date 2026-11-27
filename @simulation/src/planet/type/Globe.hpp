#pragma once

#include "geometry/type/Rotation.hpp"
#include "geometry/type/SphereField.hpp"
#include "geometry/type/SphereGrid.hpp"
#include "planet/type/Planet.hpp"
#include "tectonics/type/Euler.hpp"

#include <array>
#include <memory>

namespace derivean::sim {

// The planet's history on the cubed sphere (S3-design.md §3–§5). Crust is Lagrangian: each piece is
// one column of crust with its whole record, born in one cell of the mantle grid and turned since by
// its plates; its stored state is never remapped as it moves, so the strata do not diffuse. The grid
// of the mantle frame (one level of the geometry grid) only couples pieces to the surface processes
// through overlap weights (Jones 1999). The flat Planet stays, as a derived view (fn/FlatView).

// The identity of a piece, independent of storage and of the thread count: the planet step it was
// born in (-1 for the primordial crust), the dense index of its birth cell and its order among the
// pieces born in that cell in that step (the substep). Pieces are kept in key order.
struct PieceKey {
    int32_t step{};
    int32_t cell{};
    int32_t order{};
};

inline bool operator<(const PieceKey& a, const PieceKey& b) {
    if (a.step != b.step) return a.step < b.step;
    if (a.cell != b.cell) return a.cell < b.cell;
    return a.order < b.order;
}
inline bool operator==(const PieceKey& a, const PieceKey& b) { return a.step == b.step && a.cell == b.cell && a.order == b.order; }

// What a piece holds, m³, in double: a float thickness of 30 km has an ulp of 0.002 m, about 10⁷ m³
// on a piece of level 7, and source and target round differently (S3-design.md §4).
struct CrustStock {
    double felsic{};   // buoyant crust including its sediment
    double basalt{};   // oceanic crust from the interior
};

struct CrustPiece {
    CrustStock stock;
    CrustColumn column;     // `crust` = stock.felsic / area, derived; the rest are the piece's own intensive fields
    ColumnRecord record;    // dated cover (a record of composition, not a conserved stock) and past marks
    Rotation rotation;      // Q: everything the piece went through since birth
    PieceKey key;
    int32_t birth{};        // cell of its footprint at birth; the footprint is Q · that cell
    double area{};          // m², the birth cell's; a rotation keeps it
    double bornAt{};        // yr: the substep that bore it, or the body's time for the primordial crust
    uint16_t plate{};
};

// The cells round a cell: across its four edges and at its corners, eight inside a face and on face
// edges, seven at a cube vertex (where three cells meet); -1 pads.
using CellRing = std::array<int32_t, 8>;

// The overlap of pieces and cells: per piece the cells it covers and per cell the pieces that cover it,
// each with the area in m². Exact (candidates and overlaps of S3a) where the piece is flagged, cheap
// cloud-in-cell weights normalised to the piece's area elsewhere; both sum to the piece's area.
struct CellWeight {
    int32_t cell{};
    double area{};
};
struct PieceWeight {
    int32_t piece{};
    double area{};
};
struct Coupling {
    std::vector<int32_t> pieceStart;   // pieces + 1
    std::vector<CellWeight> byPiece;
    std::vector<int32_t> cellStart;    // cells + 1
    std::vector<PieceWeight> byCell;   // per cell in piece order
    std::vector<double> covered;       // per cell, Σ of its weights
    std::vector<CellWeight> slots;     // scratch: CouplingSlots per piece
    std::vector<uint8_t> counts;       // scratch
};
inline constexpr int CouplingSlots = 16;

// What a plate carries through an epoch besides its Euler vector.
struct GlobeMotion {
    double resistance{};   // m³: continental crust jammed at its leading contacts, its brake
    // yr: the area mean of the age of the plate's ocean floor as the epoch starts, 0 for a plate with no
    // ocean then. Which of two oceans sinks is the plates' choice, not the pieces' (sinksUnder).
    double oceanAge{};
};

// The nearest cell of another plate as a cell of the mantle grid sees it, with the relative motion of
// the two plates there (S3-design.md §3): convergence c = v_rel · n̂ (positive closes) and shear
// s = v_rel · (x × n̂), from the applied Euler vectors.
struct GlobeContact {
    float distance{1e9f};   // m to the contact
    float convergence{};    // m/yr
    float shear{};          // m/yr
    int32_t across{-1};     // the nearest cell of the other plate
    Vec3 normal;            // unit tangent, towards the other plate
};

// Totals in m³; every transfer adds to one. The interior is an account: arcs, rifts, hotspots and new
// ocean floor take from it, subduction returns to it (S3-design.md §4, §5).
struct GlobeLedger {
    double eroded{}, deposited{}, precipitated{};
    double subducted{}, accreted{}, erupted{}, stacked{};
    double born{}, consumed{};      // m² of footprint made at ridges and consumed at trenches and collisions
    double consumedFelsic{};        // m³ the consumed pieces held when they were consumed
    double churned{};               // m² of footprint consumed within ChurnAge of its birth (S3-design.md test 14)
    CrustStock interior;            // net gain of the interior since the start (negative: it gave)
    double waterInventory{};        // m³, all water the body holds (as the flat history: a closed stock, S3d replaces it)
};

// Wall time of a step's phases, s (diagnostics only: no state reads it, so determinism ignores it).
enum class GlobePhase : uint8_t { Band, Substeps, Rest, Couple, Contacts, Elevation, Climate, Surface, Count };

// What the boundary events did, for `terra globe` and the tests.
struct GlobeStats {
    std::array<double, size_t(GlobePhase::Count)> seconds{};
    long substeps{}, rounds{}, births{}, consumptions{}, cancelled{}, deferred{}, churned{}, welded{};   // welded: births with no lattice slot
    long contested{};   // candidates two or more overriding plates cover (triple junctions)
    long bandCells{}, exactPieces{}, unions{};
    long filled{};   // births where no contact opens: fillCell (at closings too) and fillSlot in the band (not the fossils' slots)
    long held{};   // candidates held back since consuming them would open a cell by more than half
    long sutured{}, fossil{};   // seams: consumed by their own plate, born in old band slots at reorganisations
    long seamsKept{};           // pieces a closing or a substep ended half covered by their own plate for want of room
    long unsettled{};           // reorganisations whose seams did not settle in SeamRounds or kept a piece
    long refusedSubsteps{};     // substeps that ended with a seam kept for want of room
    long unsettledSubsteps{};                 // substeps whose sutures still consumed after BearRounds rounds of births
};

struct Globe {
    uint64_t seed{};
    double time{};
    PlateSpeeds speeds{PlateSpeeds::Today};
    std::shared_ptr<const SphereGrid> grid;
    std::shared_ptr<const std::vector<CellRing>> rings;   // per cell of the grid
    std::vector<CrustPiece> pieces;      // in key order
    Coupling coupling;
    SphereField<float> elevation;        // m, view of the solid surface against the datum
    SphereField<float> swell;            // m, dynamic support of the current epoch, fixed in the mantle
    SphereField<float> meltHeat;         // 0..1, melt still cooling under the cell
    SphereField<float> erosion, deposition;   // m/yr, recent means
    SphereField<float> temperature, precipitation;   // °C, m/yr, read back from the flat climate
    SphereField<uint16_t> owner;         // plate of the piece with the largest weight
    SphereField<GlobeContact> contact;
    std::vector<int32_t> contactZone;
    EulerEpoch epoch;
    std::vector<GlobeMotion> motion;
    std::vector<Vec3> applied;           // rad/yr, ω̂ per plate after damping and NNR
    std::vector<float> seaLevels;
    double seaLevel{}, surfaceWater{}, surfaceKelvin{};
    GlobeLedger ledger;
    GlobeStats stats;
};

} // namespace derivean::sim

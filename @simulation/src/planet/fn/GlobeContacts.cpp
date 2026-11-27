#include "planet/fn/GlobeContacts.hpp"

#include "core/fn/Hash.hpp"
#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Metric.hpp"
#include "geometry/fn/Rotation.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/fn/Pieces.hpp"
#include "tectonics/fn/Euler.hpp"
#include "tectonics/type/Plates.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// The flat Contacts.cpp's numbers; arcs and plumes are per area or per spot, so they carry over.
constexpr double ArcYield = 1660.0;            // m² of new crust per m of plate consumed, per m of contact
constexpr double ArcOffset = 130e3;            // m behind the contact where the melt rises
constexpr double ArcWidth = 70e3;
constexpr double ArcNorm = 124e3;              // integral of the arc profile across the belt
constexpr float ArcSurfaceShare = 0.35f;
constexpr float ArcCeiling = 44000.0f;         // m of crust above which rising melt stalls in the root
constexpr double HotspotEra = 150 * Myr;
constexpr int HotspotCount = 12;
constexpr double HotspotVolume = 1800.0 * PlanetCellArea;   // m³ per planet step above a plume (1800 m on a flat cell)
constexpr uint64_t HotspotStream = 0x484f5453;
constexpr double SpreadStart = 50000.0;        // m of crust above which the pile flows under its own weight
constexpr double SpreadRange = 14000.0;

bool oceanic(const CrustColumn& column) { return column.crust < float(OceanicLimit); }

const CrustColumn* viewColumn(const Globe& globe, int cell) {
    const int piece = dominantPiece(globe.coupling, cell);
    return piece < 0 ? nullptr : &globe.pieces[size_t(piece)].column;
}

// Felsic crust from the interior into the pieces over a cell, `gain` m on the cell, `layer` m of it
// at the surface as a dated deposit. With plate ≥ 0 only that plate's pieces take it, by their weights
// over what they cover: an arc or a rift erupts into its own side, not into the other plate's pieces
// that overlap the cell. A fabric (rad, NaN for none) is the strike the flat history writes there.
void erupt(Globe& globe, int cell, double gain, Facies facies, double layer, MeltSource source, bool arc, int plate = -1, float fabric = std::nanf("")) {
    const Coupling& coupling = globe.coupling;
    const int32_t first = coupling.cellStart[size_t(cell)], last = coupling.cellStart[size_t(cell) + 1];
    double covered = 0;
    for (int32_t k = first; k < last; ++k)
        if (plate < 0 || globe.pieces[size_t(coupling.byCell[size_t(k)].piece)].plate == plate) covered += coupling.byCell[size_t(k)].area;
    if (covered <= 0) return;
    const double area = globe.grid->area[size_t(cell)];
    for (int32_t k = first; k < last; ++k) {
        const PieceWeight& w = coupling.byCell[size_t(k)];
        CrustPiece& piece = globe.pieces[size_t(w.piece)];
        if (plate >= 0 && piece.plate != plate) continue;
        const double toPiece = area * (w.area / covered) / piece.area;
        const float added = float(gain * toPiece);
        giveFelsic(piece, gain * toPiece * piece.area);
        CrustColumn& column = piece.column;
        column.tectonicGain += added;
        column.quietSince = column.meltTime = float(globe.time);
        column.melt += added;
        column.erupted += float(layer * toPiece);
        column.meltSource = source;
        if (arc && column.basement != Basement::Craton && column.basement != Basement::Orogen) column.basement = Basement::Arc;
        if (!std::isnan(fabric)) column.fabric = fabric;
        addDeposit(piece.record.cover, globe.time, float(layer * toPiece), facies);
    }
    globe.ledger.erupted += gain * area;
    globe.ledger.interior.felsic -= gain * area;
}

// Moves up to `volume` m³ of felsic crust from one cell's pieces to another's; returns what moved.
double moveCrust(Globe& globe, int from, int to, double volume) {
    if (globe.coupling.covered[size_t(to)] <= 0) return 0;
    Sum taken;
    forPiecesOver(globe.coupling, globe.pieces, from, 1.0, [&](CrustPiece& piece, double toPiece) {
        const double part = takeFelsic(piece, volume * toPiece * piece.area);
        piece.column.tectonicGain -= float(part / piece.area);
        taken.add(part);
    });
    const double moved = taken.value();
    forPiecesOver(globe.coupling, globe.pieces, to, 1.0, [&](CrustPiece& piece, double toPiece) {
        giveFelsic(piece, moved * toPiece * piece.area);
        piece.column.tectonicGain += float(moved * toPiece);
    });
    return moved;
}

} // namespace

void traceGlobeContacts(Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    const std::vector<CellRing>& rings = *globe.rings;
    for (int cell = 0; cell < grid.cells; ++cell) {
        const int piece = dominantPiece(globe.coupling, cell);
        globe.owner.value[size_t(cell)] = piece < 0 ? uint16_t(0xffff) : globe.pieces[size_t(piece)].plate;
    }
    const auto& owner = globe.owner.value;
    // As the flat history's contacts (Outline.cpp, Contacts.cpp): the epoch's vectors at its ramps, not
    // slowed by the jam, so a jammed collision still builds its arc. A common turn (NNR) does not
    // change a relative motion, so none is taken off.
    Epoch ramps;
    ramps.start = globe.epoch.start;
    ramps.end = globe.epoch.end;
    const double activity = epochActivity(ramps, globe.time);
    std::vector<Vec3> intended(globe.epoch.plates.size());
    for (size_t p = 0; p < intended.size(); ++p) intended[p] = globe.epoch.plates[p].euler * activity;
    const auto relative = [&](int cell, int other, Vec3 normal) {
        return closingAt(intended[owner[size_t(cell)]], intended[owner[size_t(other)]], grid.centre[size_t(cell)], normal);
    };
    std::vector<int32_t> source(size_t(grid.cells), -1), frontier, next;
    std::vector<double> reach(size_t(grid.cells), 1e30);
    for (int cell = 0; cell < grid.cells; ++cell) {
        if (owner[size_t(cell)] == 0xffff) continue;
        // Where plates meet at a corner the contact that closes fastest is the one that acts.
        double best = 0;
        for (int k = 0; k < 4; ++k) {
            const int other = neighbourOf(grid, cell, k);
            if (owner[size_t(other)] == owner[size_t(cell)] || owner[size_t(other)] == 0xffff) continue;
            const double closing = relative(cell, other, towards(grid.centre[size_t(cell)], grid.centre[size_t(other)])).convergence;
            if (source[size_t(cell)] >= 0 && closing <= best) continue;
            source[size_t(cell)] = other, best = closing;
        }
        if (source[size_t(cell)] < 0) continue;
        reach[size_t(cell)] = distanceBetween(grid.centre[size_t(cell)], grid.centre[size_t(source[size_t(cell)])]);
        frontier.push_back(cell);
    }
    while (!frontier.empty()) {
        next.clear();
        for (const int32_t cell : frontier)
            for (const int32_t inner : rings[size_t(cell)]) {
                if (inner < 0 || owner[size_t(inner)] != owner[size_t(cell)]) continue;
                const int32_t from = source[size_t(cell)];
                const double distance = distanceBetween(grid.centre[size_t(inner)], grid.centre[size_t(from)]);
                if (distance >= reach[size_t(inner)] || distance > ContactReach + 2 * std::sqrt(grid.area[size_t(from)])) continue;
                if (source[size_t(inner)] < 0) next.push_back(inner);
                source[size_t(inner)] = from, reach[size_t(inner)] = distance;
            }
        std::sort(next.begin(), next.end());
        next.erase(std::unique(next.begin(), next.end()), next.end());
        frontier.swap(next);
    }
    for (const int32_t cell : globe.contactZone) globe.contact.value[size_t(cell)] = GlobeContact{};
    globe.contactZone.clear();
    for (int cell = 0; cell < grid.cells; ++cell) {
        const int32_t from = source[size_t(cell)];
        if (from < 0) continue;
        // The contact runs half a cell short of the foreign cell's centre.
        const double distance = reach[size_t(cell)] - 0.5 * std::sqrt(grid.area[size_t(from)]);
        if (distance >= ContactReach) continue;
        GlobeContact& contact = globe.contact.value[size_t(cell)];
        contact.normal = towards(grid.centre[size_t(cell)], grid.centre[size_t(from)]);
        const Closing closing = relative(cell, from, contact.normal);
        contact.distance = float(std::max(0.0, distance));
        contact.convergence = float(closing.convergence);
        contact.shear = float(closing.shear);
        contact.across = from;
        globe.contactZone.push_back(cell);
    }
}

void actOnGlobeContacts(Globe& globe, double dt) {
    const SphereGrid& grid = *globe.grid;
    for (const int32_t cell : globe.contactZone) {
        const GlobeContact contact = globe.contact.value[size_t(cell)];
        const CrustColumn* column = viewColumn(globe, cell);
        if (!column) continue;
        // The contact carries the epoch's ramps already (traceGlobeContacts).
        const double closing = contact.convergence;
        const int plate = globe.owner.value[size_t(cell)];
        const Vec3 x = grid.centre[size_t(cell)];
        if (closing > 0.001) {
            if (column->crust > ArcCeiling) continue;
            const double far = 2.0 * contact.distance + std::sqrt(grid.area[size_t(cell)]);
            const Vec3 there = rotate(rotationAbout(cross(x, contact.normal), far / PlanetRadius), x);
            const CrustColumn* other = viewColumn(globe, indexOf(grid, cellOf(there, grid.level)));
            // The denser side sinks: ocean under continent, the older plate's ocean under the younger's
            // (oceanSinks): an arc rises where the other side sinks under this one.
            const int across = globe.owner.value[size_t(indexOf(grid, cellOf(there, grid.level)))];
            if (!other || !oceanic(*other) || (oceanic(*column) && (across == plate || !oceanSinks(globe, across, plate)))) continue;
            const double offset = (contact.distance - ArcOffset) / ArcWidth;
            const double gain = ArcYield * closing * dt * std::exp(-offset * offset) / ArcNorm;
            if (gain < 1.0) continue;
            erupt(globe, cell, gain, Facies{0, 25, 30, 0, 200, 0}, ArcSurfaceShare * gain, MeltSource::Arc, true, plate, flatStrikeOf(x, contact.normal));
        } else if (closing < -0.002 && contact.distance < std::sqrt(grid.area[size_t(cell)]) && !oceanic(*column)) {
            // Stretched continent leaks basalt along the opening contact.
            erupt(globe, cell, 40.0, Facies{0, 0, 15, 0, 240, 0}, 40.0, MeltSource::Rift, false, plate, flatStrikeOf(x, contact.normal));
        }
    }
    // Plumes stay put in the mantle frame for an era while the plates carry their products away.
    const int64_t era = int64_t(globe.time / HotspotEra);
    const uint64_t seed = stream(globe.seed, HotspotStream);
    for (int spot = 0; spot < HotspotCount; ++spot) {
        const double z = 2 * unit(hash(seed, era, spot, 1)) - 1, angle = 6.283185307179586 * unit(hash(seed, era, spot, 2));
        const double r = std::sqrt(std::max(0.0, 1 - z * z));
        const int cell = indexOf(grid, cellOf(Vec3{r * std::cos(angle), r * std::sin(angle), z}, grid.level));
        const double vigor = ranged(hash(seed, era, spot, 3), 0.4, 1.0);
        const double area = grid.area[size_t(cell)];
        erupt(globe, cell, HotspotVolume * vigor / area, Facies{0, 0, 10, 0, 245, 0}, 300.0 * PlanetCellArea * vigor / area, MeltSource::Plume, false);
    }
}

void relaxGlobeThickCrust(Globe& globe, double dt) {
    // Explicit conservative diffusion whose mobility rises with the excess thickness, so a narrow stack
    // widens into a plateau. The flat number 0.24 per edge of a 65.5 km cell becomes a volume per metre
    // of difference, PlanetCellArea · edge / distance, capped by the smaller cell for stability.
    const SphereGrid& grid = *globe.grid;
    const double base = 0.24 * dt / PlanetStep;
    std::vector<double> crust(size_t(grid.cells)), mobility(size_t(grid.cells));
    bool any = false;
    for (int cell = 0; cell < grid.cells; ++cell) {
        crust[size_t(cell)] = viewOf(globe.coupling, globe.pieces, cell, [](const CrustPiece& p) { return p.column.crust; });
        const double excess = std::clamp((crust[size_t(cell)] - SpreadStart) / SpreadRange, 0.0, 1.0);
        mobility[size_t(cell)] = excess * excess;
        any = any || excess > 0;
    }
    if (!any) return;
    struct Flow {
        int from, to;
        double volume;
    };
    std::vector<Flow> flows;
    for (const SphereEdge& edge : grid.edge) {
        const double rate = std::max(mobility[size_t(edge.left)], mobility[size_t(edge.right)]);
        if (rate <= 0) continue;
        const double distance = distanceBetween(grid.centre[size_t(edge.left)], grid.centre[size_t(edge.right)]);
        const double reach = std::min(PlanetCellArea * edge.length / distance, std::min(grid.area[size_t(edge.left)], grid.area[size_t(edge.right)]));
        const double volume = base * rate * reach * (crust[size_t(edge.left)] - crust[size_t(edge.right)]);
        if (volume > 0) flows.push_back({edge.left, edge.right, volume});
        else if (volume < 0) flows.push_back({edge.right, edge.left, -volume});
    }
    for (const Flow& flow : flows) moveCrust(globe, flow.from, flow.to, flow.volume);
}

} // namespace derivean::sim

#include "planet/fn/Lay.hpp"

#include "planet/fn/Boundary.hpp"

#include "geometry/fn/SphereGrid.hpp"
#include "planet/constant/Globe.hpp"
#include "planet/constant/Scale.hpp"
#include "planet/fn/Interior.hpp"
#include "planet/fn/Lattice.hpp"
#include "planet/fn/Pieces.hpp"

#include <array>

namespace derivean::sim {
namespace {

constexpr float StretchShare = 0.4f;  // of a continental neighbour's crust and cover that necks into the gap (Crossing.cpp)

void resetMarks(CrustPiece& piece) {
    const CrustColumn& c = piece.column;
    piece.record.past.fill({columnElevation(c), c.crust, c.tectonicGain, c.eroded, c.deposited, c.melt, c.erupted});
}

// The new piece of a stretched continent: 40 % of the crust, basalt and cover of each continental
// neighbour of its plate across the cell's edges, weighted by the length of the shared edge, so the
// neighbours give 40 % between them as one neighbour does on the flat grid.
bool neck(Globe& globe, const Band& band, int cell, int plate, CrustPiece& fresh) {
    const SphereGrid& grid = *globe.grid;
    // A piece across two of the cell's edges gives once, by both edges' length.
    std::array<int, 4> giver{-1, -1, -1, -1};
    std::array<double, 4> edge{};
    int givers = 0;
    double total = 0;
    for (int k = 0; k < 4; ++k) {
        const int other = neighbourOf(grid, cell, k);
        int best = -1;
        double most = 0;
        forLive(band, other, [&](int k, double area) {
            if (globe.pieces[size_t(k)].plate == plate && area > most) best = k, most = area;
        });
        if (best < 0 || globe.pieces[size_t(best)].column.crust < 0.5f * float(ContinentalLimit)) continue;
        const double length = grid.edge[size_t(grid.edges[size_t(cell)][size_t(k)])].length;
        int slot = 0;
        while (slot < givers && giver[size_t(slot)] != best) ++slot;
        if (slot == givers) giver[size_t(givers++)] = best;
        edge[size_t(slot)] += length;
        total += length;
    }
    if (total <= 0) return false;
    int widest = -1;
    for (int k = 0; k < givers; ++k) {
        CrustPiece& from = globe.pieces[size_t(giver[size_t(k)])];
        if (widest < 0 || edge[size_t(k)] > edge[size_t(widest)]) widest = k;
        const double share = StretchShare * edge[size_t(k)] / total;
        const double felsic = takeFelsic(from, share * from.stock.felsic);
        const double basalt = share * from.stock.basalt;
        from.stock.basalt -= basalt;
        fresh.stock.felsic += felsic;
        fresh.stock.basalt += basalt;
        Cover part = from.record.cover;
        scaleCover(part, float(share * from.area / fresh.area));
        stackCover(fresh.record.cover, part);
        scaleCover(from.record.cover, float(1 - share));
        from.column.tectonicGain -= float(felsic / from.area);
        from.column.quietSince = float(globe.time);
    }
    const CrustColumn& like = globe.pieces[size_t(giver[size_t(widest)])].column;
    fresh.column = like;
    fresh.column.oceanAge = 0;
    fresh.column.load = 0;
    fresh.column.tectonicGain = fresh.column.eroded = fresh.column.deposited = 0;
    fresh.column.driftX = fresh.column.driftZ = 0;
    fresh.column.quietSince = float(globe.time);
    return true;
}

// Ocean floor laid as `like`'s plate's floor there: as old as `like` when `like` is too thin to neck
// (neck's own bound), so thinned margins between ocean and continent pass their age on too.
void ageAs(CrustPiece& laid, const CrustPiece& like) {
    if (laid.column.basement != Basement::Oceanic || like.column.crust >= 0.5f * float(ContinentalLimit)) return;
    laid.column.oceanAge = like.column.oceanAge;
    refreshColumn(laid);
    resetMarks(laid);
}

} // namespace

bool fillCell(Globe& globe, Band& band, int cell, int like, PieceKey key, double bornAt) {
    const int plate = globe.pieces[size_t(like)].plate;
    CrustPiece fresh;
    Slot slot;
    fresh.birth = cell;
    if (openSlot(globe, band, cell, plate, {}, true, slot)) fresh.rotation = slot.rotation, fresh.birth = slot.birth;
    else ++globe.stats.welded;
    fresh.key = key;
    fresh.area = globe.grid->area[size_t(fresh.birth)];
    fresh.bornAt = bornAt;
    fresh.plate = uint16_t(plate);
    layNewPiece(globe, band, fresh, cell, true);
    ageAs(globe.pieces.back(), globe.pieces[size_t(like)]);
    ++globe.stats.filled;
    return true;
}

bool fillSlot(Globe& globe, Band& band, int cell, PieceKey key, double bornAt) {
    InteriorSlot found;
    if (!interiorSlot(globe, band, cell, found)) return false;
    CrustPiece fresh;
    fresh.rotation = found.slot.rotation, fresh.birth = found.slot.birth;
    fresh.key = key;
    fresh.area = globe.grid->area[size_t(fresh.birth)];
    fresh.bornAt = bornAt;
    fresh.plate = uint16_t(found.plate);
    layNewPiece(globe, band, fresh, found.cell, true);
    ageAs(globe.pieces.back(), globe.pieces[size_t(found.from)]);
    return true;
}

void layNewPiece(Globe& globe, Band& band, CrustPiece fresh, int cell, bool stretch) {
    if (!stretch || !neck(globe, band, cell, fresh.plate, fresh)) {
        fresh.column = CrustColumn{};
        fresh.column.quietSince = float(globe.time);
        fresh.column.basement = Basement::Oceanic;
        fresh.stock.basalt = OceanicCrust * fresh.area;
        globe.ledger.interior.basalt -= fresh.stock.basalt;
    }
    refreshColumn(fresh);
    resetMarks(fresh);
    globe.pieces.push_back(fresh);
    joinBand(globe, band, int(globe.pieces.size()) - 1);
    globe.ledger.born += fresh.area;
    ++globe.stats.births;
}

} // namespace derivean::sim

#pragma once

#include "geometry/fn/SpherePolygon.hpp"
#include "geometry/fn/Union.hpp"
#include "planet/fn/Coupling.hpp"
#include "planet/fn/Interior.hpp"
#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <vector>

// The gates inside one plate, in its own frame (planet/fn/Interior.hpp; lead, 27 Nov): the largest share
// of a piece that other pieces of its own plate cover together, and the most open slot of a plate's
// lattice where no other plate is near. Both read the exact coupling of the whole globe.
namespace derivean::sim::globe_test {

struct Gaps {
    double ownCover{}, slotOpen{};
    long ownHalf{}, slotHalf{};   // pieces and slots at ½ or more
};

inline Gaps measureGaps(const Globe& globe) {
    const SphereGrid& grid = *globe.grid;
    Coupling exact;
    couple(grid, globe.pieces, {}, true, exact);
    Gaps gaps;
    long unions = 0;
    std::vector<int> others, near;
    std::vector<PlanePolygon> parts;
    const auto gather = [&](std::vector<int>& into, int cell) {
        for (int32_t w = exact.cellStart[size_t(cell)]; w < exact.cellStart[size_t(cell) + 1]; ++w) {
            const int l = exact.byCell[size_t(w)].piece;
            if (std::find(into.begin(), into.end(), l) == into.end()) into.push_back(l);
        }
    };
    for (size_t k = 0; k < globe.pieces.size(); ++k) {
        const CrustPiece& piece = globe.pieces[k];
        others.clear();
        for (int32_t s = exact.pieceStart[k]; s < exact.pieceStart[k + 1]; ++s) {
            const int cell = exact.byPiece[size_t(s)].cell;
            for (int32_t w = exact.cellStart[size_t(cell)]; w < exact.cellStart[size_t(cell) + 1]; ++w) {
                const int l = exact.byCell[size_t(w)].piece;
                if (l != int(k) && !tiles(globe.pieces[size_t(l)], piece) && globe.pieces[size_t(l)].plate == piece.plate &&
                    std::find(others.begin(), others.end(), l) == others.end())
                    others.push_back(l);
            }
        }
        if (!others.empty()) {
            const TangentPlane plane = tangentPlaneAt(centreOf(grid, piece));
            const PlanePolygon own = projected(plane, footprintOf(grid, piece));
            parts.clear();
            for (const int l : others) parts.push_back(clipped(projected(plane, footprintOf(grid, globe.pieces[size_t(l)])), own));
            const double share = unionArea(parts).area / piece.area;
            gaps.ownCover = std::max(gaps.ownCover, share);
            gaps.ownHalf += share >= 0.5;
        }
        for (int e = 0; e < 4; ++e) {
            const LatticeSlot slot = slotOf(globe, int(k), e);
            near.clear();
            gather(near, slot.cell);
            for (const int32_t other : (*globe.rings)[size_t(slot.cell)])
                if (other >= 0) gather(near, other);
            const double open = interiorOpening(globe, int(k), slot, near, unions);
            gaps.slotOpen = std::max(gaps.slotOpen, open);
            gaps.slotHalf += open >= 0.5;
        }
    }
    return gaps;
}

} // namespace derivean::sim::globe_test

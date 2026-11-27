#pragma once

#include "geometry/type/SpherePolygon.hpp"
#include "planet/fn/Band.hpp"
#include "planet/fn/Lattice.hpp"

#include <vector>

// Gaps inside one plate, measured in the plate's own frame (lead's decision of 27 Nov on Codex
// DREV-CLAUDE-005-04, docs/handoff/planet-globe.md, stage 2b). A plate dealt the pieces of two old plates
// holds two lattices; a rigid plate carries the seam between them over the mantle grid and opens mantle
// cells by up to √2 of the seam's width, so inside a plate the gate is the slot of its own lattice: no
// slot next to a piece of the plate is open by half or more where no other plate is near.
namespace derivean::sim {

// Pieces of one lattice share their rotation bit for bit: a slot copies its template's rotation, and the
// pieces of one plate turn by the same rotations in the same order (planet/fn/Boundary.cpp). Two pieces of
// one lattice tile without overlap; only pieces of different lattices of one plate can overlap.
bool sameLattice(const CrustPiece& a, const CrustPiece& b);

// Whether two pieces tile: one lattice, born in different cells, so they share at most an edge. Two
// pieces of one lattice born in one cell are one footprint twice, and a seam's suture and the gates see them.
bool tiles(const CrustPiece& a, const CrustPiece& b);

// The slot of piece t's lattice across edge k of its birth cell: its birth cell, footprint and the
// mantle cell of its centre.
struct LatticeSlot {
    int birth{-1}, cell{-1};
    SpherePolygon footprint;
    double area{};
};
LatticeSlot slotOf(const Globe& globe, int t, int k);

// The share of a slot of t's plate that `near` (every piece that may reach it: those over its cell and
// that cell's ring) leaves uncovered, or −1 when a piece of another plate is among them.
double interiorOpening(const Globe& globe, int t, const LatticeSlot& slot, const std::vector<int>& near, long& unions);

// The most open slot inside one plate, open by half or more, next to a live piece over a deciding cell
// or its ring (the lower key of the piece, then the lower edge, on a tie): its birth cell, the rotation,
// plate and index of the piece it continues, and the mantle cell of its centre. Only slots whose mantle
// cell is a deciding cell or next to one are taken, and only where the band's region holds the two rings
// round that cell, so every piece that may reach the slot is listed and the new piece stays in the band.
// What it measures is kept in the band for the rest of the generation (Band::slotOpen).
struct InteriorSlot {
    Slot slot;
    int plate{-1}, from{-1}, cell{-1};
};
bool interiorSlot(Globe& globe, Band& band, int cell, InteriorSlot& found);

} // namespace derivean::sim

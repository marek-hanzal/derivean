#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// Plate contacts on the globe (S3-design.md §2, §3), the spherical counterparts of the flat Outline.cpp
// and Contacts.cpp with the same physics.

// Every cell's owner (the plate of its dominant piece) and, within ContactReach, its nearest cell of
// another plate: a multi-source spread over the cells round each cell from the outlines, each cell
// carrying its source and the geodesic distance to it. The normal is the unit tangent towards the
// source; convergence and shear come from the applied Euler vectors (positive convergence closes).
void traceGlobeContacts(Globe& globe);

// Arcs above sinking plates, basalt leaking along opening continental contacts and hotspots fixed in
// the mantle frame. Their crust comes out of the interior; each is booked into the pieces over the cell
// by weight (planet/fn/Coupling.hpp forPiecesOver).
void actOnGlobeContacts(Globe& globe, double dt);

// Thick crust flows under its own weight: one conservative flux per edge of the mantle grid, from the
// thicker cell's pieces to the thinner cell's.
void relaxGlobeThickCrust(Globe& globe, double dt);

} // namespace derivean::sim

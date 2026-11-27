#pragma once

#include "planet/fn/Band.hpp"

namespace derivean::sim {

// Moves the plates through one planet step of `dt` (yr), step number `step` (for the keys of new
// pieces): the applied Euler vectors, the band, then in each substep the rotation, the exact coupling
// of the band, consumption by a stable transfer plan and births where a contact opens (S3-design.md
// §3–§4). Ends with every piece turned, the consumed gone, the pieces in key order and the coupling
// of the whole globe (exact for the band's pieces, cloud-in-cell for the rest). `refine` multiplies the
// substeps the motion asks for, so a convergence test can halve them at the same step (Codex V4).
void advancePlates(Globe& globe, double dt, int step, int refine = 1);

// What the consumption and the births of a substep need to know about it.
struct Substep {
    int step{}, index{};
    double duration{};   // yr
};

// Whether `donor` is the denser of two crusts and sinks under `survivor` (S3-design.md §3): ocean under
// continent, the older plate's ocean under the younger's by the mean age of their ocean floor as the
// epoch began (GlobeMotion::oceanAge; the higher plate index on a tie); two continents do not sink. A
// trench so keeps one polarity along its length and through the epoch, as on Earth, where a polarity
// turns only with a reorganisation (Stern 2002, docs/credits). Floor born at a contact during the epoch
// therefore cannot turn a trench (handoff, stage 2b).
bool sinksUnder(const Globe& globe, int donor, int survivor);

// Whether the ocean of `plate` sinks under the ocean of `other` (sinksUnder's rule for two oceans,
// which the arcs of planet/fn/GlobeContacts read too); never within one plate.
bool oceanSinks(const Globe& globe, int plate, int other);

// Whether a piece carries continental crust (at least OceanicLimit).
bool continentalPiece(const CrustPiece& piece);

// A transfer plan's rule for a donor that reaches a survivor: it sinks under the survivor, or both are
// ocean, which never stack, so a chain that ends on an older ocean still sends the donor down.
bool sinksInPlan(const Globe& globe, int donor, int survivor);

// The unit tangent at `from` along the great circle towards `to`.
Vec3 towards(Vec3 from, Vec3 to);

// planet/fn/Consume.cpp: the pieces overriding plates cover by half or more, the plan, its booking.
void consumeInBand(Globe& globe, Band& band, const Substep& substep);

// planet/fn/Spread.cpp: new pieces in deciding cells uncovered by half or more where a contact opens,
// and in slots half open inside one plate. A substep bears up to BearRounds times, again after each
// suture of its seams that consumed something (planet/fn/Seam.hpp); `round` numbers them.
inline constexpr int BearRounds = 6;
void bearInBand(Globe& globe, Band& band, const Substep& substep, int round);

// The end of a step's or a reorganisation's band work: the consumed gone, the pieces in key order and
// the coupling of the whole globe, exact for the band's pieces and cloud-in-cell for the rest; with
// `keepExact` also exact for the pieces the coupling before it had exact (a closing after the step's own
// band work, planet/fn/Cohesion, keeps the step's boundaries exact for the contacts and the next band).
void settleBand(Globe& globe, const Band& band, bool keepExact);

inline constexpr float StackLimit = 64000.0f;   // m of crust a collision can stack before it backs up

} // namespace derivean::sim

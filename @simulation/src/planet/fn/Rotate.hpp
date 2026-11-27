#pragma once

#include "planet/type/Globe.hpp"

namespace derivean::sim {

// A new epoch on the globe: convection reorganises and every piece is dealt to the plate under its
// centre. Only `plate` changes; Q, the birth cell and the record go on bit for bit (Codex V1). Each
// plate then gets its Euler vector from its new crust (tectonics/fn/Euler.hpp) and the mean age of its
// ocean floor, which sets the polarity of its trenches for the epoch (sinksUnder), and the swell of the
// epoch is laid on the mantle grid.
void enterGlobeEpoch(Globe& globe);

// The Euler vectors the pieces turn by in a planet step of `dt` (yr): each plate's vector damped by
// the epoch's ramps (epochActivity) and by its jam, 1 / (1 + resistance / JamScale)², with the jam
// decaying over JamMemory; then the net rotation of the damped vectors removed (NNR after damping,
// Codex V5). Writes globe.applied.
void applyKinematics(Globe& globe, double dt);

// Turns every piece by its plate's applied Euler vector (rad/yr) over `dt` (yr): Q ← q(ω̂_p, dt) · Q,
// renormalized. One rotation per plate, so pieces with the same history keep bit-identical Q.
void rotatePieces(std::vector<CrustPiece>& pieces, const std::vector<Vec3>& applied, double dt);

// How many substeps a planet step takes: enough that the fastest relative motion across a contact
// moves at most one shortest edge per substep, and always at least one (Codex V4).
int substepCount(double fastestRelative, double dt, double shortestEdge);

// Where the rock under a direction was when its piece was born, as a point in metres of the sphere:
// R · Q⁻¹ x. Continuous inside a piece and across face seams (the material space is 3D, S3-design.md
// §8); it jumps only between pieces of different history.
Vec3 materialOf(const CrustPiece& piece, Vec3 direction);

// Σ_p I_p ω̂_p and Σ_p |I_p ω̂_p| over the pieces as they stand: the net rotation left and its scale.
struct NetRotation {
    Vec3 net;
    double scale{};
};
NetRotation netRotationOf(const Globe& globe);

} // namespace derivean::sim

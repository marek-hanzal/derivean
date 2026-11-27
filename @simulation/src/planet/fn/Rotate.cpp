#include "planet/fn/Rotate.hpp"

#include "geometry/constant/Sphere.hpp"
#include "geometry/fn/Rotation.hpp"
#include "planet/fn/Boundary.hpp"
#include "planet/fn/Pieces.hpp"
#include "tectonics/fn/Euler.hpp"

#include <algorithm>
#include <cmath>

namespace derivean::sim {
namespace {

// The flat history's jam (Motion.cpp) was 2.5e6 m·cells of continent crossing a contact, the
// thickness of each column times one cell of travel; in volume that is 2.5e6 m times its 65.5 km cell.
constexpr double JamScale = 2.5e6 * PlanetCellArea;   // m³ of jammed continent that slows a plate to a quarter
constexpr double JamMemory = 30 * Myr;

std::vector<Inertia> inertiaOf(const Globe& globe) {
    std::vector<Inertia> inertia(globe.epoch.plates.size());
    for (const CrustPiece& piece : globe.pieces) addInertia(inertia[piece.plate], centreOf(*globe.grid, piece), piece.area);
    return inertia;
}

} // namespace

void enterGlobeEpoch(Globe& globe) {
    globe.epoch = eulerEpochAt(globe.seed, globe.time);
    const size_t plates = globe.epoch.plates.size();
    std::vector<double> area(plates, 0.0), sin2(plates, 0.0), continental(plates, 0.0);
    for (CrustPiece& piece : globe.pieces) {
        const Vec3 centre = centreOf(*globe.grid, piece);
        piece.plate = uint16_t(plateAt(globe.epoch, centre));
        const double along = dot(globe.epoch.plates[piece.plate].pole, centre);
        area[piece.plate] += piece.area;
        sin2[piece.plate] += piece.area * (1 - along * along);
        if (piece.column.crust > float(ContinentalLimit)) continental[piece.plate] += piece.area;
    }
    for (size_t p = 0; p < plates; ++p) {
        sin2[p] = area[p] > 0 ? sin2[p] / area[p] : 0.0;
        continental[p] = area[p] > 0 ? continental[p] / area[p] : 0.0;
    }
    setEuler(globe.epoch, sin2, continental, globe.speeds);
    globe.motion.assign(plates, GlobeMotion{});
    // The polarity of the plate's trenches for the epoch (sinksUnder; Stern 2002, docs/credits). A plate
    // with no ocean now keeps age 0, so the ocean it opens in the epoch overrides every older plate's.
    std::vector<double> ocean(plates, 0.0);   // the plate's ocean floor, m²
    for (const CrustPiece& piece : globe.pieces) {
        if (continentalPiece(piece)) continue;
        globe.motion[piece.plate].oceanAge += double(piece.column.oceanAge) * piece.area;
        ocean[piece.plate] += piece.area;
    }
    for (size_t p = 0; p < plates; ++p) globe.motion[p].oceanAge = ocean[p] > 0 ? globe.motion[p].oceanAge / ocean[p] : 0;
    globe.applied.assign(plates, Vec3{});
    for (size_t cell = 0; cell < globe.swell.value.size(); ++cell) globe.swell.value[cell] = float(swellAt(globe.epoch, globe.grid->centre[cell]));
}

void applyKinematics(Globe& globe, double dt) {
    Epoch ramps;
    ramps.start = globe.epoch.start;
    ramps.end = globe.epoch.end;
    const double activity = epochActivity(ramps, globe.time);
    std::vector<Vec3> damped(globe.epoch.plates.size());
    for (size_t p = 0; p < damped.size(); ++p) {
        GlobeMotion& motion = globe.motion[p];
        motion.resistance *= std::exp(-dt / JamMemory);
        const double jam = 1.0 + motion.resistance / JamScale;
        damped[p] = globe.epoch.plates[p].euler * (activity / (jam * jam));
    }
    globe.applied = withoutNetRotation(damped, inertiaOf(globe));
}

void rotatePieces(std::vector<CrustPiece>& pieces, const std::vector<Vec3>& applied, double dt) {
    std::vector<Rotation> step(applied.size());
    for (size_t plate = 0; plate < applied.size(); ++plate) step[plate] = rotationOf(applied[plate], dt);
    for (CrustPiece& piece : pieces) piece.rotation = renormalized(compose(step[piece.plate], piece.rotation));
}

int substepCount(double fastestRelative, double dt, double shortestEdge) {
    const double sweep = std::abs(fastestRelative) * dt / shortestEdge;
    return std::max(1, int(std::ceil(sweep)));
}

Vec3 materialOf(const CrustPiece& piece, Vec3 direction) { return rotate(inverse(piece.rotation), direction) * PlanetRadius; }

NetRotation netRotationOf(const Globe& globe) {
    const std::vector<Inertia> inertia = inertiaOf(globe);
    NetRotation result;
    for (size_t p = 0; p < inertia.size(); ++p) {
        const Vec3 turn = apply(inertia[p], globe.applied[p]);
        result.net = result.net + turn;
        result.scale += length(turn);
    }
    return result;
}

} // namespace derivean::sim

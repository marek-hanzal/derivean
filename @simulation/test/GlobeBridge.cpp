// The bridge G between the globe and the flat world (S3-design.md §8, §11 test 11): latitudes, the two
// sheets, constant fields, east, J against a finite difference of G, vectors J⁻¹v and normals Jᵀn
// (Codex V7), the poles and the folds (V6), the view's contacts closing where the globe's close, and a
// contact J⁻¹ stretches beyond reach carrying no motion into the view (lead, 27 Nov).
// Live tiles (S3e), the material space (S3m) and crustFrame over the bridge (S3e) are tested there.
#include "GlobeFixture.hpp"

#include "climate/type/Climate.hpp"
#include "core/service/Workers.hpp"
#include "geometry/fn/Cell.hpp"
#include "geometry/fn/Frame.hpp"
#include "geometry/fn/SphereGrid.hpp"
#include "planet/fn/FlatView.hpp"
#include "planet/fn/GlobeContacts.hpp"
#include "planet/fn/GlobeEvolve.hpp"
#include "planet/fn/Sample.hpp"
#include "tectonics/fn/Euler.hpp"
#include "tectonics/type/Plates.hpp"

#include <algorithm>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

constexpr double Pi = 3.141592653589793;
// Zonal.cpp: z = 0 sits at 48° north and north is towards negative z there.
constexpr double NorthZ = PlanetSpan - 42.0 / 360.0 * PlanetSpan;
constexpr double SouthZ = NorthZ - 0.5 * PlanetSpan;

double latitudeOf(Vec3 u) { return std::asin(std::clamp(u.z, -1.0, 1.0)) * 180 / Pi; }

Vec3 tangentAt(Vec3 u, Random& random) {
    const Vec3 d = random.direction();
    return d - u * dot(d, u);
}

void checkSheets() {
    expect(std::abs(latitudeAt(NorthZ) - 90) < 1e-9 && std::abs(latitudeAt(SouthZ) + 90) < 1e-9, "the poles are not where Zonal.cpp has them");
    Random random{11};
    for (int i = 0; i < 10000; ++i) {
        const Vec2 at{random.next() * PlanetSpan, random.next() * PlanetSpan};
        const Vec3 u = globeDirectionOf(at);
        expect(std::abs(latitudeOf(u) - latitudeAt(at.z)) < 1e-9, "the latitude of G(x, z) is not latitudeAt(z)", latitudeOf(u) - latitudeAt(at.z));
        // The mirror across the north pole, wrapped onto the period.
        double mirror = 2 * NorthZ - at.z;
        mirror -= std::floor(mirror / PlanetSpan) * PlanetSpan;
        expect(length(globeDirectionOf({at.x, mirror}) - u) < 1e-12, "a place and its mirror across a pole are not one point");
        const Vec2 back = flatPositionOf(u);
        expect(length(globeDirectionOf(back) - u) < 1e-12, "the flat position of a direction does not map back to it");
    }
}

void checkVectors() {
    Random random{12};
    for (int z = 0; z < PlanetCells; ++z) {
        for (int x = 0; x < PlanetCells; x += 7) {
            const Vec2 at = planetCellCenter(x, z);
            const Vec3 u = globeDirectionOf(at);
            const SurfaceFrame frame = geographicFrame(u);
            // East on the sphere is +x on both sheets, with no z.
            const Vec2 east = flatVector(at, frame.first * 0.05);
            expect(east.x > 0 && std::abs(east.z) < 1e-15, "east does not come out as +x", east.z);
            const Vec3 v = tangentAt(u, random), n = normalized(tangentAt(u, random));
            const Vec2 flatV = flatVector(at, v), flatN = flatNormal(at, n);
            expect(std::isfinite(flatV.x) && std::isfinite(flatV.z) && std::isfinite(flatN.x) && std::isfinite(flatN.z), "a vector of the view is not finite");
            // (J⁻¹ v) · (Jᵀ n) = v · n: the convergence keeps its value and its sign.
            const BridgeJacobian j = bridgeJacobianAt(at.z);
            const double scale = length(Vec2{dot(n, frame.first) * j.east, dot(n, frame.second) * j.north});
            const double flat = (flatV.x * flatN.x + flatV.z * flatN.z) * scale, round = dot(v, n);
            expect(std::abs(flat - round) < 1e-12 * (1 + length(v)), "J⁻¹v · Jᵀn is not v · n", flat - round);
            // §8's 111: the cell centre nearest a pole is at 89.484375°, cos φ = 0.0089995.
            expect(std::abs(flatV.x) <= 111.2 * length(v) / BridgeScale, "the view's east component exceeds 111 |v| / k near a pole", flatV.x);
            // Across a fold the same vector reads with the same x and the opposite z.
            for (const double pole : {NorthZ, SouthZ}) {
                double mirror = 2 * pole - at.z;
                mirror -= std::floor(mirror / PlanetSpan) * PlanetSpan;
                const Vec2 other = flatVector({at.x, mirror}, v);
                expect(std::abs(other.x - flatV.x) <= 1e-9 * std::abs(flatV.x) + 1e-18 && std::abs(other.z + flatV.z) <= 1e-9 * std::abs(flatV.z) + 1e-18,
                       "a vector across a fold is not mirrored", other.z + flatV.z);
            }
        }
    }
}

// J is dG: a step of h flat metres east or along +z moves G by J times it on the sphere, in the
// geographic frame, to the step's second order (h / R).
void checkJacobian() {
    Random random{13};
    constexpr double H = 10.0;
    for (int i = 0; i < 2000; ++i) {
        const Vec2 at{random.next() * PlanetSpan, random.next() * PlanetSpan};
        if (std::abs(latitudeAt(at.z)) > 85) continue;
        const Vec3 u = globeDirectionOf(at);
        const SurfaceFrame frame = geographicFrame(u);
        const BridgeJacobian j = bridgeJacobianAt(at.z);
        const Vec3 alongX = (globeDirectionOf({at.x + H, at.z}) - u) * (PlanetRadius / H), alongZ = (globeDirectionOf({at.x, at.z + H}) - u) * (PlanetRadius / H);
        expect(length(alongX - frame.first * j.east) < 1e-5 * BridgeScale && length(alongZ - frame.second * j.north) < 1e-5 * BridgeScale, "J is not the derivative of G",
               std::max(length(alongX - frame.first * j.east), length(alongZ - frame.second * j.north)));
    }
}

// The view's contacts close where the globe's do and point at the other plate (Jᵀn): at every contact
// of the view, the convergence a flat reader gets from the view's normal and the plates' velocities
// carried by J⁻¹, both at the mantle cell's centre, has the sign the view stores; and within 30° of the
// equator, where J is nearly isotropic (a normal is Jᵀn, a step J⁻¹d), a step of the contact's distance
// and two cells along the normal lands on another plate for nearly every contact.
void checkContactSigns() {
    Globe globe = startGlobe(42, TectonicsStart, 4, PlateSpeeds::Today);
    globe.time = 0.5 * (globe.epoch.start + globe.epoch.end);   // the plates at full pace, past the epoch's ramp
    traceGlobeContacts(globe);
    const Planet view = flatViewOf(globe);
    Epoch ramps;
    ramps.start = globe.epoch.start;
    ramps.end = globe.epoch.end;
    const double activity = epochActivity(ramps, globe.time);
    long checked = 0, across = 0;
    for (const uint32_t i : view.contactZone) {
        const ContactCell& flat = view.contact.cells[i];
        const Vec2 at = planetCellCenter(int(i % uint32_t(PlanetCells)), int(i / uint32_t(PlanetCells)));
        const int cell = indexOf(*globe.grid, cellOf(globeDirectionOf(at), globe.grid->level));
        const GlobeContact& contact = globe.contact.value[size_t(cell)];
        const Vec2 ahead{at.x + flat.normalX * (flat.distance + 2 * PlanetCell), at.z + flat.normalZ * (flat.distance + 2 * PlanetCell)};
        const Vec2 wrapped{ahead.x - std::floor(ahead.x / PlanetSpan) * PlanetSpan, ahead.z - std::floor(ahead.z / PlanetSpan) * PlanetSpan};
        if (std::abs(latitudeAt(at.z)) < 30) across += view.owner.at(int(wrapped.x / PlanetCell), int(wrapped.z / PlanetCell)) != view.owner.cells[i], ++checked;
        if (std::abs(flat.convergence) < 1e-3) continue;
        const Vec3 x = globe.grid->centre[size_t(cell)];
        const Vec2 there = flatPositionOf(x);
        const Vec3 own = globe.epoch.plates[globe.owner.value[size_t(cell)]].euler * activity, other = globe.epoch.plates[globe.owner.value[size_t(contact.across)]].euler * activity;
        const Vec2 relative = flatVector(there, velocityAt(own - other, x)), normal = flatNormal(there, contact.normal);
        const double read = relative.x * normal.x + relative.z * normal.z;
        expect((read > 0) == (flat.convergence > 0), "a contact of the view closes where the globe's opens", read);
    }
    expect(checked > 100 && double(across) > 0.9 * double(checked), "the view's contact normals do not point at the other plate", double(across) / std::max(1.0, double(checked)));
}

// A globe contact within reach whose way J⁻¹ stretches beyond it in the view (towards the poles, where
// the view's east runs long) is no contact of the view: the cell keeps the empty record, as cells outside
// the zone do, and contactAt at its centre, where it alone weighs, mixes in no convergence and no shear.
void checkStretchedContacts() {
    Globe globe = startGlobe(42, TectonicsStart, 4, PlateSpeeds::Today);
    globe.time = 0.5 * (globe.epoch.start + globe.epoch.end);
    traceGlobeContacts(globe);
    const Planet view = flatViewOf(globe);
    long stretched = 0, moving = 0;
    for (int z = 0; z < PlanetCells; ++z)
        for (int x = 0; x < PlanetCells; ++x) {
            const Vec2 at = planetCellCenter(x, z);
            const int cell = indexOf(*globe.grid, cellOf(globeDirectionOf(at), globe.grid->level));
            const GlobeContact& contact = globe.contact.value[size_t(cell)];
            if (contact.across < 0) continue;
            const Vec2 way = flatVector(at, contact.normal * double(contact.distance));
            if (std::sqrt(way.x * way.x + way.z * way.z) < 0.999 * ContactReach) continue;
            const ContactCell& flat = view.contact.at(x, z);
            if (flat.distance < ContactReach) continue;   // at the rounding of reach, the view's float decides
            ++stretched, moving += std::abs(contact.convergence) > 1e-3f || std::abs(contact.shear) > 1e-3f;
            const ContactSample sample = contactAt(view, at);
            expect(flat.distance == ContactCell{}.distance && flat.convergence == 0 && flat.shear == 0 && flat.normalX == 0 && flat.normalZ == 0,
                   "a contact the view stretches beyond reach keeps its record", flat.convergence);
            expect(sample.convergence == 0 && sample.shear == 0, "contactAt mixes in a contact beyond reach", sample.convergence);
        }
    expect(stretched > 10 && moving > 10, "no globe contact is stretched beyond reach in the view", double(stretched));
    // contactAt reads motion from the zone's records only, whatever a record beyond reach holds.
    Planet planted = view;
    ContactCell& far = planted.contact.at(3, 5);
    far.distance = 1.5f * ContactReach, far.convergence = 0.02f, far.shear = -0.01f;
    const ContactSample sample = contactAt(planted, planetCellCenter(3, 5));
    expect(sample.convergence == 0 && sample.shear == 0 && sample.distance == 1.5 * double(ContactReach), "contactAt mixes in a record beyond reach", sample.convergence);
}

void checkConstantFields() {
    Globe globe = startGlobe(42, TectonicsStart, 4, PlateSpeeds::Today);
    std::fill(globe.swell.value.begin(), globe.swell.value.end(), 7.0f);
    std::fill(globe.temperature.value.begin(), globe.temperature.value.end(), 13.0f);
    Planet view = flatViewOf(globe);
    for (size_t i = 0; i < view.swell.size(); ++i)
        expect(view.swell.cells[i] == 7.0f && view.temperature.cells[i] == 13.0f, "the view of a constant field is not constant", double(i));
    std::fill(view.temperature.cells.begin(), view.temperature.cells.end(), -4.0f);
    std::fill(view.precipitation.cells.begin(), view.precipitation.cells.end(), 0.6f);
    readClimateBack(globe, view);
    for (size_t cell = 0; cell < globe.temperature.value.size(); ++cell)
        expect(std::abs(globe.temperature.value[cell] + 4) < 1e-5 && std::abs(globe.precipitation.value[cell] - 0.6f) < 1e-6, "a constant climate does not read back constant",
               double(cell));
}

} // namespace

void testGlobeBridge() {
    checkSheets();
    checkVectors();
    checkJacobian();
    checkContactSigns();
    checkStretchedContacts();
    checkConstantFields();
}

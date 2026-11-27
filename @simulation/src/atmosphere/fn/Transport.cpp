#include "atmosphere/fn/Kernels.hpp"

namespace derivean::sim {
namespace {

// Largest share of a cell that the low band's wind may carry out of it in one wind step. Above it
// the circulation is scaled back, which keeps the leapfrog step stable whatever happens; the
// transport keeps itself stable by taking more passes.
constexpr double CourantLimit = 0.8;
constexpr double SlackMargin = 1.999;   // just under the 2 that would let a cell empty exactly

// Monotonised central slope: second order where the field is smooth, flat at an extremum.
inline double limitedSlope(double below, double here, double above) {
    const double a = here - below, b = above - here;
    const double limited = std::copysign(std::min(2 * std::min(std::abs(a), std::abs(b)), 0.5 * std::abs(a + b)), a);
    return a * b > 0 ? limited : 0.0;
}

constexpr double StableSum = 0.5;        // share of a cell leaving it per pass up to which the fluxes are stable for any wind direction
constexpr double ShearCourant = 0.7;     // share of a cell the westerly shear alone may carry across in a wind step

// Courant numbers of one band on top of the low band's rotational wind, and the largest share of
// a cell that leaves it in a wind step. The share leaving each cell is left in the band's slack.
double fillBand(Atmosphere& atmosphere, int band) {
    const int n = AirCells;
    AirWork& work = atmosphere.work;
    const double perStep = WindStep / AirCell, scale = BandWind[band];
    for (int z = 0; z < n; ++z) {
        work.alongX[size_t(band)][size_t(z)] = std::clamp(band * atmosphere.season.shear[size_t(z)] * perStep, -ShearCourant, ShearCourant);
        work.alongZ[size_t(band)][size_t(z)] = band == Low ? atmosphere.season.drift[size_t(z)] * perStep : 0.0;
    }
    double largest = 0;
    for (int z = 0; z < n; ++z) {
        const int south = wrapAir(z + 1);
        const double alongX = work.alongX[size_t(band)][size_t(z)];
        const double north = work.alongZ[size_t(band)][size_t(z)], southward = work.alongZ[size_t(band)][size_t(south)];
        for (int x = 0; x < n; ++x) {
            const double leaving = std::max(0.0, -(scale * work.courantX.at(x, z) + alongX)) +
                                   std::max(0.0, scale * work.courantX.at(wrapAir(x + 1), z) + alongX) +
                                   std::max(0.0, -(scale * work.courantZ.at(x, z) + north)) +
                                   std::max(0.0, scale * work.courantZ.at(x, south) + southward);
            largest = std::max(largest, leaving);
            work.slack[size_t(band)].at(x, z) = leaving;
        }
    }
    return largest;
}

// Cuts the wind step of a band into as many passes as keep it stable, and turns the share leaving
// each cell per pass into the slope the cell may carry.
void settleBand(Atmosphere& atmosphere, int band, double largest) {
    const int passes = std::max(1, int(std::ceil(largest / StableSum)));
    atmosphere.work.passes[size_t(band)] = passes;
    for (double& slack : atmosphere.work.slack[size_t(band)].cells) {
        const double leaving = slack / passes;
        slack = SlackMargin * std::max(0.0, 1.0 - leaving) / std::max(leaving, 1e-9);
    }
}

void fillCourant(Atmosphere& atmosphere) {
    const double perStep = WindStep / AirCell;
    for (size_t cell = 0; cell < atmosphere.work.courantX.size(); ++cell) {
        atmosphere.work.courantX.cells[cell] = atmosphere.faceX.cells[cell] * perStep;
        atmosphere.work.courantZ.cells[cell] = atmosphere.faceZ.cells[cell] * perStep;
    }
}

void carry(const Atmosphere& atmosphere, const AirField& from, AirField& to, AirField& slope, int band, Carried kind, double share);

} // namespace

void prepareTransport(Atmosphere& atmosphere) {
    fillCourant(atmosphere);
    double largest = fillBand(atmosphere, Low);
    if (largest > CourantLimit) {
        // Winds are linear in the vorticity, so scaling both time levels scales every rotational wind.
        const double calm = 0.9 * CourantLimit / largest;
        for (AirField* field : {&atmosphere.vorticity, &atmosphere.vorticityBefore})
            for (double& value : field->cells) value *= calm;
        refreshWind(atmosphere, true);
        fillCourant(atmosphere);
        largest = fillBand(atmosphere, Low);
    }
    settleBand(atmosphere, Low, largest);
    for (int band = Middle; band <= High; ++band) settleBand(atmosphere, band, fillBand(atmosphere, band));
}

void advect(const Atmosphere& atmosphere, AirField& from, AirField& to, AirField& slope, int band, Carried kind) {
    const int passes = atmosphere.work.passes[size_t(band)];
    for (int pass = 0; pass < passes; ++pass) {
        if (pass > 0) std::swap(from.cells, to.cells);
        carry(atmosphere, from, to, slope, band, kind, 1.0 / passes);
    }
}

namespace {

// One pass: `share` of the band's wind step.
void carry(const Atmosphere& atmosphere, const AirField& from, AirField& to, AirField& slope, int band, Carried kind, double share) {
    const int n = AirCells;
    const AirWork& work = atmosphere.work;
    const double scale = BandWind[band] * share;
    const AirRows& alongX = work.alongX[size_t(band)];
    const AirRows& alongZ = work.alongZ[size_t(band)];
    const bool water = kind != Carried::Temperature;
    // Cloud is carried without slopes: its passes read this flat row and leave `slope` alone.
    const bool sloped = kind != Carried::Cloud;
    static constexpr double Flat[AirCells + 2] = {};

    // Cloud fields are empty over most of the planet; rows with nothing in or next to them are skipped.
    bool empty[AirCells];
    for (int z = 0; z < n; ++z) {
        const double* here = &from.cells[size_t(z) * n];
        double most = 0;
        for (int x = 0; x < n; ++x) most = std::max(most, here[x]);
        empty[z] = water && most == 0;
    }

    // Fluxes along X row by row, and the slopes along Z the second pass needs. The row is copied
    // with its wrapped neighbours on both ends so that the loops run without index arithmetic.
    double padded[AirCells + 4], slopes[AirCells + 2], flux[AirCells + 1];
    double* const here = padded + 2;
    const double* const slopeX = sloped ? slopes : Flat;
    for (int z = 0; z < n; ++z) {
        const int up = wrapAir(z - 1), down = wrapAir(z + 1);
        double* out = &to.cells[size_t(z) * n];
        double* slopeZ = sloped ? &slope.cells[size_t(z) * n] : nullptr;
        if (empty[z] && empty[up] && empty[down]) {
            std::fill(out, out + n, 0.0);
            if (sloped) std::fill(slopeZ, slopeZ + n, 0.0);
            continue;
        }
        const double* row = &from.cells[size_t(z) * n];
        const double* north = &from.cells[size_t(up) * n];
        const double* south = &from.cells[size_t(down) * n];
        const double* slack = &work.slack[size_t(band)].cells[size_t(z) * n];
        std::copy(row, row + n, here);
        here[-2] = row[n - 2];
        here[-1] = row[n - 1];
        here[n] = row[0];
        here[n + 1] = row[1];
        // slopes[x + 1] belongs to cell x, for x from −1 to n.
        if (sloped) {
            for (int x = -1; x <= n; ++x) slopes[x + 1] = limitedSlope(here[x - 1], here[x], here[x + 1]);
            for (int x = 0; x < n; ++x) slopeZ[x] = limitedSlope(north[x], here[x], south[x]);
        }
        if (kind == Carried::Vapour) {
            for (int x = 0; x < n; ++x) {
                const double most = here[x] * slack[x];
                slopes[x + 1] = std::clamp(slopes[x + 1], -most, most);
                slopeZ[x] = std::clamp(slopeZ[x], -most, most);
            }
            slopes[0] = slopes[n];
            slopes[n + 1] = slopes[1];
        }
        const double* courant = &work.courantX.cells[size_t(z) * n];
        const double shear = alongX[size_t(z)] * share;
        for (int x = 0; x < n; ++x) {
            const double c = scale * courant[x] + shear;
            const double fromWest = here[x - 1] + 0.5 * (1 - c) * slopeX[x], fromEast = here[x] - 0.5 * (1 + c) * slopeX[x + 1];
            flux[x] = c * (c > 0 ? fromWest : fromEast);
        }
        flux[n] = flux[0];
        for (int x = 0; x < n; ++x) out[x] = here[x] + flux[x] - flux[x + 1];
    }

    // Fluxes along Z through the north face of every row.
    for (int z = 0; z < n; ++z) {
        const int upper = wrapAir(z - 1);
        if (empty[z] && empty[upper]) continue;
        const double* row = &from.cells[size_t(z) * n];
        const double* north = &from.cells[size_t(upper) * n];
        const double* slopeHere = sloped ? &slope.cells[size_t(z) * n] : Flat;
        const double* slopeNorth = sloped ? &slope.cells[size_t(upper) * n] : Flat;
        const double* courant = &work.courantZ.cells[size_t(z) * n];
        double* out = &to.cells[size_t(z) * n];
        double* outNorth = &to.cells[size_t(upper) * n];
        const double drift = water ? alongZ[size_t(z)] * share : 0.0;
        for (int x = 0; x < n; ++x) {
            const double c = scale * courant[x] + drift;
            const double fromNorth = north[x] + 0.5 * (1 - c) * slopeNorth[x], fromSouth = row[x] - 0.5 * (1 + c) * slopeHere[x];
            const double carried = c * (c > 0 ? fromNorth : fromSouth);
            out[x] += carried;
            outNorth[x] -= carried;
        }
    }
}

} // namespace

} // namespace derivean::sim

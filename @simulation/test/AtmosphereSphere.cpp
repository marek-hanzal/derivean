// The weather grid on the sphere (S4a): tables, the discrete operators and their errors by place.
// The Poisson solve is in AtmosphereSpherePoisson.cpp, the flow and the tracer in
// AtmosphereSphereFlow.cpp, the eddy band in AtmosphereSphereBand.cpp.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/fn/SphereOperators.hpp"
#include "atmosphere/fn/StencilMatrix.hpp"
#include "geometry/fn/Edge.hpp"

#include <cstdint>
#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Sphere = 4 * std::numbers::pi * PlanetRadius * PlanetRadius;

std::vector<double> noise(size_t count, uint64_t seed) {
    std::vector<double> values(count);
    for (double& value : values) {
        seed = seed * 6364136223846793005ull + 1442695040888963407ull;
        value = double(seed >> 11) / double(1ull << 53) - 0.5;
    }
    return values;
}

Vec3 midpointOf(const SphereGrid& grid, int e) {
    const SphereEdge& edge = grid.edge[size_t(e)];
    int k = 0;
    while (grid.edges[size_t(edge.left)][size_t(k)] != e) ++k;
    return edgeOf(cellAt(grid, edge.left), k).midpoint;
}

// Cells, edges and vertices of a closed cubed sphere; areas and diamonds both tile it; vertex weights
// sum to one; eight vertices have three cells.
void checkGrid() {
    for (int level : {0, 3, 5}) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereStencil stencil = makeSphereStencil(grid);
        const size_t n = size_t(grid.side) * size_t(grid.side);
        expect(grid.edge.size() == 12 * n && grid.vertex.size() == 6 * n + 2, "grid does not have 12 n² edges and 6 n² + 2 vertices", level);
        double areas = 0, diamonds = 0;
        for (double area : grid.area) areas += area;
        for (const EdgeStencil& edge : stencil.edge) diamonds += edge.diamond;
        expect(std::abs(areas / Sphere - 1) < 1e-12, "cell areas do not tile the sphere", areas / Sphere - 1);
        expect(std::abs(diamonds / Sphere - 1) < 1e-12, "edge diamonds do not tile the sphere", diamonds / Sphere - 1);
        int corners = 0;
        for (size_t v = 0; v < grid.vertex.size(); ++v) {
            const SphereVertex& vertex = grid.vertex[v];
            double sum = 0;
            for (int k = 0; k < vertex.count; ++k) sum += stencil.weight[v][size_t(k)];
            expect(std::abs(sum - 1) < 1e-12, "vertex weights do not sum to one", sum);
            corners += vertex.count == 3;
        }
        expect(corners == 8, "the grid does not have eight cube vertices", corners);
    }
}

// The six faces' interiors agree to rounding; shared from face 0 they keep K symmetric bit for bit
// and positive, the split form stores one face and still gives the shared matrix's product bit for
// bit, in applyMatrix's line-major pass as in forEachProduct's row order, and the product moves by
// rounding only.
void checkSharedFaces() {
    for (int level = 2; level <= 7; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const SphereMatrix full = makeLaplacian(grid, makeSphereStencil(grid));
        SphereMatrix shared = full;
        const double differ = shareFaceInteriors(shared, grid.side);
        expect(grid.side >= 5 ? differ >= 0 && differ < 1e-12 : differ < 0, "the faces' interiors differ by more than rounding", differ);
        double largest = 0;
        for (int r = 0; r < grid.cells; ++r)
            for (int e = shared.start[size_t(r)]; e < shared.start[size_t(r) + 1]; ++e) {
                const int c = shared.column[size_t(e)];
                const auto row = shared.column.begin() + shared.start[size_t(c)], end = shared.column.begin() + shared.start[size_t(c) + 1];
                const auto found = std::lower_bound(row, end, r);
                expect(found != end && *found == r && shared.value[size_t(found - shared.column.begin())] == shared.value[size_t(e)],
                       "the shared stiffness is not symmetric bit for bit", r);
                if (c == r) expect(shared.diagonal[size_t(r)] == shared.value[size_t(e)], "the shared diagonal is not the matrix's", r);
                largest = std::max(largest, std::abs(shared.value[size_t(e)]));
            }
        const StencilMatrix split = makeStencilMatrix(shared, grid.side);
        expect(split.shared == (grid.side >= 5) && split.inner.size() * 6 == size_t(21 * split.interior), "the split form does not share one face",
               level);
        const std::vector<double> field = noise(size_t(grid.cells), uint64_t(level + 10));
        std::vector<double> before, after, splitProduct;
        applyMatrix(full, field, before);
        applyMatrix(shared, field, after);
        applyMatrix(split, field, splitProduct);
        expect(splitProduct == after, "the shared split product differs from the shared matrix's", level);
        std::vector<double> fused(field.size());
        forEachProduct(split, field.data(), [&fused](size_t r, double sum) { fused[r] = sum; });
        expect(fused == splitProduct, "forEachProduct differs from the shared split product", level);
        double moved = 0, energy = 0;
        for (int c = 0; c < grid.cells; ++c) {
            moved = std::max(moved, std::abs(after[size_t(c)] - before[size_t(c)]));
            energy += field[size_t(c)] * after[size_t(c)];
        }
        expect(moved < 1e-12 * largest, "sharing the faces moves the product by more than rounding", moved / largest);
        expect(energy > 0, "the shared stiffness is not positive", energy);
    }
}

// K is symmetric bit for bit, zero on constants and positive elsewhere; its Rayleigh quotient on the
// degree-1 harmonic is the eigenvalue 2 / R².
void checkStiffness() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    const SphereMatrix k = makeLaplacian(grid, stencil);
    double largest = 0;
    for (int r = 0; r < grid.cells; ++r)
        for (int e = k.start[size_t(r)]; e < k.start[size_t(r) + 1]; ++e) {
            const int c = k.column[size_t(e)];
            const auto row = k.column.begin() + k.start[size_t(c)], end = k.column.begin() + k.start[size_t(c) + 1];
            const auto found = std::lower_bound(row, end, r);
            expect(found != end && *found == r, "stiffness pattern is not symmetric", r);
            expect(k.value[size_t(found - k.column.begin())] == k.value[size_t(e)], "stiffness is not symmetric bit for bit", r);
            largest = std::max(largest, std::abs(k.value[size_t(e)]));
        }
    std::vector<double> product;
    applyMatrix(k, std::vector<double>(size_t(grid.cells), 1.0), product);
    for (double value : product) expect(std::abs(value) < 1e-13 * largest, "constant is not in the null space", value / largest);
    for (uint64_t seed = 1; seed <= 4; ++seed) {
        const std::vector<double> field = noise(size_t(grid.cells), seed);
        applyMatrix(k, field, product);
        double energy = 0;
        for (int c = 0; c < grid.cells; ++c) energy += field[size_t(c)] * product[size_t(c)];
        expect(energy > 0, "stiffness is not positive on a non-constant field", energy);
    }
    const std::vector<double> y1 = harmonicField(grid, 1, 0);
    applyMatrix(k, y1, product);
    double energy = 0, mass = 0;
    for (int c = 0; c < grid.cells; ++c) {
        energy += y1[size_t(c)] * product[size_t(c)];
        mass += grid.area[size_t(c)] * y1[size_t(c)] * y1[size_t(c)];
    }
    const double quotient = energy / mass * PlanetRadius * PlanetRadius;
    expect(std::abs(quotient - 2) < 3e-3, "Rayleigh quotient of the degree-1 harmonic is not 2 / R²", quotient);
    // The split form the solver uses gives the same product bit for bit, from the coarsest level
    // (no structured rows) to level 7, whose face rows of 124 cells span two blocks of 64, and keeps
    // the face interiors structured.
    for (int level = 1; level <= 7; ++level) {
        const SphereGrid at = makeSphereGrid(level);
        const SphereMatrix full = makeLaplacian(at, makeSphereStencil(at));
        const StencilMatrix split = makeStencilMatrix(full, at.side);
        const int inside = at.side >= 5 ? 6 * (at.side - 4) * (at.side - 4) : 0;
        expect(split.interior == inside, "the face interiors are not structured", level);
        const std::vector<double> field = noise(size_t(at.cells), uint64_t(level));
        std::vector<double> splitProduct;
        applyMatrix(full, field, product);
        applyMatrix(split, field, splitProduct);
        expect(splitProduct == product, "the split stiffness product differs from the assembled one", level);
    }
    checkSharedFaces();
}

// One flux per edge: the area-weighted divergence of any fluxes sums to zero over the closed grid,
// the stream function's fluxes have no divergence in any cell, and a gradient has no curl.
void checkFluxes() {
    const SphereGrid grid = makeSphereGrid(5);
    const SphereStencil stencil = makeSphereStencil(grid);
    const std::vector<double> flux = noise(grid.edge.size(), 7);
    const std::vector<double> divergence = divergenceOf(grid, flux);
    double sum = 0, scale = 0;
    for (int c = 0; c < grid.cells; ++c) sum += grid.area[size_t(c)] * divergence[size_t(c)];
    for (double value : flux) scale += std::abs(value);
    expect(std::abs(sum) < 1e-13 * scale, "area-weighted divergence does not sum to zero", sum / scale);

    const std::vector<double> stream = noise(size_t(grid.cells), 9);
    const std::vector<double> still = divergenceOf(grid, streamFluxOf(grid, stencil, stream));
    const std::vector<double> curl = curlOf(grid, stencil, gradientOf(grid, stencil, stream));
    for (int c = 0; c < grid.cells; ++c) {
        expect(std::abs(still[size_t(c)] * grid.area[size_t(c)]) < 1e-14, "stream-function flux has divergence", still[size_t(c)]);
        expect(std::abs(curl[size_t(c)] * grid.area[size_t(c)]) < 1e-14, "gradient has curl", curl[size_t(c)]);
    }
}

// The gradient of z at the edge midpoints, and the Laplacian of the degree-1 and degree-4 harmonics,
// on levels 5, 6 and 7. Inside a face both are second order (the error falls fourfold per level).
// In the two-cell band along face edges the edge gradient is first order: there the chord between
// the two centres misses the edge's midpoint by O(h), because the two faces' grid lines meet the edge
// obliquely. The Laplacian's local error there stays O(1) (about 0.1 of the value); the Poisson
// solution is second order there too (AtmosphereSpherePoisson.cpp), so the error does not spread.
void checkTruncation() {
    double gradient[3][3] = {}, laplacian[2][3][3] = {};
    for (int step = 0; step < 3; ++step) {
        const SphereGrid grid = makeSphereGrid(5 + step);
        const SphereStencil stencil = makeSphereStencil(grid);
        const SphereMatrix k = makeLaplacian(grid, stencil);
        const EdgeVectors g = gradientOf(grid, stencil, harmonicField(grid, 1, 0));
        for (size_t e = 0; e < g.size(); ++e) {
            const SphereEdge& edge = grid.edge[e];
            const Vec3 m = midpointOf(grid, int(e));
            const Vec3 exact = (Vec3{0, 0, 1} - m * m.z) * (std::sqrt(3.0) / PlanetRadius);   // Y_10 = √3 z
            const int band = std::max(bandOf(grid, edge.left), bandOf(grid, edge.right));
            gradient[step][band] = std::max(gradient[step][band], length(g[e] - exact) * PlanetRadius);
        }
        for (int d = 0; d < 2; ++d) {
            const int degree = d == 0 ? 1 : 4;
            const std::vector<double> u = harmonicField(grid, degree, d == 0 ? 0 : 2);
            std::vector<double> scaled = laplacianOf(grid, k, u);
            for (double& value : scaled) value /= laplacianEigenvalue(degree);
            const PlaceErrors errors = errorsOf(grid, scaled, u);
            for (int band = 0; band < 3; ++band) laplacian[d][step][band] = errors.worst[band];
        }
    }
    for (int step = 1; step < 3; ++step) {
        const double inside = gradient[step - 1][0] / gradient[step][0];
        const double seam = std::min(gradient[step - 1][1] / gradient[step][1], gradient[step - 1][2] / gradient[step][2]);
        expect(inside > 3.5, "edge gradient is not second order inside the faces", inside);
        expect(seam > 1.8, "edge gradient is not first order at face edges", seam);
        for (int d = 0; d < 2; ++d) {
            const double ratio = laplacian[d][step - 1][0] / laplacian[d][step][0];
            expect(ratio > 3.5, "Laplacian is not second order inside the faces", ratio);
            expect(laplacian[d][step][1] < 0.2 && laplacian[d][step][2] < 0.2, "Laplacian's local error at face edges exceeds 0.2",
                   std::max(laplacian[d][step][1], laplacian[d][step][2]));
        }
    }
    expect(laplacian[0][2][0] < 4e-4, "degree-1 Laplacian inside the faces is off by more than 4e-4 at level 7", laplacian[0][2][0]);
}

} // namespace

void testAtmosphereSphere() {
    checkGrid();
    checkStiffness();
    checkFluxes();
    checkTruncation();
    checkPoisson();
    checkMultigrid();
    checkFlow();
    checkTracer();
    checkDrift();
    checkStep();
    checkForcing();
    checkHour();
    checkRollback();
    checkInPlace();
    checkDamping();
    checkClosures();
    checkSeason();
    checkSeasonInputs();
    checkEddyBand();
    checkLatitudeBands();
}

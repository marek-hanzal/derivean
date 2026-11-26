#include "atmosphere/fn/Spectral.hpp"

namespace derivean::sim {
namespace {

constexpr double Pi = 3.141592653589793;

// In-place radix-2 transform of one line of interleaved complex numbers.
void transformLine(const PoissonSolver& solver, double* data, double direction) {
    const int n = solver.size;
    for (int i = 0; i < n; ++i) {
        const int j = solver.reversed[size_t(i)];
        if (i < j) {
            std::swap(data[2 * i], data[2 * j]);
            std::swap(data[2 * i + 1], data[2 * j + 1]);
        }
    }
    for (int span = 1; span < n; span <<= 1) {
        const int stride = n / (2 * span);
        for (int start = 0; start < n; start += 2 * span) {
            for (int k = 0; k < span; ++k) {
                const double wr = solver.cosine[size_t(k * stride)], wi = direction * solver.sine[size_t(k * stride)];
                double* a = data + 2 * (start + k);
                double* b = a + 2 * span;
                const double tr = wr * b[0] - wi * b[1], ti = wr * b[1] + wi * b[0];
                b[0] = a[0] - tr;
                b[1] = a[1] - ti;
                a[0] += tr;
                a[1] += ti;
            }
        }
    }
}

// The same transform down the columns of the half spectrum; every butterfly acts on two whole rows.
void transformColumns(const PoissonSolver& solver, double* rows, double direction) {
    const int n = solver.size, width = n + 2;
    for (int i = 0; i < n; ++i) {
        const int j = solver.reversed[size_t(i)];
        if (i < j) std::swap_ranges(rows + i * width, rows + (i + 1) * width, rows + j * width);
    }
    for (int span = 1; span < n; span <<= 1) {
        const int stride = n / (2 * span);
        for (int start = 0; start < n; start += 2 * span) {
            for (int k = 0; k < span; ++k) {
                const double wr = solver.cosine[size_t(k * stride)], wi = direction * solver.sine[size_t(k * stride)];
                double* __restrict a = rows + (start + k) * width;
                double* __restrict b = a + span * width;
                for (int m = 0; m < width; m += 2) {
                    const double tr = wr * b[m] - wi * b[m + 1], ti = wr * b[m + 1] + wi * b[m];
                    b[m] = a[m] - tr;
                    b[m + 1] = a[m + 1] - ti;
                    a[m] += tr;
                    a[m + 1] += ti;
                }
            }
        }
    }
}

// Scales the half spectrum wave by wave and transforms it back into a real field.
void synthesize(PoissonSolver& solver, const double* waves, const std::vector<double>& scale, double* spectrum, Field<double>& result) {
    const int n = solver.size, half = n / 2, width = n + 2;
    double* line = solver.line.data();
    for (size_t wave = 0; wave < scale.size(); ++wave) {
        spectrum[2 * wave] = waves[2 * wave] * scale[wave];
        spectrum[2 * wave + 1] = waves[2 * wave + 1] * scale[wave];
    }
    transformColumns(solver, spectrum, 1.0);

    // The missing half of each row spectrum is the conjugate mirror of the half that was kept.
    for (int z = 0; z < n; z += 2) {
        const double* waveA = spectrum + size_t(z) * size_t(width);
        const double* waveB = waveA + width;
        for (int k = 0; k <= half; ++k) {
            line[2 * k] = waveA[2 * k] - waveB[2 * k + 1];
            line[2 * k + 1] = waveA[2 * k + 1] + waveB[2 * k];
        }
        for (int k = half + 1; k < n; ++k) {
            const int mirror = n - k;
            line[2 * k] = waveA[2 * mirror] + waveB[2 * mirror + 1];
            line[2 * k + 1] = waveB[2 * mirror] - waveA[2 * mirror + 1];
        }
        transformLine(solver, line, 1.0);
        double* a = &result.cells[size_t(z) * size_t(n)];
        double* b = a + n;
        for (int x = 0; x < n; ++x) {
            a[x] = line[2 * x];
            b[x] = line[2 * x + 1];
        }
    }
}

} // namespace

PoissonSolver makePoissonSolver(int size, double cell) {
    PoissonSolver solver;
    solver.size = size;
    int bits = 0;
    while ((1 << bits) < size) ++bits;
    solver.reversed.resize(size_t(size));
    for (int i = 0; i < size; ++i) {
        int mirrored = 0;
        for (int bit = 0; bit < bits; ++bit) mirrored |= ((i >> bit) & 1) << (bits - 1 - bit);
        solver.reversed[size_t(i)] = mirrored;
    }
    for (int k = 0; k < size / 2; ++k) {
        solver.cosine.push_back(std::cos(2 * Pi * k / size));
        solver.sine.push_back(std::sin(2 * Pi * k / size));
    }
    const int waves = size / 2 + 1;
    solver.inverse.assign(size_t(size) * size_t(waves), 0.0);
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < waves; ++x) {
            if (x == 0 && z == 0) continue;
            const double eigenvalue = (2 * std::cos(2 * Pi * x / size) + 2 * std::cos(2 * Pi * z / size) - 4) / (cell * cell);
            solver.inverse[size_t(z) * size_t(waves) + size_t(x)] = 1.0 / (eigenvalue * size * size);
        }
    }
    solver.pass.assign(solver.inverse.size(), 0.0);
    solver.spectrum.assign(size_t(size) * size_t(size + 2), 0.0);
    solver.bandSpectrum = solver.spectrum;
    solver.line.assign(size_t(2 * size), 0.0);
    return solver;
}

void solvePoisson(PoissonSolver& solver, const Field<double>& source, Field<double>& result, Field<double>* band) {
    const int n = solver.size, half = n / 2, width = n + 2;
    double* line = solver.line.data();

    // Two real rows ride through one complex transform as its real and imaginary parts.
    for (int z = 0; z < n; z += 2) {
        const double* a = &source.cells[size_t(z) * size_t(n)];
        const double* b = a + n;
        for (int x = 0; x < n; ++x) {
            line[2 * x] = a[x];
            line[2 * x + 1] = b[x];
        }
        transformLine(solver, line, -1.0);
        double* waveA = &solver.spectrum[size_t(z) * size_t(width)];
        double* waveB = waveA + width;
        for (int k = 0; k <= half; ++k) {
            const int mirror = (n - k) & (n - 1);
            const double zr = line[2 * k], zi = line[2 * k + 1], cr = line[2 * mirror], ci = line[2 * mirror + 1];
            waveA[2 * k] = 0.5 * (zr + cr);
            waveA[2 * k + 1] = 0.5 * (zi - ci);
            waveB[2 * k] = 0.5 * (zi + ci);
            waveB[2 * k + 1] = 0.5 * (cr - zr);
        }
    }
    transformColumns(solver, solver.spectrum.data(), -1.0);
    if (band) synthesize(solver, solver.spectrum.data(), solver.pass, solver.bandSpectrum.data(), *band);
    synthesize(solver, solver.spectrum.data(), solver.inverse, solver.spectrum.data(), result);
}

} // namespace derivean::sim

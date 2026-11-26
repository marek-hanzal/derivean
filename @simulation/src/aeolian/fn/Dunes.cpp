#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"
#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr float Least = 1e-4f;       // m of sand below which a cell is bare
constexpr float OnSand = 0.6f;       // chance that a slab landing on sand in the wind stays
constexpr float OnBare = 0.4f;       // the same on a bare floor, which grains bounce off more readily
constexpr float GustSwing = 0.2f;    // rad to either side of the wind's heading that single hops stray
constexpr int MostHops = 256;
// Wind is faster over ground that stands above its surroundings and slack in a hollow: a slab lifted
// from a high cell travels farther (Momiji et al. 2000). That spreads the loss of a dune over its
// whole windward side, which keeps that side long and gentle.
constexpr float SpeedUp = 0.5f;      // gain of the hop per SpeedHeight of height above the surrounding ground
constexpr float SpeedHeight = 2.5f;  // cells
constexpr int Surround = 16;         // cells to each side over which the surrounding ground is averaged
constexpr float LeastGain = 0.5f, MostGain = 2.5f;
constexpr int GainEvery = 4;         // rounds between two readings of the surrounding ground

// The draws of one round, in order. One draw gives four independent fractions.
struct Draws {
    uint64_t state;
    uint64_t bits() { return state = scramble(state); }
};

float fraction(uint64_t bits, int which) { return float((bits >> (16 * which)) & 0xffff) * (1.0f / 65536.0f); }

struct Blow {
    SandBed& bed;
    const DuneRun& run;
    std::vector<uint8_t> shadow;
    std::vector<float> line, gain, sum;
    std::vector<int32_t> stack;
    Draws draws{};
    Vec2 heading;
};

int wrapped(int index, int size) { return index < 0 ? index + size : index >= size ? index - size : index; }

// How much farther than over level ground a slab lifted from each cell travels, from how far the
// cell stands above the mean of the bed within Surround cells; the bed continues across its edges.
void speedUp(Blow& blow) {
    const SandBed& bed = blow.bed;
    const int size = bed.size, width = 2 * Surround + 1;
    blow.gain.resize(bed.cells());
    blow.sum.resize(bed.cells());
    for (int z = 0; z < size; ++z) {
        float sum = 0;
        for (int x = -Surround; x <= Surround; ++x) sum += bed.top(bed.index(wrapped(x, size), z));
        for (int x = 0; x < size; ++x) {
            blow.sum[bed.index(x, z)] = sum;
            sum += bed.top(bed.index(wrapped(x + Surround + 1, size), z)) - bed.top(bed.index(wrapped(x - Surround, size), z));
        }
    }
    const float perHeight = SpeedUp / (SpeedHeight * float(bed.cell));
    for (int x = 0; x < size; ++x) {
        float sum = 0;
        for (int z = -Surround; z <= Surround; ++z) sum += blow.sum[bed.index(x, wrapped(z, size))];
        for (int z = 0; z < size; ++z) {
            const size_t i = bed.index(x, z);
            blow.gain[i] = std::clamp(1.0f + perHeight * (bed.top(i) - sum / float(width * width)), LeastGain, MostGain);
            sum += blow.sum[bed.index(x, wrapped(z + Surround + 1, size))] - blow.sum[bed.index(x, wrapped(z - Surround, size))];
        }
    }
}

// Carries a slab from cell (x, z) until it comes to rest. A slab lifted from a held cell is the
// neighbour's sand arriving; one that ends on a held or wet cell has left.
void carry(Blow& blow, int x, int z, float amount, bool arriving) {
    SandBed& bed = blow.bed;
    const int size = bed.size;
    const float climb = ClimbTan * float(bed.cell), area = float(bed.cell * bed.cell);
    const float headX = float(blow.heading.x), headZ = float(blow.heading.z);
    for (int hops = 0; hops < MostHops; ++hops) {
        const uint64_t bits = blow.draws.bits();
        const float hop = float(blow.run.hop) * blow.gain[bed.index(x, z)];
        // No two gusts blow quite the same way, and the hop is rounded to whole cells by a draw, so
        // its mean keeps the true heading while neighbouring lines of cells trade sand.
        const float swing = (fraction(bits, 0) - 0.5f) * 2 * GustSwing;
        const int stepX = int(std::floor((headX - headZ * swing) * hop + fraction(bits, 1)));
        const int stepZ = int(std::floor((headZ + headX * swing) * hop + fraction(bits, 2)));
        const int strides = std::max(std::abs(stepX), std::abs(stepZ));
        const float perStride = strides > 0 ? 1.0f / float(strides) : 0.0f;
        bool stopped = false;
        int cx = x, cz = z;   // not wrapped: across the edge the bed continues as another piece of the field, and no step is read there
        for (int s = 1; s <= strides && !stopped; ++s) {
            const int nx = x + int(std::floor(float(stepX * s) * perStride + 0.5f)), nz = z + int(std::floor(float(stepZ * s) * perStride + 0.5f));
            const size_t next = bed.index(wrapped(nx, size), wrapped(nz, size));
            if (bed.kind[next] == BedCell::Wall) continue;
            const bool known = nx >= 0 && nz >= 0 && nx < size && nz < size && cx >= 0 && cz >= 0 && cx < size && cz < size;
            if (known && bed.top(next) - bed.top(bed.index(cx, cz)) > climb * float(std::hypot(float(nx - cx), float(nz - cz)))) stopped = true;
            else cx = nx, cz = nz;
        }
        x = wrapped(cx, size), z = wrapped(cz, size);
        const size_t here = bed.index(x, z);
        const BedCell kind = bed.kind[here];
        if (kind == BedCell::Held || kind == BedCell::Wet) {
            if (!arriving) (kind == BedCell::Held ? bed.ledger.left : bed.ledger.drowned) += double(amount) * area;
            return;
        }
        if (kind == BedCell::Wall) continue;
        const float stays = stopped || blow.shadow[here] || hops + 1 == MostHops ? 1.0f : bed.sand[here] > Least ? OnSand : OnBare;
        if (fraction(bits, 3) >= stays) continue;
        bed.sand[here] += amount;
        if (arriving) bed.ledger.entered += double(amount) * area;
        slideSand(bed, here, blow.stack);
        return;
    }
}

} // namespace

std::vector<int> windTurns(const WindRegime& regime, int year) {
    std::vector<int> turns;
    if (regime.potential <= 0) return turns;
    // Largest remainders: the rounds add up to the year and follow the shares as closely as whole rounds can.
    int given[RoseSectors], total = 0;
    double rest[RoseSectors];
    for (int k = 0; k < RoseSectors; ++k) {
        const double exact = year * regime.drift[size_t(k)] / regime.potential;
        given[k] = int(exact), rest[k] = exact - given[k], total += given[k];
    }
    for (; total < year; ++total) {
        int most = 0;
        for (int k = 1; k < RoseSectors; ++k)
            if (rest[k] > rest[most]) most = k;
        ++given[most], rest[most] = -1;
    }
    for (int k = 0; k < RoseSectors; ++k) turns.insert(turns.end(), size_t(given[k]), k);
    return turns;
}

void blowBed(SandBed& bed, const WindRegime& regime, const DuneRun& run) {
    const std::vector<int> turns = windTurns(regime, run.year);
    if (turns.empty() || run.rounds <= 0) return;
    const size_t cells = bed.cells();
    Blow blow{bed, run};
    slideAll(bed);
    for (int round = 0; round < run.rounds; ++round) {
        blow.heading = sectorHeading(turns[size_t(round) % turns.size()]);
        castShadow(bed, blow.heading, blow.line, blow.shadow);
        if (round % GainEvery == 0) speedUp(blow);
        blow.draws.state = hash(bed.seed, round);
        for (size_t poll = 0; poll < cells; ++poll) {
            const size_t from = size_t((blow.draws.bits() >> 11) % cells);
            const BedCell kind = bed.kind[from];
            if (kind == BedCell::Wet || kind == BedCell::Wall || blow.shadow[from] || bed.sand[from] < Least) continue;
            const float amount = std::min(run.slab, bed.sand[from]);
            if (kind == BedCell::Open) {
                bed.sand[from] -= amount;
                slideSand(bed, from, blow.stack);
            }
            carry(blow, int(from % size_t(bed.size)), int(from / size_t(bed.size)), amount, kind == BedCell::Held);
        }
    }
    slideAll(bed);
}

} // namespace derivean::sim

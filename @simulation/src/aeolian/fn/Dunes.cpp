#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"
#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr float Least = 1e-4f;       // m of sand below which a cell is bare
constexpr double OnSand = 0.6;       // chance that a slab landing on sand in the wind stays
constexpr double OnBare = 0.4;       // the same on a bare floor, which grains bounce off more readily
constexpr int MostHops = 256;

// The draws of one round, in order.
struct Draws {
    uint64_t state;
    double next() { return unit(state = scramble(state)); }
};

struct Blow {
    SandBed& bed;
    const DuneRun& run;
    const std::vector<uint8_t>& shadow;
    Draws& draws;
    std::vector<int32_t>& stack;
    Vec2 heading;
    float area;
};

int wrapped(int index, int size) { return index < 0 ? index + size : index >= size ? index - size : index; }

// Carries a slab from cell (x, z) until it comes to rest. A slab lifted from a held cell is the
// neighbour's sand arriving; one that ends on a held or wet cell has left.
void carry(Blow& blow, int x, int z, float amount, bool arriving) {
    SandBed& bed = blow.bed;
    const int size = bed.size;
    const float climb = ClimbTan * float(bed.cell);
    for (int hops = 0; hops < MostHops; ++hops) {
        // The hop is rounded to whole cells by a draw, so its mean keeps the true heading.
        const int stepX = int(std::floor(blow.heading.x * blow.run.hop + blow.draws.next()));
        const int stepZ = int(std::floor(blow.heading.z * blow.run.hop + blow.draws.next()));
        const int strides = std::max(std::abs(stepX), std::abs(stepZ));
        bool stopped = false;
        int cx = x, cz = z;
        for (int s = 1; s <= strides && !stopped; ++s) {
            const int nx = wrapped(x + int(std::lround(double(stepX) * s / strides)), size);
            const int nz = wrapped(z + int(std::lround(double(stepZ) * s / strides)), size);
            const size_t next = bed.index(nx, nz);
            if (bed.kind[next] == BedCell::Wall) continue;
            const float reach = float(std::hypot(nx - cx, nz - cz));
            // Across the edge the bed continues as another piece of the field: no step is read there.
            if (reach < 3 && bed.top(next) - bed.top(bed.index(cx, cz)) > climb * reach) stopped = true;
            else cx = nx, cz = nz;
        }
        x = cx, z = cz;
        const size_t here = bed.index(x, z);
        const BedCell kind = bed.kind[here];
        if (kind == BedCell::Held || kind == BedCell::Wet) {
            if (!arriving) (kind == BedCell::Held ? bed.ledger.left : bed.ledger.drowned) += double(amount) * blow.area;
            return;
        }
        if (kind == BedCell::Wall) continue;
        const double stays = stopped || blow.shadow[here] || hops + 1 == MostHops ? 1.0 : bed.sand[here] > Least ? OnSand : OnBare;
        if (blow.draws.next() >= stays) continue;
        bed.sand[here] += amount;
        if (arriving) bed.ledger.entered += double(amount) * blow.area;
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
    std::vector<uint8_t> shadow;
    std::vector<float> line;
    std::vector<int32_t> stack;
    slideAll(bed);
    for (int round = 0; round < run.rounds; ++round) {
        const Vec2 heading = sectorHeading(turns[size_t(round) % turns.size()]);
        castShadow(bed, heading, line, shadow);
        Draws draws{hash(bed.seed, round)};
        Blow blow{bed, run, shadow, draws, stack, heading, float(bed.cell * bed.cell)};
        for (size_t poll = 0; poll < cells; ++poll) {
            const size_t from = std::min(cells - 1, size_t(draws.next() * double(cells)));
            const BedCell kind = bed.kind[from];
            if (kind == BedCell::Wet || kind == BedCell::Wall || shadow[from] || bed.sand[from] < Least) continue;
            const float amount = std::min(run.slab, bed.sand[from]);
            if (kind == BedCell::Open) {
                bed.sand[from] -= amount;
                slideSand(bed, from, stack);
            }
            carry(blow, int(from % size_t(bed.size)), int(from / size_t(bed.size)), amount, kind == BedCell::Held);
        }
    }
}

} // namespace derivean::sim

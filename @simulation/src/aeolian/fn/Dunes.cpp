#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"
#include "core/fn/Hash.hpp"
#include "core/service/Workers.hpp"

#include <functional>

namespace derivean::sim {
namespace {

constexpr float OnSand = 0.6f;       // share of the sand arriving on a sand bed in the wind that stays
constexpr float OnBare = 0.4f;       // the same on a bare floor, which grains bounce off more readily
constexpr float GustSwing = 0.2f;    // rad to either side of a sector's heading that the wind of single rounds strays
constexpr float GustReach = 0.4f;    // share of the hop by which the hops of single rounds are shorter or longer
constexpr float Gust = 0.5f;         // a gust lifts between this much less and this much more than the mean
constexpr int GustCells = 3;         // cells across one gust
// Wind speeds up where the ground rises gently before it, stalls before a steep rise, and slackens
// where the ground falls away. A cell gives more sand the more it faces the wind, up to a slope of
// StallSlope / 2: sand lifted on the windward side and set down a hop later is what makes a low
// swell grow into a dune. The rise is read over a hop's length, because the wind answers to the
// shape of a hill and not to the step between two cells.
constexpr float SlopeGain = 4.0f;    // gain of the lifted slab per unit of gentle windward slope
constexpr float StallSlope = 0.4f;   // windward slope at which the gain of a rise is gone again
// A cell that stands above its four neighbours gives more: this keeps swells shorter than a few
// hops from growing and a windward side smooth.
constexpr float CrestGain = 4.0f;    // gain per cell of height above the four neighbours
constexpr float LeastGain = 0.1f, MostGain = 2.5f;
// A grain hopping over ground that falls to one side lands a little down that side. Without this
// neighbouring lines of cells along the wind would each grow dunes of their own.
constexpr float Deflect = 3.0f;      // cells of sideways shift of a hop per unit of sideways slope
constexpr int LandingRounds = 12;    // rounds without lifting in which sand still in the air comes down

struct Work {
    std::vector<float> flight, arriving, taken, line, gust;
    std::vector<uint8_t> shadow;
    int gusts{};   // lattice points per side
    // Bands of rows that send their sand side by side: every other band at once, so two that run
    // together never reach the same cell. How the rows are cut follows from the bed and the hop
    // alone, never from the pool, and that fixes the order in which a cell's arrivals are summed.
    std::vector<int> band;      // first row of each band, and the bed's size after the last
    std::vector<double> entered;
};

int wrapped(int index, int size) { return index < 0 ? index + size : index >= size ? index - size : index; }

void eachRange(const SandBed& bed, int count, const std::function<void(int, int)>& body) {
    if (bed.workers && count > 1) bed.workers->parallelFor(count, body);
    else body(0, count);
}

// Rows of the bed in bands at least twice as tall as the farthest a hop can carry, in an even
// number so that the bands either side of the seam of the torus run apart too. One band where the
// bed is too small for two.
void cutBands(const SandBed& bed, const DuneRun& run, Work& work) {
    const int reach = int(std::ceil(run.hop * (1.0 + GustReach) + 0.7 * Deflect)) + 2;
    int count = bed.size / (2 * reach);
    count -= count % 2;
    work.band.clear();
    if (count < 2) {
        work.band = {0, bed.size};
    } else {
        for (int k = 0; k < count; ++k) work.band.push_back(k * (bed.size / count));
        work.band.push_back(bed.size);
    }
    work.entered.assign(work.band.size() - 1, 0.0);
}

float topAt(const SandBed& bed, int x, int z) { return bed.top(bed.index(wrapped(x, bed.size), wrapped(z, bed.size))); }

// Gusts are wider than a cell: their strength is seeded on a lattice and renewed every round.
void drawGusts(const SandBed& bed, Work& work, uint64_t round) {
    work.gusts = bed.size / GustCells + 2;
    work.gust.resize(size_t(work.gusts) * size_t(work.gusts));
    for (size_t k = 0; k < work.gust.size(); ++k) work.gust[k] = 1.0f + Gust * (2 * float(unit(hash(bed.seed, int64_t(round), int64_t(k)))) - 1);
}

// What a cell in the wind gives up this round.
float lifted(const SandBed& bed, const Work& work, int x, int z, Vec2 heading, const DuneRun& run) {
    const float here = bed.top(bed.index(x, z)), cell = float(bed.cell);
    const double backX = x - heading.x * run.hop, backZ = z - heading.z * run.hop;
    const int lowX = int(std::floor(backX)), lowZ = int(std::floor(backZ));
    const float partX = float(backX - lowX), partZ = float(backZ - lowZ);
    const float upwind = (topAt(bed, lowX, lowZ) * (1 - partX) + topAt(bed, lowX + 1, lowZ) * partX) * (1 - partZ) +
                         (topAt(bed, lowX, lowZ + 1) * (1 - partX) + topAt(bed, lowX + 1, lowZ + 1) * partX) * partZ;
    const float slope = (here - upwind) / (cell * float(run.hop));
    const float rise = slope > 0 ? slope * (1.0f - slope / StallSlope) : slope;
    const float crest = (here - 0.25f * (topAt(bed, x - 1, z) + topAt(bed, x + 1, z) + topAt(bed, x, z - 1) + topAt(bed, x, z + 1))) / cell;
    const int gx = x / GustCells, gz = z / GustCells;
    const float tx = float(x % GustCells) / GustCells, tz = float(z % GustCells) / GustCells;
    const float* row = &work.gust[size_t(gz) * size_t(work.gusts) + size_t(gx)];
    const float gust = (row[0] * (1 - tx) + row[1] * tx) * (1 - tz) + (row[work.gusts] * (1 - tx) + row[work.gusts + 1] * tx) * tz;
    return std::min(bed.sand[bed.index(x, z)], run.slab * std::clamp(1.0f + SlopeGain * rise + CrestGain * crest, LeastGain, MostGain) * gust);
}

// Every cell sends what it lifts and what is still passing over it one hop downwind, shared among
// the four cells around the landing point. Sand that crosses a brink falls out of the wind at once:
// it comes down in the first cell of a wind shadow on its way. Sand that would have to climb a step
// comes down where it is.
void send(SandBed& bed, Work& work, int x, int z, Vec2 heading, double hop, double& entered) {
    const int size = bed.size;
    const float climb = ClimbTan * float(bed.cell);
    const size_t from = bed.index(x, z);
    const BedCell kind = bed.kind[from];
    const float out = work.flight[from] + work.taken[from];
    if (out <= 0) return;
    const double aside = std::clamp(double((topAt(bed, x, z + 1) - topAt(bed, x, z - 1)) * heading.x - (topAt(bed, x + 1, z) - topAt(bed, x - 1, z)) * heading.z) / (2 * bed.cell), -0.7, 0.7) * Deflect;
    const double moveX = heading.x * hop + aside * heading.z, moveZ = heading.z * hop - aside * heading.x;
    const int strides = int(std::ceil(std::max(std::abs(moveX), std::abs(moveZ))));
    size_t fell = from;
    for (int stride = 1; stride <= strides && fell == from; ++stride) {
        const size_t over = bed.index(wrapped(x + int(std::lround(moveX * stride / strides)), size), wrapped(z + int(std::lround(moveZ * stride / strides)), size));
        if (work.shadow[over]) fell = over;
    }
    const int lowX = int(std::floor(x + moveX)), lowZ = int(std::floor(z + moveZ));
    const float partX = float(x + moveX - lowX), partZ = float(z + moveZ - lowZ);
    const size_t to[4] = {bed.index(wrapped(lowX, size), wrapped(lowZ, size)), bed.index(wrapped(lowX + 1, size), wrapped(lowZ, size)),
                          bed.index(wrapped(lowX, size), wrapped(lowZ + 1, size)), bed.index(wrapped(lowX + 1, size), wrapped(lowZ + 1, size))};
    const float share[4] = {(1 - partX) * (1 - partZ), partX * (1 - partZ), (1 - partX) * partZ, partX * partZ};
    // Across the edge the bed continues as another piece of the field: no step is read there.
    const bool inside = lowX >= 0 && lowZ >= 0 && lowX + 1 < size && lowZ + 1 < size;
    if (fell == from && inside && kind != BedCell::Wall) {
        float ahead = 0;
        for (int c = 0; c < 4; ++c) ahead += share[c] * bed.top(to[c]);
        if (ahead - bed.top(from) > climb * float(std::hypot(moveX, moveZ))) {
            work.taken[from] = kind == BedCell::Open ? -work.flight[from] : 0.0f;
            return;
        }
    }
    if (kind == BedCell::Held) entered += work.taken[from] * bed.cell * bed.cell;
    if (fell != from) work.arriving[fell] += out;
    else
        for (int c = 0; c < 4; ++c) work.arriving[to[c]] += out * share[c];
}

void launch(SandBed& bed, Work& work, Vec2 heading, double hop, const DuneRun& run, bool lifting) {
    const int size = bed.size;
    // What every cell gives is settled before any of it moves, and the bed changes only after all
    // of it has been aimed: no cell's share or path depends on the order of the sweep.
    eachRange(bed, size, [&](int begin, int end) {
        for (int z = begin; z < end; ++z)
            for (int x = 0; x < size; ++x) {
                const size_t i = bed.index(x, z);
                const bool gives = lifting && !work.shadow[i] && bed.sand[i] > 0;
                work.taken[i] = !gives ? 0.0f : bed.kind[i] == BedCell::Open ? lifted(bed, work, x, z, heading, run) : bed.kind[i] == BedCell::Held ? run.slab : 0.0f;
            }
    });
    std::fill(work.arriving.begin(), work.arriving.end(), 0.0f);
    const int bands = int(work.band.size()) - 1;
    for (int parity = 0; parity < 2; ++parity) {
        eachRange(bed, (bands + 1 - parity) / 2, [&](int begin, int end) {
            for (int pick = begin; pick < end; ++pick) {
                const int band = 2 * pick + parity;
                for (int z = work.band[size_t(band)]; z < work.band[size_t(band) + 1]; ++z)
                    for (int x = 0; x < size; ++x) send(bed, work, x, z, heading, hop, work.entered[size_t(band)]);
            }
        });
    }
    eachRange(bed, size, [&](int begin, int end) {
        for (size_t i = bed.index(0, begin); i < bed.index(0, end); ++i)
            if (bed.kind[i] == BedCell::Open) bed.sand[i] -= work.taken[i];
    });
}

// Of what arrives, a cell keeps all in a wind shadow, most on sand and less on a bare floor; the
// rest passes on in the next round. `last` brings everything down.
void land(SandBed& bed, Work& work, float slab, bool last) {
    const double area = bed.cell * bed.cell;
    for (size_t i = 0; i < bed.cells(); ++i) {
        const float arrived = work.arriving[i];
        work.flight[i] = 0;
        if (arrived <= 0) continue;
        switch (bed.kind[i]) {
        case BedCell::Open: {
            const float stays = last || work.shadow[i] ? 1.0f : OnBare + (OnSand - OnBare) * std::min(1.0f, bed.sand[i] / slab);
            bed.sand[i] += stays * arrived;
            work.flight[i] = (1 - stays) * arrived;
            break;
        }
        case BedCell::Held: bed.ledger.left += arrived * area; break;
        case BedCell::Wet: bed.ledger.drowned += arrived * area; break;
        case BedCell::Wall: work.flight[i] = arrived; break;
        }
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
    Work work;
    cutBands(bed, run, work);
    work.flight.assign(bed.cells(), 0.0f);
    work.arriving.assign(bed.cells(), 0.0f);
    work.taken.assign(bed.cells(), 0.0f);
    slideAll(bed);
    const int rounds = run.rounds + LandingRounds;
    for (int round = 0; round < rounds; ++round) {
        const Vec2 sector = sectorHeading(turns[size_t(round) % turns.size()]);
        // The wind of a sector swings about its heading from round to round, so that neighbouring
        // lines of cells trade sand whatever the heading is against the grid; and its gusts differ in
        // strength, or hops of one fixed length would let the bed grow a ripple of that length.
        const double swing = GustSwing * (2 * unit(hash(bed.seed, -1, round)) - 1);
        const Vec2 heading = normalized({sector.x - sector.z * swing, sector.z + sector.x * swing});
        const double hop = run.hop * (1 + GustReach * (2 * unit(hash(bed.seed, -2, round)) - 1));
        castShadow(bed, heading, work.line, work.shadow);
        drawGusts(bed, work, uint64_t(round));
        launch(bed, work, heading, hop, run, round < run.rounds);
        land(bed, work, run.slab, round + 1 == rounds);
        slidePairs(bed, round);
    }
    for (const double sent : work.entered) bed.ledger.entered += sent;
    // Sand still over the outer ring when the wind drops has left the bed.
    for (const float passing : work.flight) bed.ledger.left += double(passing) * bed.cell * bed.cell;
    slideAll(bed);
}

} // namespace derivean::sim

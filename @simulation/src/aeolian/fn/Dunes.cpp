#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"
#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr float OnSand = 0.6f;       // share of the sand arriving on a sand bed in the wind that stays
constexpr float OnBare = 0.4f;       // the same on a bare floor, which grains bounce off more readily
constexpr float GustSwing = 0.2f;    // rad to either side of a sector's heading that the wind of single rounds strays
constexpr float Gust = 0.3f;         // range of the seeded unevenness of what a round lifts, around one
// Wind speeds up where the ground rises before it and the higher a place stands above the ground
// around it, and slackens where the ground falls away and in a hollow. A cell gives more sand the
// steeper it faces the wind: sand lifted on the windward side and set down a hop later is what makes
// a low swell grow into a dune. It gives more the higher it stands over its surroundings, which
// spreads the loss over the whole windward side and keeps that side long and gentle; and more where
// it stands above its four neighbours, which keeps swells shorter than a few hops from growing.
constexpr float SlopeGain = 2.0f;    // gain of the lifted slab per unit of windward slope
constexpr float HeightGain = 8.0f;   // gain per unit of height above the surrounding ground over the width that ground is averaged across
constexpr float CrestGain = 2.0f;    // gain per cell of height above the four neighbours
constexpr int Surround = 12;         // cells to each side over which the surrounding ground is averaged
constexpr float LeastGain = 0.1f, MostGain = 2.5f;
// A grain hopping over ground that falls to one side lands a little down that side. Without this
// neighbouring lines of cells along the wind would each grow dunes of their own.
constexpr float Deflect = 3.0f;      // cells of sideways shift of a hop per unit of sideways slope and cell of hop
constexpr int LandingRounds = 12;    // rounds without lifting in which sand still in the air comes down

struct Work {
    std::vector<float> flight, arriving, taken, line, around, sum;
    std::vector<uint8_t> shadow;
};

int wrapped(int index, int size) { return index < 0 ? index + size : index >= size ? index - size : index; }

// Mean top of the bed within Surround cells of every cell, the bed continuing across its edges.
void surroundings(const SandBed& bed, Work& work) {
    const int size = bed.size, width = 2 * Surround + 1;
    work.around.resize(bed.cells());
    work.sum.resize(bed.cells());
    for (int z = 0; z < size; ++z) {
        float sum = 0;
        for (int x = -Surround; x <= Surround; ++x) sum += bed.top(bed.index(wrapped(x, size), z));
        for (int x = 0; x < size; ++x) {
            work.sum[bed.index(x, z)] = sum;
            sum += bed.top(bed.index(wrapped(x + Surround + 1, size), z)) - bed.top(bed.index(wrapped(x - Surround, size), z));
        }
    }
    for (int x = 0; x < size; ++x) {
        float sum = 0;
        for (int z = -Surround; z <= Surround; ++z) sum += work.sum[bed.index(x, wrapped(z, size))];
        for (int z = 0; z < size; ++z) {
            work.around[bed.index(x, z)] = sum / float(width * width);
            sum += work.sum[bed.index(x, wrapped(z + Surround + 1, size))] - work.sum[bed.index(x, wrapped(z - Surround, size))];
        }
    }
}

// What a cell in the wind gives up this round.
float lifted(const SandBed& bed, const Work& work, int x, int z, Vec2 heading, float slab, uint64_t round) {
    const int size = bed.size;
    const size_t i = bed.index(x, z);
    const float here = bed.top(i), cell = float(bed.cell);
    // Rise from the cell upwind, along each axis by the share of the wind that blows along it.
    const float west = bed.top(bed.index(wrapped(x - 1, size), z)), east = bed.top(bed.index(wrapped(x + 1, size), z));
    const float north = bed.top(bed.index(x, wrapped(z - 1, size))), south = bed.top(bed.index(x, wrapped(z + 1, size)));
    const float slope = float(std::abs(heading.x) * (here - (heading.x > 0 ? west : east)) + std::abs(heading.z) * (here - (heading.z > 0 ? north : south))) / cell;
    const float crest = (here - 0.25f * (west + east + north + south)) / cell;
    const float height = (here - work.around[i]) / (cell * float(Surround));
    const float gust = 1.0f + Gust * (float(unit(hash(bed.seed, int64_t(i), int64_t(round)))) - 0.5f);
    return std::min(bed.sand[i], slab * std::clamp(1.0f + SlopeGain * slope + HeightGain * height + CrestGain * crest, LeastGain, MostGain) * gust);
}

// Every cell sends what it lifts and what is still passing over it one hop downwind, shared among
// the four cells around the landing point. Sand that would have to climb a step stays where it is.
void launch(SandBed& bed, Work& work, Vec2 heading, double hop, const DuneRun& run, uint64_t round, bool lifting) {
    const int size = bed.size;
    const float area = float(bed.cell * bed.cell), climb = ClimbTan * float(bed.cell * hop);
    const double hopX = heading.x * hop, hopZ = heading.z * hop;
    // What every cell gives is settled before any of it moves, so no cell's share depends on the order of the sweep.
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = bed.index(x, z);
            const bool gives = lifting && !work.shadow[i] && bed.sand[i] > 0;
            work.taken[i] = !gives ? 0.0f : bed.kind[i] == BedCell::Open ? lifted(bed, work, x, z, heading, run.slab, round) : bed.kind[i] == BedCell::Held ? run.slab : 0.0f;
        }
    }
    std::fill(work.arriving.begin(), work.arriving.end(), 0.0f);
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t from = bed.index(x, z);
            const BedCell kind = bed.kind[from];
            const float out = work.flight[from] + work.taken[from];
            if (out <= 0) continue;
            // Slope across the wind, from the neighbours on either side along each axis.
            const float fallX = bed.top(bed.index(wrapped(x + 1, size), z)) - bed.top(bed.index(wrapped(x - 1, size), z));
            const float fallZ = bed.top(bed.index(x, wrapped(z + 1, size))) - bed.top(bed.index(x, wrapped(z - 1, size)));
            const double aside = std::clamp(double(fallZ * heading.x - fallX * heading.z) / (2 * bed.cell), -0.7, 0.7) * Deflect;
            const double landX = x + hopX + aside * heading.z, landZ = z + hopZ - aside * heading.x;
            const int lowX = int(std::floor(landX)), lowZ = int(std::floor(landZ));
            const float partX = float(landX - lowX), partZ = float(landZ - lowZ);
            const size_t to[4] = {bed.index(wrapped(lowX, size), wrapped(lowZ, size)), bed.index(wrapped(lowX + 1, size), wrapped(lowZ, size)),
                                  bed.index(wrapped(lowX, size), wrapped(lowZ + 1, size)), bed.index(wrapped(lowX + 1, size), wrapped(lowZ + 1, size))};
            const float share[4] = {(1 - partX) * (1 - partZ), partX * (1 - partZ), (1 - partX) * partZ, partX * partZ};
            // Across the edge the bed continues as another piece of the field: no step is read there.
            const bool inside = lowX >= 0 && lowZ >= 0 && lowX + 1 < size && lowZ + 1 < size;
            if (inside && kind != BedCell::Wall) {
                float ahead = 0;
                for (int c = 0; c < 4; ++c) ahead += share[c] * bed.top(to[c]);
                if (ahead - bed.top(from) > climb) {
                    if (kind == BedCell::Open) bed.sand[from] += work.flight[from];
                    continue;
                }
            }
            if (kind == BedCell::Open) bed.sand[from] -= work.taken[from];
            else if (kind == BedCell::Held) bed.ledger.entered += double(work.taken[from]) * area;
            for (int c = 0; c < 4; ++c) work.arriving[to[c]] += out * share[c];
        }
    }
}

// Of what arrives, a cell keeps all in a wind shadow, most on sand and less on a bare floor; the
// rest passes on in the next round. `last` brings everything down.
void land(SandBed& bed, Work& work, float slab, bool last) {
    const float area = float(bed.cell * bed.cell);
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
        case BedCell::Held: bed.ledger.left += double(arrived) * area; break;
        case BedCell::Wet: bed.ledger.drowned += double(arrived) * area; break;
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
    work.flight.assign(bed.cells(), 0.0f);
    work.arriving.assign(bed.cells(), 0.0f);
    work.taken.assign(bed.cells(), 0.0f);
    slideAll(bed);
    for (int round = 0; round < run.rounds + LandingRounds; ++round) {
        const bool lifting = round < run.rounds;
        const Vec2 sector = sectorHeading(turns[size_t(round) % turns.size()]);
        // The wind of a sector swings about its heading from round to round, so that neighbouring
        // lines of cells trade sand whatever the heading is against the grid.
        const double swing = GustSwing * (unit(hash(bed.seed, -1, round)) * 2 - 1);
        const Vec2 heading = normalized({sector.x - sector.z * swing, sector.z + sector.x * swing});
        // Gusts differ in strength too: hops of one fixed length would let the bed grow a ripple of that length.
        const double hop = run.hop * (0.6 + 0.8 * unit(hash(bed.seed, -2, round)));
        castShadow(bed, heading, work.line, work.shadow);
        surroundings(bed, work);
        launch(bed, work, heading, hop, run, uint64_t(round), lifting);
        land(bed, work, run.slab, round + 1 == run.rounds + LandingRounds);
        slidePairs(bed, round);
    }
    // Sand still over the outer ring when the wind drops has left the bed.
    for (const float passing : work.flight) bed.ledger.left += double(passing) * bed.cell * bed.cell;
    slideAll(bed);
}

} // namespace derivean::sim

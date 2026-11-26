#include "aeolian/fn/Wind.hpp"

#include "aeolian/constant/Wind.hpp"
#include "aeolian/fn/Dunes.hpp"
#include "aeolian/fn/Regime.hpp"
#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

// A tile's dune field is the transport model run for this much wind work on the sand its parent
// hands down: long enough for the forms of its own cell size to stand in balance with wind and
// supply (the model needs a few hundred rounds from a smooth start), far shorter than the time its
// water and slopes settle over. Dunes move on; the field is one moment of a state that keeps its
// statistics. A century, tried first, gave a Site field under a moderate wind under two hundred
// rounds: cell-sized lumps where the field should have ridges.
constexpr double Span[LevelCount] = {0, 0, 40e3, 3000.0, 0};   // yr
constexpr int Hop = 4;                  // cells
constexpr float SlabShare = 0.1f;       // of a cell: the slab of a thick field
constexpr float SlabsDeep = 8.0f;       // slabs the sand of a thinner field is deep where it lies
constexpr int MostRounds = 500;
constexpr int LeastRounds = 8;
constexpr float LeastField = 0.02f;     // m of mobile sand, as a mean over the free cells, below which no field is worked
constexpr float PassShare = 0.6f;       // share of the sand in the air that comes down per hop on sand (Dunes.cpp)
constexpr int DampReach[LevelCount] = {0, 0, 1, 2, 0};   // cells from water within which sand is held damp
constexpr uint64_t DuneStream = 0x44554e45;

// m above which sand near water is dry enough to blow; far below the ground away from water.
std::vector<float> dampLine(const Domain& domain, const std::vector<uint8_t>& wet) {
    const int size = domain.size, reach = DampReach[size_t(domain.key.level)];
    std::vector<float> line(domain.cells(), -1e9f);
    for (int z = 0; z < size; ++z)
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            if (!wet[i]) continue;
            const float water = std::max(domain.level[i], domain.surface[i]) + CapillaryRise;
            for (int oz = std::max(0, z - reach); oz <= std::min(size - 1, z + reach); ++oz)
                for (int ox = std::max(0, x - reach); ox <= std::min(size - 1, x + reach); ++ox) line[domain.index(ox, oz)] = std::max(line[domain.index(ox, oz)], water);
        }
    return line;
}

} // namespace

void shapeDunes(Domain& domain) {
    WindField& wind = domain.aeolian;
    const size_t cells = domain.cells();
    const int size = domain.size, last = size - 1;
    const float area = float(domain.cell * domain.cell);
    // Water is read off the step's flood before anything is laid on the ground it ran over.
    std::vector<uint8_t> wet(cells);
    for (size_t i = 0; i < cells; ++i) wet[i] = heldByWater(domain, i);
    const std::vector<float> damp = dampLine(domain, wet);
    // The cells of finished neighbours end on what those neighbours ended on, their dunes included.
    std::vector<float> top(cells);
    for (size_t i = 0; i < cells; ++i) {
        for (Ground* ground : {&domain.ground, &domain.settledFrom, &domain.settledTo}) ground->sediment[Sand][i] += wind.parked[i];
        bookWind(domain, i, wind.parked[i]);
        wind.parked[i] = 0;
        top[i] = domain.ground.rock[i] + domain.sedimentAt(i);
    }

    SandBed bed;
    bed.size = size, bed.cell = domain.cell, bed.reposeTan = ReposeTan[Sand], bed.workers = wind.workers;
    bed.seed = hash(DuneStream, int64_t(domain.key.level), domain.key.x, domain.key.z);
    bed.base.resize(cells), bed.sand.resize(cells), bed.kind.resize(cells);
    double mobile = 0, dry = 0;
    int open = 0, sandy = 0;
    for (int z = 0; z < size; ++z) {
        for (int x = 0; x < size; ++x) {
            const size_t i = domain.index(x, z);
            const bool ring = x == 0 || z == 0 || x == last || z == last;
            bed.kind[i] = ring ? BedCell::Wall : wet[i] ? BedCell::Wet : domain.hold[i] != Free ? BedCell::Held : BedCell::Open;
            const float loose = wet[i] ? 0.0f : std::min(mobileSand(domain, i), std::max(0.0f, top[i] - damp[i]));
            bed.sand[i] = loose, bed.base[i] = top[i] - loose;
            if (bed.kind[i] != BedCell::Open) continue;
            mobile += loose, ++open, sandy += loose > LeastField;
            dry += dryShare(domain.rain[i], domain.warmthNow[i], wind.halfRange);
        }
    }
    if (open == 0 || mobile / open < LeastField) return;
    // A thin sheet is worked in thinner slabs, or the whole of it would leave a cell at once.
    DuneRun run;
    run.hop = Hop;
    run.slab = std::clamp(float(mobile / std::max(1, sandy)) / SlabsDeep, 0.25f * SlabShare * float(domain.cell), SlabShare * float(domain.cell));
    const double perRound = run.slab * Hop * domain.cell / PassShare;   // m² of sand per metre of width a round passes over a covered bed
    const double work = Span[size_t(domain.key.level)] * wind.regime.potential * dry / open;
    run.rounds = std::min(MostRounds, int(work / perRound));
    if (run.rounds < LeastRounds) return;
    const std::vector<float> before = bed.sand;
    blowBed(bed, wind.regime, run);
    wind.rounds = run.rounds;
    wind.span = run.rounds * perRound / std::max(1e-9, wind.regime.potential * dry / open);

    for (size_t i = 0; i < cells; ++i) {
        if (bed.kind[i] != BedCell::Open) continue;
        const float change = bed.sand[i] - before[i];
        if (change >= 0) {
            layBlown(domain, i, change);
            wind.ledger.sandLaid += double(change) * area;
        } else {
            wind.ledger.dustRaised += double(takeSand(domain, i, -change)) * area;
            wind.ledger.sandLifted += double(-change) * area;
        }
    }
    // At this scale dust is gone with the wind: where it settles is decided by the Region.
    wind.ledger.dustOut = wind.ledger.dustRaised;
    wind.ledger.sandIn += bed.ledger.entered, wind.ledger.sandOut += bed.ledger.left, wind.ledger.sandDrowned += bed.ledger.drowned;
    tidyLabels(domain);
}

} // namespace derivean::sim

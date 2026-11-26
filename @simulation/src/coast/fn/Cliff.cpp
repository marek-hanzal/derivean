#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

constexpr float LeastWithstand = 0.03f;   // m; loose and rotten ground, and crushed weak rock, give way to any wave
// km a block travels to and fro in the surf while the face behind it goes back by its own width:
// what the rock's abrasion leaves of it by then is the shingle of the shore.
constexpr float SurfTravel = 60.0f;
// km for its sand: grains of quartz outlast that, grains of chalk or mudstone do not, so only
// rock that wears slowly feeds a beach.
constexpr float SandTravel = 15.0f;
constexpr float MudLine = 2.5f;    // depth over wave height below which mud can settle
constexpr float MudSink = 0.12f;   // share of a mud cloud that sinks in each cell it crosses there
constexpr int CutSlices = 8;       // most cells a face goes back by in one step

// m, the level a column is cut down to when its foot has been cut through.
float benchOf(const Domain& domain, float water) { return water - BenchDepth[size_t(domain.key.level)] - 0.5f * swingOf(domain, water); }

// m/yr a face goes back under the trains that strike it. While the cut is only a notch the waves
// reach it as they arrive; once it is a bench they break on it and lose height across it, so a
// face stops where its bench has grown wide enough to bring them below what the rock withstands.
float retreatOf(const Domain& domain, const Shore& shore, size_t i, float withstand) {
    const float water = shore.still[i], depth = water + swingOf(domain, water) - benchOf(domain, water);
    const float cutBack = shore.notch[i], benched = std::min(1.0f, cutBack / NotchBench);
    float rate = 0;
    for (int k = 0; k < WaveDirections; ++k) {
        const float arriving = std::sqrt(shore.arrive[i * WaveDirections + size_t(k)]);
        if (arriving <= withstand) continue;
        float height = arriving + (std::min(arriving, Breaker * depth) - arriving) * benched;
        height /= 1.0f + BedFriction * height * cutBack / (depth * depth);
        rate += Quarry * std::max(0.0f, height - withstand) / (withstand * withstand);
    }
    return rate / WaveDirections;
}

// Mud goes out to sea as a cloud: it is taken down the bed from cell to cell and sinks on the way
// wherever the bed lies below the reach of the waves, a share in every such cell and the rest in a
// hollow it cannot leave. Across a level bench it drifts on, away from where it came down: a bench
// the waves sweep keeps no mud. A cloud still in the surf when it can go no farther has gone out of
// the domain's reach.
void suspend(Domain& domain, Shore& shore, size_t from, float mud) {
    const int size = domain.size, fx = int(from % size_t(size)), fz = int(from / size_t(size));
    const double area = domain.cell * domain.cell;
    std::vector<size_t> path{from};
    size_t at = from;
    for (int step = 0; step < 4 * size && mud > 0; ++step) {
        const int x = int(at % size_t(size)), z = int(at / size_t(size));
        size_t next = at;
        int far = -1;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t n = domain.index(nx, nz);
            const int away = (nx - fx) * (nx - fx) + (nz - fz) * (nz - fz);
            if (shore.top[n] < shore.top[next] || (shore.top[n] == shore.top[next] && next != at && away > far) ||
                (next == at && shore.top[n] == shore.top[at] && away > (x - fx) * (x - fx) + (z - fz) * (z - fz) && std::find(path.begin(), path.end(), n) == path.end()))
                next = n, far = away;
        }
        float highest = 0;
        for (int k = 0; k < WaveDirections; ++k) highest = std::max(highest, shore.arrive[at * WaveDirections + size_t(k)]);
        const float depth = shore.still[at] == NoWater ? 0.0f : shore.still[at] - shore.top[at];
        const bool calm = depth * depth >= MudLine * MudLine * highest;
        if (next == at) {
            // A hollow: it all settles if the water there is calm; otherwise the surf takes it away.
            if (!calm) break;
            domain.ground.sediment[Fines][at] += mud;
            shore.top[at] += mud;
            return;
        }
        if (calm && at != from) {
            const float sunk = MudSink * mud;
            domain.ground.sediment[Fines][at] += sunk;
            shore.top[at] += sunk;
            mud -= sunk;
        }
        at = next;
        path.push_back(at);
        if (domain.hold[at] != Free) break;
    }
    shore.ledger.left += double(mud) * area;
}

// The column comes down: its rock above the bench breaks into the sizes it weathers to. Blocks
// are ground down in the surf by the rock's own abrasion; sand and gravel wait in the surf of the
// cell for the waves to carry them off. What stood on the column as loose cover falls with it.
// Returns the mud: it is put into the water once the waves of the new shore are known.
float bringDown(Domain& domain, Shore& shore, size_t i, float bench) {
    const float fallen = domain.ground.rock[i] - bench;
    const RockTraits& rock = domain.unit[i].traits;
    const float blocks = std::exp(-rock.abrasion * 1000.0f * SurfTravel), grains = std::exp(-rock.abrasion * 1000.0f * SandTravel);
    const float finer = rock.sand + rock.fines, ground = fallen * rock.coarse * (1.0f - blocks);
    const float sand = (fallen * rock.sand + (finer > 0 ? ground * rock.sand / finer : ground)) * grains;
    shore.pending[Coarse][i] += fallen * rock.coarse * blocks + domain.ground.sediment[Coarse][i];
    shore.pending[Sand][i] += sand + domain.ground.sediment[Sand][i];
    const float mud = fallen * (1.0f - rock.solute) - fallen * rock.coarse * blocks - sand + domain.ground.sediment[Fines][i];
    for (auto& layer : domain.ground.sediment) layer[i] = 0;
    domain.ground.rock[i] = bench;
    shore.top[i] = bench;
    shore.cut[i] += fallen;
    shore.notch[i] = 0;
    const double area = domain.cell * domain.cell;
    shore.ledger.cut += double(fallen) * area;
    shore.ledger.dissolved += double(fallen * rock.solute) * area;
    const int x = int(i % size_t(domain.size)), z = int(i / size_t(domain.size));
    if (domain.owned(x, z)) domain.ledger.dissolved += double(fallen * rock.solute) * area;
    // The water that reached the column now reaches what stood behind it.
    for (int k = 0; k < 8; ++k) {
        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
        if (nx < 0 || nz < 0 || nx >= domain.size || nz >= domain.size) continue;
        float& still = shore.still[domain.index(nx, nz)];
        if (still != NoWater) continue;
        still = shore.still[i];
        shore.minX = std::min(shore.minX, std::max(0, nx - 1)), shore.maxX = std::max(shore.maxX, std::min(domain.size - 1, nx + 1));
        shore.minZ = std::min(shore.minZ, std::max(0, nz - 1)), shore.maxZ = std::max(shore.maxZ, std::min(domain.size - 1, nz + 1));
    }
    return mud;
}

} // namespace

float withstandOf(const Domain& domain, size_t cell) {
    const float intact = definitionOf(domain.unit[cell].traits.dominant).compressiveStrength;
    const float jointed = intact * domain.sound[cell] * (1.0f - (1.0f - CrushedStrength) * domain.weakness[cell]) / std::max(0.05f, domain.quality[cell]);
    return std::max(LeastWithstand, Withstand * std::max(0.0f, jointed));
}

void cutCliffs(Domain& domain, Shore& shore, double years) {
    const float cell = float(domain.cell);
    const size_t cells = domain.cells();
    // Years each column still has to be cut in this step. A face that is cut through before the
    // step is over comes down, and the rest of its time goes to the ground it lays bare, which the
    // waves are raised again to reach: a weak rock goes back by as many cells as its rate allows.
    std::vector<float> spare(cells, float(years)), next(cells, 0.0f);
    std::vector<uint8_t> worked(cells, 0);
    std::vector<std::pair<size_t, float>> clouds;
    for (int slice = 0; slice < CutSlices; ++slice) {
        if (slice > 0) raiseWaves(domain, shore);
        bool fell = false;
        for (int z = shore.minZ; z <= shore.maxZ; ++z) {
            for (int x = shore.minX; x <= shore.maxX; ++x) {
                const size_t i = domain.index(x, z);
                const float water = shore.still[i], time = spare[i];
                if (time <= 0 || water == NoWater || domain.hold[i] != Free) continue;
                // A face is rock that stands out of the water at high tide with its foot in reach of it.
                if (shore.top[i] < water + swingOf(domain, water) || domain.ground.rock[i] <= benchOf(domain, water)) continue;
                const float rate = retreatOf(domain, shore, i, withstandOf(domain, i));
                worked[i] = 1;
                if (rate <= 0) continue;
                const float through = (cell - shore.notch[i]) / rate;
                if (through > time) {
                    shore.notch[i] += rate * time;
                    continue;
                }
                clouds.push_back({i, bringDown(domain, shore, i, benchOf(domain, water))});
                fell = true;
                for (int k = 0; k < 8; ++k) {
                    const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                    if (nx < 0 || nz < 0 || nx >= domain.size || nz >= domain.size) continue;
                    const size_t n = domain.index(nx, nz);
                    if (!worked[n]) next[n] = std::max(next[n], time - through);
                }
            }
        }
        if (!fell) break;
        spare.swap(next);
        std::fill(next.begin(), next.end(), 0.0f);
    }
    if (clouds.empty()) return;
    // Mud settles only where the waves of the shore as it now stands cannot keep it up.
    raiseWaves(domain, shore);
    for (const auto& [cell, mud] : clouds) suspend(domain, shore, cell, mud);
}

} // namespace derivean::sim

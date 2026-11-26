#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

constexpr float LeastWithstand = 0.03f;   // m; loose and rotten ground gives way to any wave
// km a block travels to and fro in the surf while the face behind it goes back by its own width:
// what the rock's abrasion leaves of it by then is the shingle of the shore.
constexpr float SurfTravel = 60.0f;
// km for its sand: grains of quartz outlast that, grains of chalk or mudstone do not, so only
// rock that wears slowly feeds a beach.
constexpr float SandTravel = 15.0f;
constexpr float MudLine = 2.5f;    // depth over wave height below which mud can settle
constexpr float MudSink = 0.12f;   // share of a mud cloud that sinks in each cell it crosses there

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
// wherever the bed lies below the reach of the waves, a share in every such cell and the rest
// where no neighbour lies lower.
void suspend(Domain& domain, Shore& shore, size_t from, float mud) {
    const int size = domain.size;
    const double area = domain.cell * domain.cell;
    size_t at = from;
    for (int step = 0; step < 4 * size && mud > 0; ++step) {
        const int x = int(at % size_t(size)), z = int(at / size_t(size));
        size_t lowest = at;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            if (shore.top[domain.index(nx, nz)] < shore.top[lowest]) lowest = domain.index(nx, nz);
        }
        const bool last = lowest == at;
        at = lowest;
        if (domain.hold[at] != Free) {
            shore.ledger.left += double(mud) * area;
            return;
        }
        float highest = 0;
        for (int k = 0; k < WaveDirections; ++k) highest = std::max(highest, shore.arrive[at * WaveDirections + size_t(k)]);
        const float depth = shore.still[at] == NoWater ? 0.0f : shore.still[at] - shore.top[at];
        if (!last && depth * depth < MudLine * MudLine * highest) continue;
        const float sunk = last ? mud : MudSink * mud;
        domain.ground.sediment[Fines][at] += sunk;
        shore.top[at] += sunk;
        mud -= sunk;
        if (last) return;
    }
    shore.ledger.left += double(mud) * area;
}

// The column comes down: its rock above the bench breaks into the sizes it weathers to. Blocks
// are ground down in the surf by the rock's own abrasion; sand and gravel wait in the surf of the
// cell for the waves to carry them off. What stood on the column as loose cover falls with it.
void bringDown(Domain& domain, Shore& shore, size_t i, float bench) {
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
    suspend(domain, shore, i, mud);
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
}

} // namespace

float withstandOf(const Domain& domain, size_t cell) {
    const float intact = definitionOf(domain.unit[cell].traits.dominant).compressiveStrength;
    const float jointed = intact * domain.sound[cell] * (1.0f - (1.0f - CrushedStrength) * domain.weakness[cell]) / std::max(0.05f, domain.quality[cell]);
    return std::max(LeastWithstand, Withstand * std::sqrt(std::max(0.0f, jointed)));
}

void cutCliffs(Domain& domain, Shore& shore, double years) {
    const float cell = float(domain.cell);
    for (int z = shore.minZ; z <= shore.maxZ; ++z) {
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            const float water = shore.still[i];
            if (water == NoWater || domain.hold[i] != Free) continue;
            // A face is rock that stands out of the water at high tide with its foot in reach of it.
            if (shore.top[i] < water + swingOf(domain, water) || domain.ground.rock[i] <= benchOf(domain, water)) continue;
            const float rate = retreatOf(domain, shore, i, withstandOf(domain, i));
            if (rate <= 0) continue;
            shore.notch[i] = std::min(cell, shore.notch[i] + rate * float(years));
            if (shore.notch[i] >= cell * 0.999f) bringDown(domain, shore, i, benchOf(domain, water));
        }
    }
}

} // namespace derivean::sim

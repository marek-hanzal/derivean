#include "coast/fn/Coast.hpp"

namespace derivean::sim {
namespace {

constexpr float LeastWithstand = 0.03f;   // m; loose and rotten ground gives way to any wave
// km a block travels to and fro in the surf while the face behind it goes back by its own width:
// what the rock's abrasion leaves of it by then is the shingle of the shore.
constexpr float SurfTravel = 60.0f;

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

// The column comes down: its rock above the bench breaks into the sizes it weathers to. Blocks
// are ground down in the surf by the rock's own abrasion; everything waits in the surf of the
// cell for the waves to carry it off. What stood on the column as loose cover falls with it.
void bringDown(Domain& domain, Shore& shore, size_t i, float bench) {
    const float fallen = domain.ground.rock[i] - bench;
    const RockTraits& rock = domain.unit[i].traits;
    const float survives = std::exp(-rock.abrasion * 1000.0f * SurfTravel), finer = rock.sand + rock.fines;
    const float ground = fallen * rock.coarse * (1.0f - survives);
    shore.pending[Coarse][i] += fallen * rock.coarse * survives + domain.ground.sediment[Coarse][i];
    shore.pending[Sand][i] += fallen * rock.sand + (finer > 0 ? ground * rock.sand / finer : ground) + domain.ground.sediment[Sand][i];
    shore.pending[Fines][i] += fallen * rock.fines + (finer > 0 ? ground * rock.fines / finer : 0.0f) + domain.ground.sediment[Fines][i];
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
}

} // namespace

float withstandOf(const Domain& domain, size_t cell) {
    const float intact = definitionOf(domain.unit[cell].traits.dominant).compressiveStrength;
    const float jointed = intact * domain.sound[cell] * (1.0f - (1.0f - CrushedStrength) * domain.weakness[cell]) / std::max(0.05f, domain.quality[cell]);
    return std::max(LeastWithstand, Withstand * std::sqrt(std::max(0.0f, jointed)));
}

double cutCliffs(Domain& domain, Shore& shore, double years) {
    if (years <= 0) return 0;
    const float cell = float(domain.cell);
    double soonest = years;
    bool any = false;
    for (int z = shore.minZ; z <= shore.maxZ; ++z) {
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            shore.rate[i] = 0;
            const float water = shore.still[i];
            if (water == NoWater || domain.hold[i] != Free) continue;
            // A face is rock that stands out of the water at high tide with its foot in reach of it.
            if (shore.top[i] < water + swingOf(domain, water) || domain.ground.rock[i] <= benchOf(domain, water)) continue;
            const float rate = retreatOf(domain, shore, i, withstandOf(domain, i));
            if (rate <= 0) continue;
            shore.rate[i] = rate;
            soonest = std::min(soonest, double(std::max(0.0f, cell - shore.notch[i]) / rate));
            any = true;
        }
    }
    if (!any) return years;
    for (int z = shore.minZ; z <= shore.maxZ; ++z) {
        for (int x = shore.minX; x <= shore.maxX; ++x) {
            const size_t i = domain.index(x, z);
            if (shore.rate[i] <= 0) continue;
            shore.notch[i] += shore.rate[i] * float(soonest);
            if (shore.notch[i] >= cell * 0.999f) bringDown(domain, shore, i, benchOf(domain, shore.still[i]));
        }
    }
    return soonest;
}

} // namespace derivean::sim

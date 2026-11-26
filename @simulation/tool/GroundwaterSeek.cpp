#include "Groundwater.hpp"

#include "groundwater/fn/Conductivity.hpp"
#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Heat.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/type/World.hpp"
#include "strata/type/Column.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr double Lattice = 16384.0;   // m between the places looked at
constexpr float Looked = 150.0f;      // m of rock under the surface that counts
constexpr float Platform = 2500.0f;   // m of the layering in which thick carbonate is first looked for
constexpr int Candidates = 24;        // places whose region is built to see how deep it has cut and whether it is dry
constexpr int Probes = 5;             // region cells a side read around a candidate, 2 km apart

// Metres of limestone or chalk of the dated cover between two depths below the datum: beds, not
// the marble of a basement. Nothing where the cover was folded: what is sought lies flat.
float carbonateIn(const Planet& planet, Vec2 point, float from, float to) {
    const StrataColumn column = strataAt(planet, point);
    if (column.foldSpan > QuietFoldSpan) return 0;
    float soluble = 0, at = from;
    while (at < to) {
        const StrataUnit unit = unitAt(column, at, 0);
        const Bed bed = bedAt(planet.seed, column, unit, at, column.material);
        const float bottom = std::min(std::max(bed.bottom, at + 0.5f), to);
        if (unit.slot >= 0 && fissuredBySolution(definitionOf(bed.rock))) soluble += bottom - at;
        at = bottom + 0.01f;
    }
    return soluble;
}

struct Candidate {
    Vec2 at;
    double score{}, rain{}, above{}, warmth{};
};

} // namespace

Vec2 seekWaterPlace(World& world, const std::string& kind) {
    const Planet& planet = world.planet;
    const int side = int(PlanetSpan / Lattice);
    const bool hot = kind == "hot";
    Vec2 best{};
    double most = -1;
    std::string why;
    std::vector<Candidate> candidates;
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) {
            const Vec2 at{(x + 0.5) * Lattice, (z + 0.5) * Lattice}, point = planetPoint(planet.seed, at);
            const double above = samplePlanet(planet.elevation, point, Kernel::Linear) - planet.seaLevel, rain = samplePlanet(planet.precipitation, point, Kernel::Linear);
            const double warmth = samplePlanet(planet.temperature, point, Kernel::Linear);
            if (above < 80 || rain < (hot ? 0.3 : 0.6) || warmth < 3) continue;
            if (hot) {
                const Geotherm heat = geothermAt(planet, point);
                if (heat.gradient <= most) continue;
                char reason[256];
                std::snprintf(reason, sizeof reason, "%.0f K/km from below, %.0f %% of it magmatic, melt %.2f Myr ago; rain %.2f m/yr, %.0f m above the sea", 1000.0 * heat.gradient, 100.0 * heat.magmatic,
                              heat.sinceMelt / 1e6, rain, above);
                most = heat.gradient, best = at, why = reason;
                continue;
            }
            // Thick carbonate in the layering: the region's rivers may have cut far into it, so how
            // deep the ground now lies is only known once the region is built.
            const float thick = carbonateIn(planet, point, 0, Platform);
            if (thick < 100) continue;
            candidates.push_back({at, thick * std::min(rain, 2.0) * std::clamp(above / 800.0, 0.2, 1.5), rain, above, warmth});
        }
    if (!hot) {
        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
        if (candidates.size() > size_t(Candidates)) candidates.resize(Candidates);
        const double cell = specOf(Level::Region).cell;
        for (const Candidate& candidate : candidates) world.atlas.request(tileAt(Level::Region, candidate.at), 0);
        // What the landscape reads: the layering below the depth its region has cut to.
        for (const Candidate& candidate : candidates) {
            double share = 0, cut = 0, dry = 0, above = 0;
            for (int pz = 0; pz < Probes; ++pz)
                for (int px = 0; px < Probes; ++px) {
                    const Vec2 at{candidate.at.x + (px - Probes / 2) * 2 * cell, candidate.at.z + (pz - Probes / 2) * 2 * cell};
                    const std::shared_ptr<const Tile> region = world.atlas.require(tileAt(Level::Region, at));
                    const int64_t gx = int64_t(std::floor(at.x / cell)) - int64_t(region->key.x) * TileCells, gz = int64_t(std::floor(at.z / cell)) - int64_t(region->key.z) * TileCells;
                    const size_t from = size_t(gz) * TileCells + size_t(gx);
                    const float depth = std::max(0.0f, region->now.datum[from] - region->now.rock[from]);
                    const float ground = region->now.rock[from] + region->now.sediment[Coarse][from] + region->now.sediment[Sand][from] + region->now.sediment[Fines][from];
                    // Soluble rock under a lake or the sea drains by no conduits.
                    if (region->water[from] > ground + 1e-3f) continue;
                    share += carbonateIn(planet, planetPoint(planet.seed, at), depth, depth + Looked) / Looked;
                    cut += depth, above += ground - region->seaLevel, ++dry;
                }
            const double probes = Probes * Probes;
            share /= probes, cut /= std::max(1.0, dry), above /= std::max(1.0, dry);
            const double score = share * std::min(candidate.rain, 2.0) * std::clamp(above / 800.0, 0.2, 1.5);
            std::printf("  candidate X %.0f Z %.0f: %.0f %% dry, %.0f m above the sea; region cut %.0f m into the layering, %.0f %% of the %.0f m below that is limestone or chalk\n", candidate.at.x, candidate.at.z,
                        100 * dry / probes, above, cut, 100 * share, double(Looked));
            if (score <= most) continue;
            char reason[256];
            std::snprintf(reason, sizeof reason, "%.0f %% of the top %.0f m under the region's dry ground is flat-lying limestone or chalk (cut %.0f m); rain %.2f m/yr, %.0f m above the sea, %.0f C", 100 * share,
                          double(Looked), cut, candidate.rain, above, candidate.warmth);
            most = score, best = candidate.at, why = reason;
        }
    }
    std::printf("sought %s over the planet on a lattice of %.0f km: X %.0f Z %.0f | %s\n", hot ? "the hottest ground with rain" : "flat-lying limestone standing in the rain", Lattice / 1000, best.x, best.z,
                most <= 0 ? "none found" : why.c_str());
    return best;
}

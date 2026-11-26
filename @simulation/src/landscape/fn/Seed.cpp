#include "landscape/fn/Build.hpp"

#include "landscape/fn/Forcing.hpp"
#include "planet/fn/Sample.hpp"

namespace derivean::sim {
namespace {

// Height of the seeded roughness a level starts with and the wavelength it sits at. It only breaks
// the symmetry of an interpolated surface; relief comes from what the replay does with it.
constexpr float RoughHeight[LevelCount] = {0, 14.0f, 2.2f, 0.22f, 0.02f};
constexpr double RoughLength[LevelCount] = {0, 9000.0, 620.0, 42.0, 2.6};
constexpr float LeastLake = 1.0f;   // m of water a parent cell holds for a finer level to keep its basin
constexpr float GroundMantle = 0.3f;   // m of loose cover above which a ground tile starts rough in its mantle, not in its rock

// The parent tiles under a domain, addressed by the parent's global cell index.
struct ParentGrid {
    int baseX{}, baseZ{}, countX{}, countZ{};
    std::vector<const Tile*> tiles;

    ParentGrid(TileKey child, const TileLookup& lookup) {
        const int margin = parentMargin(child.level), span = TileCells / Refinement;
        baseX = int(std::floor((child.x * span - margin) / double(TileCells)));
        baseZ = int(std::floor((child.z * span - margin) / double(TileCells)));
        countX = int(std::floor(((child.x + 1) * span + margin - 1) / double(TileCells))) - baseX + 1;
        countZ = int(std::floor(((child.z + 1) * span + margin - 1) / double(TileCells))) - baseZ + 1;
        const Level level = Level(int(child.level) - 1);
        for (int z = 0; z < countZ; ++z)
            for (int x = 0; x < countX; ++x) tiles.push_back(lookup({level, baseX + x, baseZ + z}));
    }

    template<typename Pick>
    float at(int64_t gx, int64_t gz, Pick&& pick) const {
        const int tx = std::clamp(int(std::floor(gx / double(TileCells))) - baseX, 0, countX - 1);
        const int tz = std::clamp(int(std::floor(gz / double(TileCells))) - baseZ, 0, countZ - 1);
        const int lx = std::clamp(int(gx - int64_t(tx + baseX) * TileCells), 0, TileCells - 1);
        const int lz = std::clamp(int(gz - int64_t(tz + baseZ) * TileCells), 0, TileCells - 1);
        return pick(*tiles[size_t(tz) * size_t(countX) + size_t(tx)], size_t(lz) * TileCells + size_t(lx));
    }

    // Smoothing spline over the parent's cell centres. A parent cell is the mean of everything
    // inside it, and its single-cell ridges and pits are artefacts of its grid, not landforms; the
    // child starts from the smooth trend and makes relief of its own size by replaying its window.
    template<typename Pick>
    float smooth(double fx, double fz, Pick&& pick) const {
        const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
        double wx[4], wz[4];
        splineWeights(fx - double(ix), wx);
        splineWeights(fz - double(iz), wz);
        double sum = 0;
        for (int j = 0; j < 4; ++j)
            for (int i = 0; i < 4; ++i) sum += wx[i] * wz[j] * at(ix - 1 + i, iz - 1 + j, pick);
        return float(sum);
    }

    template<typename Pick>
    float linear(double fx, double fz, Pick&& pick) const {
        const int64_t ix = int64_t(std::floor(fx)), iz = int64_t(std::floor(fz));
        const double tx = fx - double(ix), tz = fz - double(iz);
        return float(mix(mix(at(ix, iz, pick), at(ix + 1, iz, pick), tx), mix(at(ix, iz + 1, pick), at(ix + 1, iz + 1, pick), tx), tz));
    }
};

Vec2 materialOf(const Domain& domain, int x, int z) {
    const int stride = domain.stride;
    const Vec2 node{domain.origin.x + ((x / stride) * stride + 0.5) * domain.cell, domain.origin.z + ((z / stride) * stride + 0.5) * domain.cell};
    return domain.strataOf(domain.index(x, z)).material + (domain.center(x, z) - node);
}

// The rock the ice of the last glacial maximum crossed before it reached each parent cell under the
// tile: up the ice surface from there to the first rock of another kind, or to where the ice began.
void traceIce(Domain& domain, const ParentGrid& parent, double parentCell) {
    constexpr int Farthest = 96;   // parent cells
    const int span = TileCells / Refinement;
    domain.upIce.assign(size_t(span) * size_t(span), IceSource{});
    const auto iceAt = [&](int64_t x, int64_t z) { return parent.at(x, z, [](const Tile& t, size_t c) { return t.iceAge[c]; }); };
    const auto topAt = [&](int64_t x, int64_t z) {
        return parent.at(x, z, [](const Tile& t, size_t c) { return t.now.rock[c] + t.now.sediment[0][c] + t.now.sediment[1][c] + t.now.sediment[2][c] + t.iceAge[c]; });
    };
    const auto rockAt = [&](int64_t x, int64_t z) { return Material(uint8_t(parent.at(x, z, [](const Tile& t, size_t c) { return float(t.rock[c]); }))); };
    for (int pz = 0; pz < span; ++pz) {
        for (int px = 0; px < span; ++px) {
            int64_t x = int64_t(domain.key.x) * span + px, z = int64_t(domain.key.z) * span + pz;
            if (iceAt(x, z) <= 0) continue;
            const Material own = rockAt(x, z);
            Material found = own;
            for (int step = 0; step < Farthest && found == own; ++step) {
                int best = -1;
                float highest = topAt(x, z);
                for (int k = 0; k < 8; ++k) {
                    const float top = topAt(x + NeighborX[k], z + NeighborZ[k]);
                    if (top > highest && iceAt(x + NeighborX[k], z + NeighborZ[k]) > 0) highest = top, best = k;
                }
                if (best < 0) break;
                x += NeighborX[best], z += NeighborZ[best];
                found = rockAt(x, z);
            }
            domain.upIce[size_t(pz) * size_t(span) + size_t(px)] = {{(double(x) + 0.5) * parentCell, (double(z) + 0.5) * parentCell}, found};
        }
    }
}

} // namespace

void seedFromPlanet(Domain& domain, const Setting& setting) {
    const Backdrop& backdrop = *setting.backdrop;
    const bool wet = setting.planet->surfaceWater > 0;
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const Vec2 point = planetPoint(setting.seed, domain.center(x, z));
            const float rough = RoughHeight[1] * roughnessAt(setting.seed, materialOf(domain, x, z), RoughLength[1]);
            domain.ground.rock[i] = float(samplePlanet(backdrop.pastElevation, point)) + rough;
            domain.ground.datum[i] = domain.ground.rock[i];
            if (wet && domain.ground.rock[i] > domain.seaLevel) {
                domain.ground.sediment[Sand][i] = 0.5f;
                domain.ground.sediment[Fines][i] = 0.4f;
            }
            domain.rimFloor[i] = float(samplePlanet(backdrop.graded, point, Kernel::Linear));
        }
    }
    // The planet's rivers enter where its drainage, routed finer than its own cells, crosses the
    // domain's edge: near where the region's own valleys will take them, and at the same place for
    // the two regions on either side of an edge.
    const Drainage& drainage = backdrop.drainage;
    injectInflow(domain, {Drainage::Cell, [&](int64_t gx, int64_t gz) { return drainage.discharge[drainage.index(gx, gz)]; },
                          [&](int64_t gx, int64_t gz) { return drainage.load[drainage.index(gx, gz)]; },
                          [&](int64_t gx, int64_t gz) { return drainage.flowTo[drainage.index(gx, gz)]; }});
}

void seedFromParent(Domain& domain, const Setting& setting, const TileLookup& lookup) {
    const ParentGrid parent(domain.key, lookup);
    const size_t level = size_t(domain.key.level);
    const double parentCell = domain.cell * Refinement;
    const bool ground = domain.key.level == Level::Ground;
    domain.standing.assign(domain.cells(), -1e9f);
    domain.lakeFloor.assign(domain.cells(), -1e9f);
    if (domain.key.level == Level::Site) traceIce(domain, parent, parentCell);
    for (int z = 0; z < domain.size; ++z) {
        for (int x = 0; x < domain.size; ++x) {
            const size_t i = domain.index(x, z);
            const Vec2 here = domain.center(x, z);
            const double fx = here.x / parentCell - 0.5, fz = here.z / parentCell - 0.5;
            const float rock = parent.smooth(fx, fz, [](const Tile& t, size_t c) { return t.now.rock[c]; });
            // The layering is a surface of its own: it is carried down with the same smoothing as the
            // rock, not as a depth under it, or every valley would bend the beds towards itself.
            const float datum = parent.smooth(fx, fz, [](const Tile& t, size_t c) { return t.now.datum[c]; });
            float loose = 0;
            for (int c = 0; c < SedimentClasses; ++c) {
                domain.ground.sediment[c][i] = std::max(0.0f, parent.linear(fx, fz, [c](const Tile& t, size_t k) { return t.now.sediment[c][k]; }));
                loose += domain.ground.sediment[c][i];
            }
            // Roughness goes into whatever forms the surface, so it never invents rock under a plain.
            const float rough = RoughHeight[level] * roughnessAt(setting.seed, materialOf(domain, x, z), RoughLength[level]);
            const bool mantled = ground && loose > GroundMantle;
            domain.ground.rock[i] = rock + (mantled || loose > 1.0f ? 0.0f : rough);
            if (mantled) {
                // A ground cell tells a mantle a few decimetres thick from the rock under it.
                const int top = domain.ground.sediment[Fines][i] > 0.02f ? Fines : domain.ground.sediment[Sand][i] > 0.02f ? Sand : Coarse;
                domain.ground.sediment[top][i] = std::max(0.0f, domain.ground.sediment[top][i] + rough);
            }
            domain.ground.datum[i] = std::max(domain.ground.rock[i], datum);
            {
                // The parent's standing water: a ground tile holds it as its own, and a finer level
                // keeps the basin under it (levelHollows). Above the ground level a pit the parent's
                // flood has barely filled is a hollow of its surface, not a lake.
                const float least = ground ? 1e-3f : LeastLake;
                const int64_t px = int64_t(std::floor(here.x / parentCell)), pz = int64_t(std::floor(here.z / parentCell));
                const auto lake = [least](const Tile& t, size_t c) {
                    const float top = t.now.rock[c] + t.now.sediment[0][c] + t.now.sediment[1][c] + t.now.sediment[2][c];
                    return t.water[c] > top + least ? t.water[c] : -1e9f;
                };
                domain.standing[i] = parent.at(px, pz, lake);
                // A lake the parent holds is held at the parent's level: a parent cell beside it whose
                // ground stands at that water or above is its rim, one of them its outlet, and none is
                // cut below the water. A cell lower than a lake beside it (the channel below an outlet,
                // or a lower lake's shore) does not hold that lake. What fills the lake is this level's
                // own water, so it may stand lower, never spill lower.
                const float top = parent.at(px, pz, [](const Tile& t, size_t c) { return t.now.rock[c] + t.now.sediment[0][c] + t.now.sediment[1][c] + t.now.sediment[2][c]; });
                domain.lakeFloor[i] = domain.standing[i];
                for (int k = 0; k < 9; ++k) {
                    const float beside = parent.at(px + k % 3 - 1, pz + k / 3 - 1, lake);
                    if (beside <= top + least) domain.lakeFloor[i] = std::max(domain.lakeFloor[i], beside);
                }
            }
            if (ground) {
                // Ice is the parent's: a ground tile is too small to hold it by itself.
                domain.ice[i] = std::max(0.0f, parent.linear(fx, fz, [](const Tile& t, size_t c) { return t.ice[c]; }));
                domain.iceAge[i] = std::max(0.0f, parent.linear(fx, fz, [](const Tile& t, size_t c) { return t.iceAge[c]; }));
            }
            // The parent's rock uplift already holds its tectonics and the plate's response to erosion.
            domain.tectonic[i] = parent.linear(fx, fz, [](const Tile& t, size_t c) { return t.uplift[c]; });
            domain.residual[i] = domain.rebound[i] = 0;
            domain.rain[i] = parent.linear(fx, fz, [](const Tile& t, size_t c) { return t.rain[c]; });
            domain.trend[i] = rock + loose;
        }
    }
    injectInflow(domain, {parentCell, [&](int64_t gx, int64_t gz) { return parent.at(gx, gz, [](const Tile& t, size_t c) { return t.discharge[c]; }); },
                          [&](int64_t gx, int64_t gz) { return parent.at(gx, gz, [](const Tile& t, size_t c) { return t.load[c]; }); },
                          [&](int64_t gx, int64_t gz) { return uint8_t(parent.at(gx, gz, [](const Tile& t, size_t c) { return float(t.flowTo[c]); })); }});
}

void injectInflow(Domain& domain, const CoarseFlow& coarse) {
    const double extent = domain.size * domain.cell;
    const int last = domain.size - 1;
    for (int z = 0; z <= last; ++z) {
        for (int x = 0; x <= last; ++x) {
            if (x > 0 && z > 0 && x < last && z < last) continue;
            const Vec2 here = domain.center(x, z);
            const uint8_t to = coarse.flowTo(int64_t(std::floor(here.x / coarse.cell)), int64_t(std::floor(here.z / coarse.cell)));
            // Cells the coarse level does not route (its sea and its own edge) stay open.
            const int outX = x == 0 ? -1 : x == last ? 1 : 0, outZ = z == 0 ? -1 : z == last ? 1 : 0;
            domain.outlet[domain.index(x, z)] = to > 7 || NeighborX[to] * outX + NeighborZ[to] * outZ > 0;
        }
    }
    const auto inside = [&](Vec2 p) {
        return p.x > domain.origin.x + domain.cell && p.z > domain.origin.z + domain.cell &&
               p.x < domain.origin.x + extent - domain.cell && p.z < domain.origin.z + extent - domain.cell;
    };
    const int64_t lowX = int64_t(std::floor(domain.origin.x / coarse.cell)) - 1, lowZ = int64_t(std::floor(domain.origin.z / coarse.cell)) - 1;
    const int64_t highX = int64_t(std::floor((domain.origin.x + extent) / coarse.cell)) + 1, highZ = int64_t(std::floor((domain.origin.z + extent) / coarse.cell)) + 1;
    const int reach = std::max(1, int(coarse.cell / domain.cell / 2));
    for (int64_t gz = lowZ; gz <= highZ; ++gz) {
        for (int64_t gx = lowX; gx <= highX; ++gx) {
            const Vec2 from{(gx + 0.5) * coarse.cell, (gz + 0.5) * coarse.cell};
            const uint8_t to = coarse.flowTo(gx, gz);
            if (to > 7 || inside(from)) continue;
            const Vec2 target{from.x + NeighborX[to] * coarse.cell, from.z + NeighborZ[to] * coarse.cell};
            if (!inside(target)) continue;
            // The river crosses near the midpoint of the two centres; it enters at the lowest cell there.
            const Vec2 crossing = (from + target) * 0.5;
            const int cx = std::clamp(int((crossing.x - domain.origin.x) / domain.cell), 1, domain.size - 2);
            const int cz = std::clamp(int((crossing.z - domain.origin.z) / domain.cell), 1, domain.size - 2);
            size_t best = domain.index(cx, cz);
            for (int offset = -reach; offset <= reach; ++offset) {
                const bool alongX = NeighborX[to] == 0 || (NeighborZ[to] != 0 && (cz == 1 || cz == domain.size - 2));
                const size_t candidate = domain.index(std::clamp(cx + (alongX ? offset : 0), 1, domain.size - 2),
                                                      std::clamp(cz + (alongX ? 0 : offset), 1, domain.size - 2));
                if (domain.ground.rock[candidate] + domain.sedimentAt(candidate) < domain.ground.rock[best] + domain.sedimentAt(best)) best = candidate;
            }
            domain.inflow[best] += coarse.discharge(gx, gz);
            domain.inflowLoad[best] += coarse.load(gx, gz);
        }
    }
}

} // namespace derivean::sim

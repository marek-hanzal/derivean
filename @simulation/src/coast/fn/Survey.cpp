#include "coast/fn/Survey.hpp"

#include "measure/fn/Face.hpp"

namespace derivean::sim {
namespace {

constexpr int Side[4] = {0, 2, 4, 6};     // the four neighbours across a cell edge
constexpr float BeachTop = 6.0f;          // m above the water up to which loose ground behind the waterline is beach
constexpr float StripTop = 12.0f;         // m above the water up to which a strip between two waters may stand
constexpr float StackRise = 3.0f;         // m a stack stands above the water
constexpr int StackCells = 16;            // cells a stack may cover

struct Reader {
    const ShoreGrid& grid;
    ShoreSurvey& survey;
    std::vector<float> cliffs, platforms, beaches[2], slopes[SedimentClasses], struck;
    std::vector<int32_t> behind;   // wet cells found behind a strip

    bool inside(int x, int z) const { return x >= 0 && z >= 0 && x < grid.side && z < grid.side; }
    bool platform(size_t i) const { return grid.wet(i) && grid.cut[i] > 0 && grid.loose(i) < BareCover && grid.water[i] - grid.top(i) <= BenchWater; }

    ShoreKind kindOf(int x, int z, float water, float& height) const {
        const size_t i = grid.index(x, z);
        const float loose = grid.loose(i);
        if (loose >= BareCover) return grid.cover[Coarse][i] >= CoarseShare * loose ? ShoreKind::Gravel : grid.cover[Sand][i] >= grid.cover[Fines][i] ? ShoreKind::Sand : ShoreKind::Mud;
        const int reach = std::max(2, int(std::lround(CliffReach / grid.cell)));
        height = 0;
        for (int dz = -reach; dz <= reach; ++dz)
            for (int dx = -reach; dx <= reach; ++dx) {
                if (!inside(x + dx, z + dz)) continue;
                const float rise = grid.top(grid.index(x + dx, z + dz)) - water, run = (std::sqrt(float(dx * dx + dz * dz)) + 0.5f) * float(grid.cell);
                if (rise >= CliffRise && rise >= CliffSteep * run) height = std::max(height, rise);
            }
        return height > 0 ? ShoreKind::Cliff : ShoreKind::Rock;
    }

    // Cells of platform before a shore cell, across the shore: the shortest run over its wet sides.
    float benchBefore(int x, int z) const {
        int shortest = 0;
        for (const int k : Side) {
            int run = 0;
            for (int nx = x + NeighborX[k], nz = z + NeighborZ[k]; inside(nx, nz) && platform(grid.index(nx, nz)) && run < 400; nx += NeighborX[k], nz += NeighborZ[k]) ++run;
            if (run > 0 && (shortest == 0 || run < shortest)) shortest = run;
        }
        const float within = grid.notch[grid.index(x, z)];
        return shortest > 0 ? float(shortest * grid.cell) + within : within >= NotchBench ? within : 0.0f;
    }

    // Walks inland from a beach cell, away from its water: the dry beach, and whether other water lies behind it.
    void walkInland(int x, int z, int k, float water, bool sea) {
        const size_t i = grid.index(x, z), front = grid.index(x + NeighborX[k], z + NeighborZ[k]);
        const int back = (k + 4) & 7, far = int(StripWidth / grid.cell);
        int beach = 0;
        bool onBeach = true;
        for (int step = 0, nx = x, nz = z; step <= far && inside(nx, nz); ++step, nx += NeighborX[back], nz += NeighborZ[back]) {
            const size_t n = grid.index(nx, nz);
            if (grid.wet(n)) {
                if (!(survey.flag[i] & 2)) survey.strip += grid.cell;
                survey.flag[i] |= 2;
                behind.push_back(int32_t(n));
                break;
            }
            if (grid.top(n) > water + StripTop) break;
            onBeach = onBeach && grid.loose(n) >= BareCover && grid.top(n) <= water + BeachTop;
            if (onBeach) ++beach;
        }
        beaches[sea ? 0 : 1].push_back(float(beach * grid.cell));
        const ShoreKind kind = ShoreKind(survey.kind[i]);
        slopes[kind == ShoreKind::Gravel ? Coarse : kind == ShoreKind::Sand ? Sand : Fines].push_back((grid.top(i) - grid.top(front)) / float(grid.cell));
    }

    // Rock that stands alone in the water.
    void countStacks() {
        std::vector<uint8_t> seen(grid.rock.size(), 0);
        std::vector<int32_t> body;
        for (int z = 0; z < grid.side; ++z)
            for (int x = 0; x < grid.side; ++x) {
                const size_t start = grid.index(x, z);
                if (seen[start] || grid.wet(start)) continue;
                body.assign(1, int32_t(start));
                seen[start] = 1;
                bool edge = false, bare = true;
                float rise = 0;
                for (size_t next = 0; next < body.size(); ++next) {
                    const int cx = body[next] % grid.side, cz = body[next] / grid.side;
                    bare = bare && grid.loose(size_t(body[next])) < BareCover;
                    for (int k = 0; k < 8; ++k) {
                        const int nx = cx + NeighborX[k], nz = cz + NeighborZ[k];
                        if (!inside(nx, nz)) { edge = true; continue; }
                        const size_t n = grid.index(nx, nz);
                        if (grid.wet(n)) rise = std::max(rise, grid.top(size_t(body[next])) - grid.water[n]);
                        else if (!seen[n]) seen[n] = 1, body.push_back(int32_t(n));
                    }
                }
                if (edge || !bare || int(body.size()) > StackCells || rise < StackRise) continue;
                ++survey.stacks;
                for (const int32_t cell : body) survey.flag[size_t(cell)] |= 8;
            }
    }

    // Water behind a strip is a lagoon as far as it lies sheltered from the open water's waves.
    void fillLagoons() {
        float open = 0;
        for (size_t i = 0; i < grid.wave.size(); ++i)
            if (grid.wet(i)) open = std::max(open, grid.wave[i]);
        for (size_t next = 0; next < behind.size(); ++next) {
            const size_t i = size_t(behind[next]);
            if ((survey.flag[i] & 4) || grid.wave[i] >= Sheltered * open) continue;
            survey.flag[i] |= 4;
            survey.lagoon += grid.cell * grid.cell;
            for (const int k : Side) {
                const int nx = int(i % size_t(grid.side)) + NeighborX[k], nz = int(i / size_t(grid.side)) + NeighborZ[k];
                if (!inside(nx, nz)) continue;
                if (grid.wet(grid.index(nx, nz))) behind.push_back(int32_t(grid.index(nx, nz)));
                else survey.lagoonShore += grid.cell;
            }
        }
    }
};

} // namespace

ShoreSurvey surveyShore(const ShoreGrid& grid) {
    ShoreSurvey survey;
    survey.kind.assign(grid.rock.size(), uint8_t(ShoreKind::None));
    survey.flag.assign(grid.rock.size(), 0);
    Reader reader{grid, survey, {}, {}, {}, {}, {}, {}};
    for (size_t i = 0; i < grid.rock.size(); ++i)
        if (reader.platform(i)) survey.flag[i] |= 1;
    for (int z = 0; z < grid.side; ++z) {
        for (int x = 0; x < grid.side; ++x) {
            const size_t i = grid.index(x, z);
            if (grid.wet(i)) continue;
            int edges = 0;
            float water = NoWater;
            for (const int k : Side)
                if (reader.inside(x + NeighborX[k], z + NeighborZ[k]) && grid.wet(grid.index(x + NeighborX[k], z + NeighborZ[k])))
                    ++edges, water = std::max(water, grid.water[grid.index(x + NeighborX[k], z + NeighborZ[k])]);
            if (edges == 0) continue;
            const bool sea = water <= float(grid.seaLevel) + SeaBand;
            float height = 0;
            const ShoreKind kind = reader.kindOf(x, z, water, height);
            survey.kind[i] = uint8_t(kind);
            (sea ? survey.sea : survey.lake)[size_t(kind)] += edges * grid.cell;
            reader.struck.push_back(grid.wave[i]);
            if (kind == ShoreKind::Cliff || kind == ShoreKind::Rock) {
                if (kind == ShoreKind::Cliff) reader.cliffs.push_back(height);
                const float bench = reader.benchBefore(x, z);
                if (bench > 0) survey.benched += edges * grid.cell, reader.platforms.push_back(bench);
                continue;
            }
            for (const int k : Side)
                if (reader.inside(x + NeighborX[k], z + NeighborZ[k]) && grid.wet(grid.index(x + NeighborX[k], z + NeighborZ[k]))) reader.walkInland(x, z, k, water, sea);
        }
    }
    reader.countStacks();
    reader.fillLagoons();
    survey.cliffHeight = distributionOf(reader.cliffs);
    survey.platformWidth = distributionOf(reader.platforms);
    survey.struck = distributionOf(reader.struck);
    for (int k = 0; k < 2; ++k) survey.beachWidth[k] = distributionOf(reader.beaches[k]);
    for (int c = 0; c < SedimentClasses; ++c) survey.beachSlope[c] = distributionOf(reader.slopes[c]);
    return survey;
}

} // namespace derivean::sim

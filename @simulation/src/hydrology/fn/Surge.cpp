#include "hydrology/fn/Water.hpp"

#include "chronology/type/Calendar.hpp"

namespace derivean::sim {
namespace {

constexpr double Courant = 0.5;         // share of the gravity-wave limit one step may take
constexpr double LongestStep = 2.0;     // s
constexpr double ThinnestFlow = 1e-4;   // m of water over a face below which nothing moves through it
constexpr double StandingWater = 0.05;  // m of depth from which a cell's rise and fall says whether an event is still going on
constexpr double OwnWeight = 0.8;       // share of a face's own discharge in the smoothed value it starts a step from
// Width and length of the four faces as shares of the cell. With these weights the eight
// neighbours together carry a plane wave at the same speed in every direction.
constexpr double FaceWidth[4] = {0.5, 0.35355339, 0.5, 0.35355339};
constexpr double FaceReach[4] = {1.0, 1.41421356, 1.0, 1.41421356};

} // namespace

// Local-inertia shallow water: each face keeps a discharge that gravity accelerates down the
// surface slope and Manning friction, taken implicitly, slows. A face starts each step from its
// own discharge blended with the two faces in line with it; without that the scheme leaves waves
// of two cells' length undamped in deep water. What a cell sends out in a step is scaled back to
// what it holds, so depth never goes negative and every cubic metre that leaves one cell arrives
// in another. Rain, runoff and evaporation are too slow to matter to an event; the caller leaves
// them to the quiet solver.
double surge(WaterGrid& grid, double seconds, WaterFlows& total, SurgeScratch& scratch) {
    const size_t cells = grid.cells();
    WaterFlows flows;
    const double area = grid.cell * grid.cell;
    const int32_t offset[4] = {1, grid.width + 1, grid.width, grid.width - 1};
    std::vector<double>&surface = scratch.surface, &sent = scratch.sent, &gain = scratch.gain, &saltGain = scratch.saltGain;
    std::vector<uint8_t>& moves = scratch.moves;
    for (auto* work : {&surface, &sent, &gain, &saltGain}) work->resize(cells);
    for (auto& face : scratch.fresh) face.resize(cells);
    moves.resize(cells);
    scratch.before = grid.depth;
    for (double left = seconds; left > 0;) {
        // Only the rows and columns that hold water, and one cell around them, can move any.
        double deepest = 0.01;
        int lowX = grid.width, highX = -1, lowZ = grid.height, highZ = -1;
        for (int z = 0; z < grid.height; ++z) {
            for (int x = 0; x < grid.width; ++x) {
                const size_t cell = size_t(z) * size_t(grid.width) + size_t(x);
                const CellKind kind = grid.kindOf(int32_t(cell));
                surface[cell] = kind == CellKind::Rim ? double(grid.held[cell]) : double(grid.ground[cell]) + grid.depth[cell];
                moves[cell] = kind == CellKind::Open ? grid.depth[cell] > 0 : kind == CellKind::Rim && grid.held[cell] > grid.ground[cell];
                if (!moves[cell]) continue;
                deepest = std::max(deepest, surface[cell] - double(grid.ground[cell]));
                lowX = std::min(lowX, x), highX = std::max(highX, x), lowZ = std::min(lowZ, z), highZ = std::max(highZ, z);
            }
        }
        std::fill(sent.begin(), sent.end(), 0.0);
        std::fill(gain.begin(), gain.end(), 0.0);
        std::fill(saltGain.begin(), saltGain.end(), 0.0);
        for (auto& face : scratch.fresh) std::fill(face.begin(), face.end(), 0.0f);
        if (grid.periodic) lowX = lowZ = 0, highX = grid.width - 1, highZ = grid.height - 1;
        lowX = std::max(lowX - 1, 0), lowZ = std::max(lowZ - 1, 0), highX = std::min(highX + 1, grid.width - 1);
        // What is left of the call goes in equal steps: a short last step would kick the faces out
        // of a steady flow every time.
        const double dt = left / std::ceil(left / std::min(LongestStep, Courant * grid.cell / std::sqrt(Gravity * deepest)));
        left = left - dt < 1e-9 * seconds ? 0.0 : left - dt;

        for (int z = lowZ; z <= highZ; ++z) {
            for (int x = lowX; x <= highX; ++x) {
                const size_t cell = size_t(z) * size_t(grid.width) + size_t(x);
                const CellKind kind = grid.kindOf(int32_t(cell));
                if (kind == CellKind::Outside) continue;
                const bool inner = !grid.periodic && x > 0 && x + 1 < grid.width && z > 0 && z + 1 < grid.height;
                for (int k = 0; k < 4; ++k) {
                    const int32_t other = inner ? int32_t(cell) + offset[k] : grid.neighbour(int32_t(cell), k);
                    if (other < 0 || (!moves[cell] && !moves[size_t(other)]) || grid.kindOf(other) == CellKind::Outside ||
                        (kind == CellKind::Rim && grid.kindOf(other) == CellKind::Rim))
                        continue;
                    const double depth = std::max(surface[cell], surface[size_t(other)]) - std::max(grid.ground[cell], grid.ground[size_t(other)]);
                    if (depth <= ThinnestFlow) continue;
                    const double own = grid.flux[k][cell];
                    const double start = inner ? OwnWeight * own + 0.5 * (1.0 - OwnWeight) * (double(grid.flux[k][cell - size_t(offset[k])]) + double(grid.flux[k][size_t(other)])) : own;
                    const double slope = (surface[size_t(other)] - surface[cell]) / (grid.cell * FaceReach[k]);
                    const double fastest = depth * std::sqrt(Gravity * depth);
                    const double next = std::clamp((start - Gravity * depth * dt * slope) /
                                                       (1.0 + Gravity * dt * BedRoughness * BedRoughness * std::abs(own) / (depth * depth * std::cbrt(depth))),
                                                   -fastest, fastest);
                    scratch.fresh[k][cell] = float(next);
                    sent[next > 0 ? cell : size_t(other)] += std::abs(double(scratch.fresh[k][cell])) * FaceWidth[k] * grid.cell * dt;
                }
            }
        }
        for (int k = 0; k < 4; ++k) grid.flux[k].swap(scratch.fresh[k]);
        // A cell cannot send more than it holds; a dry rim cell has nothing to send. A face reaches
        // one row and one column beyond the cells that own it.
        const size_t first = size_t(lowZ) * size_t(grid.width), last = std::min(cells, size_t(std::min(highZ + 2, grid.height)) * size_t(grid.width));
        for (size_t cell = grid.periodic ? 0 : first; cell < (grid.periodic ? cells : last); ++cell) {
            const CellKind kind = grid.kindOf(int32_t(cell));
            const double holds = kind == CellKind::Open ? grid.depth[cell] * area : grid.held[cell] > grid.ground[cell] ? sent[cell] : 0.0;
            sent[cell] = sent[cell] > holds ? holds / sent[cell] : 1.0;
        }
        for (size_t cell = grid.periodic ? 0 : first; cell < (grid.periodic ? cells : last); ++cell) {
            for (int k = 0; k < 4; ++k) {
                float& flux = grid.flux[k][cell];
                if (flux == 0) continue;
                const size_t other = size_t(grid.neighbour(int32_t(cell), k));
                const size_t from = flux > 0 ? cell : other, to = flux > 0 ? other : cell;
                // The volume is taken before the scaled discharge is stored as a float: rounded up, it
                // would let a cell send a hair more than it holds.
                const double scaled = double(flux) * sent[from], volume = std::abs(scaled) * FaceWidth[k] * grid.cell * dt;
                flux = float(scaled);
                const bool fromRim = grid.kindOf(int32_t(from)) == CellKind::Rim, toRim = grid.kindOf(int32_t(to)) == CellKind::Rim;
                const double salt = fromRim ? volume * grid.salinity[from] : grid.depth[from] > 0 ? volume * grid.salt[from] / grid.depth[from] : 0.0;
                if (fromRim) flows.fromBody.water += volume, flows.fromBody.salt += salt;
                else gain[from] -= volume, saltGain[from] -= salt;
                if (toRim) flows.outflow.water += volume, flows.outflow.salt += salt;
                else gain[to] += volume, saltGain[to] += salt;
            }
        }
        for (const WaterSource& source : grid.sources) {
            const double entered = source.flow * dt / YearSeconds;
            gain[size_t(source.cell)] += entered;
            saltGain[size_t(source.cell)] += entered * grid.sourceSalinity;
            flows.inflow.water += entered;
            flows.inflow.salt += entered * grid.sourceSalinity;
        }
        for (size_t cell = 0; cell < cells; ++cell) {
            if (gain[cell] == 0 && saltGain[cell] == 0) continue;
            grid.depth[cell] = std::max(0.0, grid.depth[cell] + gain[cell] / area);
            grid.salt[cell] = std::max(0.0, grid.salt[cell] + saltGain[cell] / area);
        }
    }
    total += flows;
    // Read over the whole call, so that water sloshing within it does not count as an event.
    double unrest = 0;
    for (size_t cell = 0; cell < cells; ++cell)
        if (std::max(scratch.before[cell], grid.depth[cell]) > StandingWater) unrest = std::max(unrest, std::abs(grid.depth[cell] - scratch.before[cell]) / seconds);
    return unrest;
}

} // namespace derivean::sim

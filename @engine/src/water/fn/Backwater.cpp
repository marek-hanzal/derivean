#include "water/type/WaterTransfer.hpp"

#include <limits>
#include <optional>

namespace derivean {
namespace {

constexpr Cell Up{0, 1, 0};
constexpr int Rate = WaterUnits / 4;

struct Candidate {
    Cell point;
    int64_t head;
};

int64_t headAt(const WaterState& state, Cell point) {
    return int64_t(point.y) * WaterUnits + waterAt(state, point);
}

void equalize(WaterState& state, const VoxelWorld& world, Cell seed,
              std::unordered_set<Cell, CellHash>& visited) {
    std::vector<Cell> queue{seed};
    std::optional<Candidate> donor, outlet;
    bool confined = false;
    for (size_t i = 0; i < queue.size(); ++i) {
        const Cell point = queue[i];
        const Cell above = point + Up;
        const int aboveFill = waterAt(state, above);
        const bool capped = materialAt(world, above) != Material::Air;
        confined |= capped;
        if (aboveFill < WaterUnits && !capped) {
            const Cell surface = aboveFill > 0 ? above : point;
            const int64_t head = headAt(state, surface);
            if (!donor || head > donor->head || (head == donor->head && surface < donor->point))
                donor = Candidate{surface, head};
        }

        for (Cell direction : Neighbors) {
            const Cell adjacent = point + direction;
            if (waterAt(state, adjacent) == WaterUnits) {
                if (visited.insert(adjacent).second) queue.push_back(adjacent);
            } else if (materialAt(world, adjacent) == Material::Air) {
                const int64_t head = headAt(state, adjacent);
                if (!outlet || head < outlet->head || (head == outlet->head && adjacent < outlet->point))
                    outlet = Candidate{adjacent, head};
            }
        }
    }

    if (!confined || !donor || !outlet || donor->point == outlet->point) return;
    const int headTransfer = int(std::clamp((donor->head - outlet->head) / 2, int64_t(0), int64_t(Rate)));
    const int units = std::min({headTransfer, waterAt(state, donor->point), WaterUnits - waterAt(state, outlet->point)});
    addWater(state, donor->point, -units);
    addWater(state, outlet->point, units);
}

} // namespace

void applyBackwater(WaterState& state, const VoxelWorld& world, std::span<const Cell> active) {
    std::unordered_set<Cell, CellHash> visited;
    for (Cell point : active) {
        if (waterAt(state, point) != WaterUnits || visited.contains(point)) continue;
        visited.insert(point);
        equalize(state, world, point, visited);
    }
}

} // namespace derivean

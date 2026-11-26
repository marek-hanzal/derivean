#include "water/type/Water.hpp"

namespace derivean {
namespace {

void face(FaceMesh& result, int axis, int sign, int plane, int u, int v, int width, int height) {
    result.insert(result.end(), {int16_t(axis), int16_t(sign), int16_t(plane), int16_t(u), int16_t(v),
                                 int16_t(width), int16_t(height), int16_t(Material::Water), 0});
}

} // namespace

FaceMesh meshWater(const WaterState& state, const VoxelWorld& world, Cell chunk) {
    FaceMesh result;
    if (!state.chunks.contains(chunk)) return result;
    const Bounds area = chunkBounds(chunk);
    const auto terrain = sampleWorld(world, area.padded(1));
    const Cell up{0, 1, 0}, down{0, -1, 0};
    for (int y = area.min.y; y < area.max.y; ++y)
        for (int z = area.min.z; z < area.max.z; ++z)
            for (int x = area.min.x; x < area.max.x; ++x) {
                const Cell point{x, y, z};
                const int amount = waterAt(state, point);
                if (amount == 0) continue;
                const Cell local = (point - area.min) * WaterUnits;
                if (amount < WaterUnits || (waterAt(state, point + up) == 0 && terrain[point + up] == Material::Air))
                    face(result, 1, 1, local.y + amount, local.z, local.x, WaterUnits, WaterUnits);
                if (waterAt(state, point + down) < WaterUnits && terrain[point + down] == Material::Air)
                    face(result, 1, -1, local.y, local.z, local.x, WaterUnits, WaterUnits);
                for (Cell offset : Neighbors) {
                    if (offset.y) continue;
                    const Cell adjacent = point + offset;
                    if (terrain[adjacent] != Material::Air) continue;
                    const int exposedAbove = waterAt(state, adjacent);
                    if (exposedAbove >= amount) continue;
                    const int axis = offset.x ? 0 : 2;
                    const int sign = offset[axis];
                    const int plane = local[axis] + (sign > 0 ? WaterUnits : 0);
                    if (axis == 0)
                        face(result, axis, sign, plane, local.y + exposedAbove, local.z,
                             amount - exposedAbove, WaterUnits);
                    else
                        face(result, axis, sign, plane, local.x, local.y + exposedAbove,
                             WaterUnits, amount - exposedAbove);
                }
            }
    return result;
}

} // namespace derivean

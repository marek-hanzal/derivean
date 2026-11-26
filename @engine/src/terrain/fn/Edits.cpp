#include "terrain/type/Edits.hpp"

#include <map>

namespace derivean {
namespace {

struct Sum {
    Position weighted{};
    size_t count{};
};

} // namespace

void recordEdits(EditStore& store, std::span<const VoxelChange> changes, double time) {
    std::map<std::pair<Material, Material>, Sum> sums;
    for (const VoxelChange& change : changes) {
        const Cell chunk = chunkOf(change.point);
        auto& edits = store.chunks[chunk];
        if (!edits) {
            edits = std::make_unique<ChunkEdits>();
            edits->fill(Untouched);
            store.levels[{chunk.x, 0, chunk.z}].push_back(chunk.y);
        }
        const Cell local = change.point - chunk * ChunkSide;
        (*edits)[size_t((local.y * ChunkSide + local.z) * ChunkSide + local.x)] = uint8_t(change.after);

        Sum& sum = sums[{change.before, change.after}];
        for (int axis = 0; axis < 3; ++axis) sum.weighted[axis] += (change.point[axis] + 0.5) * VoxelSize;
        ++sum.count;
    }
    for (const auto& [materials, sum] : sums) {
        const double count = double(sum.count);
        store.log.push_back({{sum.weighted[0] / count, sum.weighted[1] / count, sum.weighted[2] / count},
                             time, count * double(VoxelVolume), materials.first, materials.second});
    }
}

std::vector<Edit> takeEdits(EditStore& store) { return std::exchange(store.log, {}); }

} // namespace derivean

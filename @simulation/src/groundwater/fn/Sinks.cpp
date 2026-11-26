#include "groundwater/fn/Stages.hpp"

#include "groundwater/fn/Swallow.hpp"

namespace derivean::sim {

// A stream on soluble rock loses water to it cell after cell. The landscape's discharge is that of
// a stream that loses none, so the share still on the surface is followed down each course here:
// a cell's water is what its donors pass on after their own losses, plus what joins in the cell.
// One sweep carries that one cell downstream. Once the landscape takes the same losses out of its
// own routing (fn/Swallow.hpp), its recorded loss replaces this estimate.
void sinkStreams(WaterField& field) {
    const int size = field.size;
    const size_t cells = field.cells();
    std::vector<float> kept(cells, 1.0f), next(cells, 1.0f);
    const auto lostAt = [&](size_t i, float share) {
        const AquiferColumn& column = field.column[i];
        return column.drain == Drain::None ? swallowed(column.swallow, column.stream * share, float(LocalCell)) : 0.0f;
    };
    for (int sweep = 0; sweep < SinkSweeps; ++sweep) {
        forRows(field, [&](int begin, int end) {
            for (int z = begin; z < end; ++z)
                for (int x = 0; x < size; ++x) {
                    const size_t i = field.index(x, z);
                    float arriving = 0, left = 0;
                    for (int k = 0; k < 8; ++k) {
                        const int nx = x + NeighborX[k], nz = z + NeighborZ[k];
                        if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
                        const size_t j = field.index(nx, nz);
                        const AquiferColumn& donor = field.column[j];
                        if (donor.flowTo != ((k + 4) & 7) || donor.stream <= 0) continue;
                        arriving += donor.stream;
                        left += donor.stream * kept[j] - lostAt(j, kept[j]);
                    }
                    const float joining = std::max(0.0f, field.column[i].stream - arriving);
                    next[i] = arriving + joining > 0 ? std::clamp((left + joining) / (arriving + joining), 0.0f, 1.0f) : 1.0f;
                }
        });
        kept.swap(next);
    }
    for (size_t i = 0; i < cells; ++i) field.column[i].sunk = field.column[i].swallow > 0 ? lostAt(i, kept[i]) : 0.0f;
}

} // namespace derivean::sim

#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"

namespace derivean::sim {

// One sweep from the upwind edge: the line over a cell is the line over the air's place one cell
// upwind, lowered by the fall of that stride, or the cell's own top where that is higher.
void castShadow(const SandBed& bed, Vec2 heading, std::vector<float>& line, std::vector<uint8_t>& shadow) {
    const int size = bed.size;
    const bool alongX = std::abs(heading.x) >= std::abs(heading.z);
    const double major = alongX ? heading.x : heading.z, minor = alongX ? heading.z : heading.x;
    const int forward = major > 0 ? 1 : -1;
    const double sideways = minor / std::abs(major);   // cells across the sweep per cell along it
    const float fall = ShadowTan * float(bed.cell * std::sqrt(1.0 + sideways * sideways));
    line.resize(bed.cells());
    shadow.resize(bed.cells());
    const auto at = [&](int along, int across) { return alongX ? bed.index(along, across) : bed.index(across, along); };
    for (int stride = 0; stride < size; ++stride) {
        const int along = forward > 0 ? stride : size - 1 - stride;
        for (int across = 0; across < size; ++across) {
            const size_t i = at(along, across);
            const float top = bed.top(i);
            if (stride == 0) {
                line[i] = top, shadow[i] = 0;
                continue;
            }
            const double from = std::clamp(across - sideways, 0.0, double(size - 1));
            const int low = int(from), high = std::min(low + 1, size - 1);
            const float part = float(from - low);
            const float cast = line[at(along - forward, low)] * (1 - part) + line[at(along - forward, high)] * part - fall;
            shadow[i] = cast > top + 1e-3f;
            line[i] = std::max(top, cast);
        }
    }
}

} // namespace derivean::sim

#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"

namespace derivean::sim {

// Sweeps from the upwind edge: the line over a cell is the line over the air's place one cell
// upwind, lowered by the fall of that stride, or the cell's own top where that is higher. The bed
// continues across its edges as another piece of the same field standing on this side's ground, so a
// second lap carries on with how high the line stood over the ground at the far edge, until it has
// sunk into the ground everywhere.
void castShadow(const SandBed& bed, Vec2 heading, std::vector<float>& line, std::vector<uint8_t>& shadow) {
    const int size = bed.size;
    const bool alongX = std::abs(heading.x) >= std::abs(heading.z);
    const double major = alongX ? heading.x : heading.z, minor = alongX ? heading.z : heading.x;
    const int forward = major > 0 ? 1 : -1;
    const double sideways = minor / std::abs(major);   // cells across the sweep per cell along it
    const float fall = ShadowTan * float(bed.cell * std::sqrt(1.0 + sideways * sideways));
    line.resize(bed.cells());
    shadow.assign(bed.cells(), 0);
    const auto at = [&](int along, int across) { return alongX ? bed.index(along, across) : bed.index(across, along); };
    const auto wrapped = [&](int index) { return index < 0 ? index + size : index >= size ? index - size : index; };
    for (int across = 0; across < size; ++across) line[at(forward > 0 ? 0 : size - 1, across)] = bed.top(at(forward > 0 ? 0 : size - 1, across));
    for (int stride = 1; stride < 2 * size; ++stride) {
        const int along = forward > 0 ? stride % size : size - 1 - stride % size, before = wrapped(along - forward);
        const bool seam = stride % size == 0;
        bool cast = false;
        for (int across = 0; across < size; ++across) {
            const size_t i = at(along, across);
            const float top = bed.top(i);
            const double from = across - sideways;
            const int low = int(std::floor(from));
            const float part = float(from - low);
            const size_t a = at(before, wrapped(low)), b = at(before, wrapped(low + 1));
            const float over = seam ? top + (line[a] - bed.top(a)) * (1 - part) + (line[b] - bed.top(b)) * part - fall
                                    : line[a] * (1 - part) + line[b] * part - fall;
            const bool shaded = over > top + 1e-3f;
            if (stride < size || shaded || shadow[i]) shadow[i] = shaded, line[i] = std::max(top, over);
            cast = cast || shaded;
        }
        if (stride >= size && !cast) break;
    }
}

} // namespace derivean::sim

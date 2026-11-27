#include "aeolian/fn/Dunes.hpp"

#include "aeolian/constant/Wind.hpp"

namespace derivean::sim {

// Sweeps from the upwind edge: the line over a cell is the line over the air's place one cell
// upwind, lowered by the fall of that stride, or the cell's own top where that is higher. The bed
// continues across its edges as another piece of the same field standing on this side's ground, so a
// second lap carries on with how high the line stood over the ground at the far edge, until it has
// sunk into the ground everywhere.
void castShadow(const SandBed& bed, const std::vector<float>& top, Vec2 heading, std::vector<float>& line, std::vector<float>& turned, std::vector<uint8_t>& shadow) {
    const int size = bed.size;
    const size_t side = size_t(size);
    const bool alongX = std::abs(heading.x) >= std::abs(heading.z);
    const double major = alongX ? heading.x : heading.z, minor = alongX ? heading.z : heading.x;
    const int forward = major > 0 ? 1 : -1;
    const double sideways = minor / std::abs(major);   // cells across the sweep per cell along it
    const float fall = ShadowTan * float(bed.cell * std::sqrt(1.0 + sideways * sideways));
    // The sweep reads the surface and keeps the line in an order of its own, a cell across at
    // along * size + across, so that every stride runs along a row; along x that is the bed turned
    // over its diagonal. Only the shadow is written in the bed's order.
    const float* ground = top.data();
    if (alongX) {
        constexpr size_t Block = 32;
        turned.resize(bed.cells());
        for (size_t z0 = 0; z0 < side; z0 += Block)
            for (size_t x0 = 0; x0 < side; x0 += Block)
                for (size_t z = z0; z < std::min(side, z0 + Block); ++z)
                    for (size_t x = x0; x < std::min(side, x0 + Block); ++x) turned[x * side + z] = top[z * side + x];
        ground = turned.data();
    }
    line.resize(bed.cells());
    shadow.assign(bed.cells(), 0);
    const auto sweep = [&](int along, int across) { return size_t(along) * side + size_t(across); };
    const auto at = [&](int along, int across) { return alongX ? bed.index(along, across) : bed.index(across, along); };
    const auto wrapped = [&](int index) { return index < 0 ? index + size : index >= size ? index - size : index; };
    const int first = forward > 0 ? 0 : size - 1;
    for (int across = 0; across < size; ++across) line[sweep(first, across)] = ground[sweep(first, across)];
    for (int stride = 1; stride < 2 * size; ++stride) {
        const int along = forward > 0 ? stride % size : size - 1 - stride % size, before = wrapped(along - forward);
        const bool seam = stride % size == 0;
        const float* lineBefore = &line[sweep(before, 0)];
        const float* groundBefore = ground + sweep(before, 0);
        const float* groundHere = ground + sweep(along, 0);
        float* lineHere = &line[sweep(along, 0)];
        bool cast = false;
        for (int across = 0; across < size; ++across) {
            const size_t i = at(along, across);
            const float here = groundHere[across];
            const double from = across - sideways;
            const int low = int(std::floor(from));
            const float part = float(from - low);
            const int a = wrapped(low), b = wrapped(low + 1);
            const float over = seam ? here + (lineBefore[a] - groundBefore[a]) * (1 - part) + (lineBefore[b] - groundBefore[b]) * part - fall
                                    : lineBefore[a] * (1 - part) + lineBefore[b] * part - fall;
            const bool shaded = over > here + 1e-3f;
            if (stride < size || shaded || shadow[i]) shadow[i] = shaded, lineHere[across] = std::max(here, over);
            cast = cast || shaded;
        }
        if (stride >= size && !cast) break;
    }
}

} // namespace derivean::sim

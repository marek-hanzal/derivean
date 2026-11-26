#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Water.hpp"

namespace derivean::sim {

void strokeBounds(const Edit& edit, Vec2& low, Vec2& high) {
    const double half = 0.5 * edit.width;
    const Vec2 along = edit.to - edit.from;
    // The square around a point, or the rectangle around a segment: its side reaches half a width
    // across the segment and nothing beyond its ends.
    const Vec2 reach = dot(along, along) > 0 ? Vec2{std::abs(normalized(along).z) * half, std::abs(normalized(along).x) * half} : Vec2{half, half};
    low = {std::min(edit.from.x, edit.to.x) - reach.x, std::min(edit.from.z, edit.to.z) - reach.z};
    high = {std::max(edit.from.x, edit.to.x) + reach.x, std::max(edit.from.z, edit.to.z) + reach.z};
}

// A cell belongs to a stroke when its centre lies in the rectangle of the stroke's width along the
// segment; a stroke whose ends coincide is the square of that width around the point. Water over
// ground that is cut stays on it and stands on the new ground until the solvers move it. Water
// over ground that is built up keeps its surface: what the fill pushes out from under it is put
// on the deepest water beside the stroke, or on the lowest ground there, and finds its level.
void rasterise(World& world, const Edit& edit) {
    Overlay& overlay = world.interventions.overlay;
    const double cell = specOf(Level::Site).cell, area = cell * cell, half = 0.5 * edit.width;
    const Vec2 along = edit.to - edit.from;
    const double span = dot(along, along);
    const auto cellOf = [&](double metres) { return int64_t(std::floor(metres / cell)); };
    Vec2 low, high;
    strokeBounds(edit, low, high);
    const int64_t lowX = cellOf(low.x) - 1, highX = cellOf(high.x) + 1, lowZ = cellOf(low.z) - 1, highZ = cellOf(high.z) + 1;
    EditedTile* tile = nullptr;
    size_t at = 0;
    const auto locate = [&](int64_t gx, int64_t gz) {
        const TileKey key{Level::Site, int32_t(std::floor(double(gx) / TileCells)), int32_t(std::floor(double(gz) / TileCells))};
        if (!tile || !(tile->key == key)) {
            const auto found = overlay.tiles.find({key.z, key.x});
            tile = found == overlay.tiles.end() ? nullptr : found->second.get();
        }
        at = size_t(gz - int64_t(key.z) * TileCells) * TileCells + size_t(gx - int64_t(key.x) * TileCells);
        return tile != nullptr;
    };
    Tally pushed;                    // m of water and kg/m² of salt, summed over cells
    EditedTile* beside = nullptr;    // where it goes
    size_t besideAt = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (int64_t gz = lowZ; gz <= highZ; ++gz) {
            for (int64_t gx = lowX; gx <= highX; ++gx) {
                const Vec2 offset = Vec2{(double(gx) + 0.5) * cell, (double(gz) + 0.5) * cell} - edit.from;
                const double share = span > 0 ? dot(offset, along) / span : 0.0;
                const bool inside = span > 0 ? share >= 0 && share <= 1 && std::abs(cross(along, offset)) <= half * std::sqrt(span)
                                             : std::abs(offset.x) <= half && std::abs(offset.z) <= half;
                if (!locate(gx, gz)) continue;
                const double ground = groundOf(tile->now, at);
                if (pass == 1) {
                    // Second pass: where the pushed water goes. The deepest water the stroke left alone,
                    // which is the body it was pushed out of; with none, the lowest ground beside it.
                    if (inside && edit.kind != EditKind::Lower) continue;
                    const double depth = tile->depth[at], other = beside ? beside->depth[besideAt] : -1.0;
                    if (!beside || depth > other || (depth == 0 && other == 0 && ground < groundOf(beside->now, besideAt))) beside = tile, besideAt = at;
                    continue;
                }
                if (!inside) continue;
                const double target = mix(edit.heightFrom, edit.heightTo, share);
                if (edit.kind != EditKind::Raise && ground > target) {
                    overlay.dug += cutColumn(tile->now, at, ground - target) * area;
                } else if (edit.kind != EditKind::Lower && ground < target) {
                    overlay.filled += layColumn(tile->now, at, target - ground, edit.fill) * area;
                    const double depth = tile->depth[at], left = std::clamp(ground + depth - double(groundOf(tile->now, at)), 0.0, depth);
                    if (depth > left) {
                        const double salt = tile->salt[at] * left / depth;
                        pushed.water += depth - left;
                        pushed.salt += tile->salt[at] - salt;
                        tile->depth[at] = left;
                        tile->salt[at] = salt;
                    }
                } else {
                    continue;
                }
                tile->touched = true;
            }
        }
        if (pushed.water <= 0) break;
    }
    if (pushed.water <= 0) return;
    if (!beside) locate(cellOf(edit.from.x), cellOf(edit.from.z)), beside = tile, besideAt = at;   // nowhere beside it: back onto the fill
    beside->depth[besideAt] += pushed.water;
    beside->salt[besideAt] += pushed.salt;
    beside->touched = true;
}

} // namespace derivean::sim

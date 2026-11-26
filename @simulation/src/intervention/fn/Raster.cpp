#include "intervention/fn/Intervention.hpp"

#include "hydrology/fn/Water.hpp"

namespace derivean::sim {
namespace {

// The live tile that holds site cell (gx, gz), and the cell in its arrays; `tile` is kept between
// calls, as neighbouring cells mostly share one.
bool locateCell(Overlay& overlay, int64_t gx, int64_t gz, EditedTile*& tile, size_t& at) {
    const TileKey key{Level::Site, int32_t(std::floor(double(gx) / TileCells)), int32_t(std::floor(double(gz) / TileCells))};
    if (!tile || !(tile->key == key)) {
        const auto found = overlay.tiles.find({key.z, key.x});
        tile = found == overlay.tiles.end() ? nullptr : found->second.get();
    }
    at = size_t(gz - int64_t(key.z) * TileCells) * TileCells + size_t(gx - int64_t(key.x) * TileCells);
    return tile != nullptr;
}

// Each site cell takes the mean of the grid over its square. The floor founders with its layering,
// datum and all, as the replay lets it (volcanic/fn/Settle.cpp). Rock laid turns the cover it buries
// into rock, as the replay welds it, over the share of the cell the rock covers: the column keeps
// one rock surface with the loose material on it. The water on a cell keeps its depth and stands on the new
// ground until the solvers move it; lava that fills a lake lifts the lake, which then runs off it.
// Nothing is pushed aside, so no water is made or lost.
void layDeposit(Overlay& overlay, const Deposit& laid) {
    const double cell = specOf(Level::Site).cell, area = cell * cell;
    const size_t cells = size_t(std::max(0, laid.width)) * size_t(std::max(0, laid.height));
    if (!(laid.cell > 0) || cells == 0) return;
    const Vec2 far = laid.origin + Vec2{laid.cell * laid.width, laid.cell * laid.height};
    const int64_t lowX = int64_t(std::floor(laid.origin.x / cell)), highX = int64_t(std::ceil(far.x / cell)) - 1;
    const int64_t lowZ = int64_t(std::floor(laid.origin.z / cell)), highZ = int64_t(std::ceil(far.z / cell)) - 1;
    const auto value = [&](const std::vector<float>& field, size_t i) { return field.size() == cells ? double(field[i]) : 0.0; };
    EditedTile* tile = nullptr;
    size_t at = 0;
    for (int64_t gz = lowZ; gz <= highZ; ++gz) {
        for (int64_t gx = lowX; gx <= highX; ++gx) {
            if (!locateCell(overlay, gx, gz, tile, at)) continue;
            // The grid cells under this site cell, and how much of the site cell each covers.
            const double x0 = (double(gx) * cell - laid.origin.x) / laid.cell, x1 = (double(gx + 1) * cell - laid.origin.x) / laid.cell;
            const double z0 = (double(gz) * cell - laid.origin.z) / laid.cell, z1 = (double(gz + 1) * cell - laid.origin.z) / laid.cell;
            double drop = 0, rock = 0, lava = 0, loose[SedimentClasses]{}, buried = 0;   // buried: share of the cell under rock
            for (int z = std::max(0, int(std::floor(z0))); z < std::min(laid.height, int(std::ceil(z1))); ++z) {
                const double across = std::min(z1, double(z + 1)) - std::max(z0, double(z));
                for (int x = std::max(0, int(std::floor(x0))); x < std::min(laid.width, int(std::ceil(x1))); ++x) {
                    const double share = across * (std::min(x1, double(x + 1)) - std::max(x0, double(x))) / ((x1 - x0) * (z1 - z0));
                    const size_t i = size_t(z) * size_t(laid.width) + size_t(x);
                    drop += share * value(laid.drop, i), rock += share * value(laid.rock, i), lava += share * value(laid.lava, i);
                    if (value(laid.rock, i) > 0) buried += share;
                    for (int c = 0; c < SedimentClasses; ++c) loose[c] += share * value(laid.loose[c], i);
                }
            }
            if (drop <= 0 && rock <= 0 && loose[Coarse] <= 0 && loose[Sand] <= 0 && loose[Fines] <= 0) continue;
            // Booked from the column as it came out, so that the budget closes to the rounding of
            // the columns themselves.
            Ground& column = tile->now;
            const auto held = [&] { return double(column.rock[at]) + double(column.sediment[Coarse][at]) + double(column.sediment[Sand][at]) + double(column.sediment[Fines][at]); };
            const double was = held();
            float& top = column.rock[at];
            top = float(double(top) - std::max(0.0, drop));
            column.datum[at] = float(double(column.datum[at]) - std::max(0.0, drop));
            const double sunk = was - held();
            if (rock > 0) {
                if (tile->pile.empty()) tile->pile.assign(column.rock.size(), LivePile{});
                LivePile& pile = tile->pile[at];
                const float datum = column.datum[at];
                if (pile.thickness(top, datum) <= 0) pile = {datum - top, 0.0f, 0.0f, laid.kind, laid.vent};   // none, or cut away: a new one starts here
                for (const auto& layer : column.sediment) pile.laid += std::max(0.0f, layer[at]) * float(std::min(1.0, buried));
                pile.laid += float(rock), pile.lava += float(std::min(rock, lava));
                pile.kind = laid.kind, pile.vent = laid.vent;
                for (auto& layer : column.sediment) {
                    const float welded = std::max(0.0f, layer[at]) * float(std::min(1.0, buried));
                    top += welded, layer[at] -= welded;
                }
                top = float(double(top) + rock);
            }
            for (int c = 0; c < SedimentClasses; ++c) layColumn(column, at, loose[c], SedimentClass(c));
            overlay.dug += sunk * area;
            overlay.filled += (held() - was + sunk) * area;
            tile->touched = true;
        }
    }
}

} // namespace

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
    if (edit.kind == EditKind::Deposit) {
        if (edit.deposit) layDeposit(overlay, *edit.deposit);
        return;
    }
    const double cell = specOf(Level::Site).cell, area = cell * cell, half = 0.5 * edit.width;
    const Vec2 along = edit.to - edit.from;
    const double span = dot(along, along);
    const auto cellOf = [&](double metres) { return int64_t(std::floor(metres / cell)); };
    Vec2 low, high;
    strokeBounds(edit, low, high);
    const int64_t lowX = cellOf(low.x) - 1, highX = cellOf(high.x) + 1, lowZ = cellOf(low.z) - 1, highZ = cellOf(high.z) + 1;
    EditedTile* tile = nullptr;
    size_t at = 0;
    const auto locate = [&](int64_t gx, int64_t gz) { return locateCell(overlay, gx, gz, tile, at); };
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

#include "volcanic/fn/Volcanic.hpp"

#include "core/fn/Hash.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr uint64_t ColumnStream = 0x434f4c53;
constexpr uint64_t NeckStream = 0x4e45434b;
// Column width against sheet thickness: Giant's Causeway, 0.45 m in a flow some 30 m thick;
// Devils Postpile, 0.6–1 m in one of about 120 m; thin flows crack into a hand's breadth.
constexpr float ColumnGrowth = 0.09f;        // m of width per square root of a metre of sheet
constexpr float NarrowestColumn = 0.15f, WidestColumn = 2.5f;   // m
constexpr float JointedSheet = 3.0f;         // m below which a sheet cools too fast to set regular columns
constexpr float JointOpen = 0.15f;           // of a column's width, how far a joint stands open where it weathers
constexpr float JointSkin = 1.2f;            // weathered skin, in column widths, within which the joints stand open
constexpr float MostRecess = 0.6f;           // of a column's width, how far a column weathers back from its face at most
// A column is drawn when it spans this many points of the run; a joint stands open as a gap of its
// own when it is this many points wide, and is drawn at least one point wide then.
constexpr float DrawnColumn = 2.0f, DrawnJoint = 4.0f;
constexpr double ColumnJitter = 0.3;         // of the spacing: hexagons mostly, some five- and seven-sided columns
constexpr double RowHeight = 0.8660254037844386;

// A neck cools from its wall inwards: its columns lie across the conduit, not along it, and
// close to its axis they are too crowded to set apart.
constexpr float NeckCore = 2.0f;             // column widths from the axis within which no joint is drawn

// The column a point lies in, on a jittered triangular lattice whose cells are near-regular
// hexagons, and its distance in column widths to the nearest joint (the nearest side of its cell).
struct ColumnAt {
    int64_t i{}, j{};
    double toJoint{};
};

// Along x the lattice repeats every `period` columns when that is set, so it closes around a ring.
ColumnAt columnAt(uint64_t seed, double x, double z, int64_t period = 0) {
    constexpr int Candidates = 16;
    Vec2 sites[Candidates];
    int64_t keys[Candidates][2];
    int count = 0, nearest = 0;
    const int64_t row = int64_t(std::floor(z / RowHeight));
    for (int64_t j = row - 1; j <= row + 2; ++j) {
        const int64_t column = int64_t(std::floor(x - 0.5 * double(j)));
        for (int64_t i = column - 1; i <= column + 2; ++i) {
            const int64_t key = period > 0 ? ((i % period) + period) % period : i;
            sites[count] = {double(i) + 0.5 * double(j) + ColumnJitter * (unit(hash(seed, key, j, 1)) - 0.5), double(j) * RowHeight + ColumnJitter * (unit(hash(seed, key, j, 2)) - 0.5)};
            keys[count][0] = key, keys[count][1] = j;
            if (length(sites[count] - Vec2{x, z}) < length(sites[nearest] - Vec2{x, z})) nearest = count;
            ++count;
        }
    }
    // The nearest side is the nearest of the planes halfway to every other centre.
    const Vec2 at{x, z}, own = sites[nearest];
    const double near = length(own - at);
    double side = 1e30;
    for (int k = 0; k < count; ++k) {
        if (k == nearest) continue;
        const double apart = length(sites[k] - own);
        if (apart > 0) side = std::min(side, (length(sites[k] - at) * length(sites[k] - at) - near * near) / (2.0 * apart));
    }
    return {keys[nearest][0], keys[nearest][1], side};
}

// A column weathered back at a face, or the joint beside it standing open. `x`, `z` are in
// column widths on the face, `period` closes the lattice around a ring.
bool weathered(uint64_t flow, double x, double z, int64_t period, float width, float depth, float step) {
    if (depth < 0 || depth > JointSkin * width || width < DrawnColumn * step) return false;
    const ColumnAt column = columnAt(flow, x, z, period);
    // Each column weathers back from the face by its own amount, so a face shows them as prisms.
    if (depth < MostRecess * width * float(unit(hash(flow, column.i, column.j, 3)))) return true;
    if (width < DrawnJoint * step) return false;
    return column.toJoint * width < 0.5 * std::max(JointOpen * width, step);
}

float widthFor(float thickness) { return std::clamp(ColumnGrowth * std::sqrt(thickness), NarrowestColumn, WidestColumn); }

} // namespace

bool columnOpen(uint64_t seed, const PilePoint& point, const PileBed& bed, double x, double z, float depth, float step) {
    const float thickness = bed.top - bed.bottom;
    if (!bed.sheet || thickness < JointedSheet) return false;
    const float width = widthFor(thickness);
    // Each flow sets its own columns.
    const uint64_t flow = hash(stream(seed, ColumnStream) ^ point.vent, int64_t(std::lround(bed.bottom * 8.0f)));
    return weathered(flow, x / width, z / width, 0, width, depth, step);
}

Neck neckOf(const VolcanicTile& volcanic, Vec2 position) {
    for (const Vent& vent : volcanic.vents)
        for (const Feeder& feeder : vent.feeders)
            if (length(position - feeder.site) < feeder.radius) return {vent.magma == Magma::Andesite ? Material::Andesite : Material::Basalt, feeder.site, feeder.radius, vent.id};
    return {};
}

bool neckOpen(uint64_t seed, const Neck& neck, double x, double y, double z, float depth, float step) {
    if (neck.rock == Material::Air) return false;
    // It cools across its whole width from both sides.
    const float width = widthFor(2.0f * neck.radius);
    const Vec2 out{x - neck.site.x, z - neck.site.z};
    if (length(out) < NeckCore * width) return false;
    const int64_t around = std::max<int64_t>(3, std::lround(6.283185307179586 * neck.radius / width));
    const double along = std::atan2(out.z, out.x) / 6.283185307179586 * double(around);
    return weathered(hash(stream(seed, NeckStream) ^ neck.vent, 0), along, y / width, around, width, depth, step);
}

} // namespace derivean::sim

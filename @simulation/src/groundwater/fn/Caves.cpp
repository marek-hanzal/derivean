#include "groundwater/fn/Caves.hpp"

#include "groundwater/constant/Hydraulics.hpp"

namespace derivean::sim {
namespace {

// The stretch of a vertical line that lies inside one tube: every point within the radius of the
// axis. The tube is convex, so the stretch is one interval; it is found on each of the three parts
// of the axis a point can be nearest to (either end, or in between) and joined.
bool tubeSpan(const Conduit& conduit, double x, double z, float& bottom, float& top) {
    const double dx = conduit.bx - conduit.ax, dy = conduit.by - conduit.ay, dz = conduit.bz - conduit.az;
    const double length2 = dx * dx + dy * dy + dz * dz, radius2 = double(conduit.radius) * conduit.radius;
    const double px = x - conduit.ax, pz = z - conduit.az;
    double low = 1e30, high = -1e30;
    const auto ball = [&](double cx, double cy, double cz, double from, double to) {
        const double reach2 = radius2 - (x - cx) * (x - cx) - (z - cz) * (z - cz);
        if (reach2 < 0) return;
        const double reach = std::sqrt(reach2), a = std::max(cy - reach, from), b = std::min(cy + reach, to);
        if (a <= b) low = std::min(low, a), high = std::max(high, b);
    };
    if (length2 < 1e-12) {
        ball(conduit.ax, conduit.ay, conduit.az, -1e30, 1e30);
    } else {
        // Along the axis a point at height y sits at t = (flat + (y - ay)·dy) / length²; the ends of
        // the axis are where t leaves [0, 1].
        const double flat = px * dx + pz * dz;
        double atStart = -1e30, atEnd = 1e30;   // heights at which t is 0 and 1
        if (std::abs(dy) > 1e-12) atStart = conduit.ay - flat / dy, atEnd = conduit.ay + (length2 - flat) / dy;
        const bool rising = dy > 1e-12, level = std::abs(dy) <= 1e-12;
        const double t = flat / length2;
        if (level) {
            if (t <= 0) ball(conduit.ax, conduit.ay, conduit.az, -1e30, 1e30);
            else if (t >= 1) ball(conduit.bx, conduit.by, conduit.bz, -1e30, 1e30);
        } else {
            ball(conduit.ax, conduit.ay, conduit.az, rising ? -1e30 : atStart, rising ? atStart : 1e30);
            ball(conduit.bx, conduit.by, conduit.bz, rising ? atEnd : -1e30, rising ? 1e30 : atEnd);
        }
        // Between the ends: |p|² − (p·d)²/length² ≤ radius², a quadratic in the height above the start.
        const double a = 1 - dy * dy / length2, b = -2 * flat * dy / length2, c = px * px + pz * pz - flat * flat / length2 - radius2;
        if (!level || (t > 0 && t < 1)) {
            double from = -1e30, to = 1e30;
            if (a > 1e-12) {
                const double root = b * b - 4 * a * c;
                if (root >= 0) from = conduit.ay + (-b - std::sqrt(root)) / (2 * a), to = conduit.ay + (-b + std::sqrt(root)) / (2 * a);
                else from = 1, to = 0;
            } else if (c > 0) {
                from = 1, to = 0;
            }
            if (!level) from = std::max(from, std::min(atStart, atEnd)), to = std::min(to, std::max(atStart, atEnd));
            if (from <= to) low = std::min(low, from), high = std::max(high, to);
        }
    }
    if (low > high) return false;
    bottom = float(low), top = float(high);
    return top > bottom;
}

// The index cell under a position, and the items listed for it.
bool itemsAt(const CaveField& field, Vec2 position, const uint32_t*& begin, const uint32_t*& end) {
    if (field.cellsPerSide <= 0) return false;
    const double span = tileSpan(Level::Local), cell = span / field.cellsPerSide;
    const int x = std::clamp(int(std::floor((position.x - field.key.x * span) / cell)), 0, field.cellsPerSide - 1);
    const int z = std::clamp(int(std::floor((position.z - field.key.z * span) / cell)), 0, field.cellsPerSide - 1);
    const size_t at = size_t(z) * size_t(field.cellsPerSide) + size_t(x);
    begin = field.cellItems.data() + field.cellStart[at], end = field.cellItems.data() + field.cellStart[at + 1];
    return begin != end;
}

} // namespace

void indexCaves(CaveField& field) {
    field.cellsPerSide = 0;
    field.cellStart.clear(), field.cellItems.clear();
    if (field.conduits.empty()) return;
    const int side = TileCells;
    const double span = tileSpan(Level::Local), cell = span / side;
    const auto range = [&](double low, double high, double origin, int& first, int& last) {
        first = std::clamp(int(std::floor((low - origin) / cell)), 0, side - 1), last = std::clamp(int(std::floor((high - origin) / cell)), 0, side - 1);
    };
    field.cellsPerSide = side;
    field.cellStart.assign(size_t(side) * side + 1, 0);
    // Counted, then filled: conduits stay in their own order inside every cell.
    for (int pass = 0; pass < 2; ++pass) {
        std::vector<uint32_t> next(pass ? field.cellStart : std::vector<uint32_t>());
        for (size_t c = 0; c < field.conduits.size(); ++c) {
            const Conduit& conduit = field.conduits[c];
            int x0, x1, z0, z1;
            range(std::min(conduit.ax, conduit.bx) - conduit.radius, std::max(conduit.ax, conduit.bx) + conduit.radius, field.key.x * span, x0, x1);
            range(std::min(conduit.az, conduit.bz) - conduit.radius, std::max(conduit.az, conduit.bz) + conduit.radius, field.key.z * span, z0, z1);
            for (int z = z0; z <= z1; ++z)
                for (int x = x0; x <= x1; ++x) {
                    const size_t at = size_t(z) * size_t(side) + size_t(x);
                    if (pass) field.cellItems[next[at]++] = uint32_t(c);
                    else ++field.cellStart[at + 1];
                }
        }
        if (pass) break;
        for (size_t at = 0; at < size_t(side) * side; ++at) field.cellStart[at + 1] += field.cellStart[at];
        field.cellItems.resize(field.cellStart.back());
    }
}

int caveSpans(const CaveField& field, Vec2 position, CaveSpan* spans, int capacity) {
    const uint32_t *item, *end;
    if (capacity <= 0 || !itemsAt(field, position, item, end)) return 0;
    // Sorted by their bottoms as they are found, then joined where they touch.
    CaveSpan found[MostCaveSpans];
    int tubes = 0;
    for (; item != end && tubes < MostCaveSpans; ++item) {
        CaveSpan span;
        if (!tubeSpan(field.conduits[*item], position.x, position.z, span.bottom, span.top)) continue;
        int at = tubes++;
        for (; at > 0 && found[at - 1].bottom > span.bottom; --at) found[at] = found[at - 1];
        found[at] = span;
    }
    int count = 0;
    for (int tube = 0; tube < tubes; ++tube) {
        if (count > 0 && found[tube].bottom <= spans[count - 1].top) spans[count - 1].top = std::max(spans[count - 1].top, found[tube].top);
        else if (count < capacity) spans[count++] = found[tube];
        else break;
    }
    return count;
}

int conduitAt(const CaveField& field, double x, double y, double z) {
    const uint32_t *item, *end;
    if (!itemsAt(field, {x, z}, item, end)) return -1;
    for (int tube = 0; item != end && tube < MostCaveSpans; ++item) {
        float bottom, top;
        if (!tubeSpan(field.conduits[*item], x, z, bottom, top)) continue;
        if (y >= bottom && y < top) return int(*item);
        ++tube;
    }
    return -1;
}

bool inCave(const CaveField& field, double x, double y, double z) { return conduitAt(field, x, y, z) >= 0; }

} // namespace derivean::sim

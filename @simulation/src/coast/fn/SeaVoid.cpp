#include "coast/type/SeaVoid.hpp"

#include "coast/type/Shore.hpp"
#include "core/fn/Hash.hpp"
#include "landscape/fn/Forcing.hpp"
#include "rockmass/type/RockMass.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t CaveStream = 0x53454143;
constexpr float FaceRise = 0.5f;        // m of rock over the water from which a shore cell is a face
constexpr float FaceCover = 1.0f;       // m of loose cover over which the rock under it is no face
constexpr float NotchLeast = 0.3f;      // m of cut from which a notch is drawn
constexpr float NotchDeepest = 3.5f;    // m; beyond that the cut is a bench with the notch at its back
// Along a joint that is broken through, rock keeps CrushedStrength of its strength and the waves
// lose height as they run in: the cave ends where they are down to what that rock withstands,
// CaveReach metres for every e-fold of excess.
constexpr float CaveReach = 6.0f;
constexpr float CaveLeast = 2.0f, CaveLongest = 30.0f;
constexpr double CaveShare = 0.6;       // share of such joints that have opened
// Joints closer than this are the fabric of a face, not a cave; planes more than three site
// cells apart are in the site surface already, as the coves the waves cut along them.
constexpr float JointLeast = 2.5f;
constexpr int MostSpans = 8;

struct SiteCell {
    const Tile* tile{};
    size_t at{};
    float ground() const { return tile->now.rock[at] + loose(); }
    float loose() const { return tile->now.sediment[0][at] + tile->now.sediment[1][at] + tile->now.sediment[2][at]; }
    bool wet() const { return tile->water[at] > ground() + Film; }
};

SiteCell cellAt(const SiteTiles& tiles, int64_t gx, int64_t gz) {
    const int32_t tx = int32_t(std::floor(double(gx) / TileCells)), tz = int32_t(std::floor(double(gz) / TileCells));
    return {tiles(tx, tz), size_t(gz - int64_t(tz) * TileCells) * TileCells + size_t(gx - int64_t(tx) * TileCells)};
}

// The caves of one face: for each joint set, the plane that meets the face nearest its middle.
void openCaves(const Setting& setting, int64_t gx, int64_t gz, int side, Vec2 from, Vec2 to, Vec2 inward, float water, float struck, float datum, std::vector<SeaVoid>& out) {
    const double cell = specOf(Level::Site).cell;
    const Vec2 centre{(double(gx) + 0.5) * cell, (double(gz) + 0.5) * cell};
    const StrataColumn column = strataAt(*setting.planet, planetPoint(setting.seed, centre));
    const float depth = std::max(0.0f, datum - water);
    const StrataUnit layer = unitAt(column, depth, foldAt(setting.seed, column, column.material));
    const Bed bed = bedAt(setting.seed, column, layer, depth, column.material);
    const RockMass mass = rockMassAt(setting.seed, column, bed, layer.laidDown, column.material);
    const MaterialDefinition& rock = definitionOf(bed.rock);
    const float sound = rock.massCohesion > 0 ? std::clamp(mass.cohesion / rock.massCohesion, 0.05f, 4.0f) : 1.0f;
    const float withstand = Withstand * std::sqrt(rock.compressiveStrength * sound * CrushedStrength);
    if (struck <= withstand) return;
    const float length = std::min(CaveLongest, CaveReach * std::log(struck / std::max(1e-3f, withstand)));
    if (length < CaveLeast) return;
    const auto materialOf = [&](Vec2 p) { return column.material + (p - centre); };
    for (int set = 0; set < JointSets; ++set) {
        int order = 0;
        float spacing = mass.sets[set].spacing;
        while (order < JointOrders && spacing < JointLeast) spacing *= OrderStep, ++order;
        if (order >= JointOrders || spacing > 3.0f * float(cell)) continue;
        Vec2 run{std::cos(mass.sets[set].strike), std::sin(mass.sets[set].strike)};
        const double into = dot(run, inward);
        if (std::abs(into) < 0.5) continue;   // a joint along the face parts a slab from it; it opens no cave
        if (into < 0) run = run * -1.0;
        if (unit(hash(stream(setting.seed, CaveStream), gx, gz, side * JointSets + set)) >= CaveShare) continue;
        // Where the nearest broken plane of the set crosses the face, looked for along it a little inside the rock.
        Vec2 mouth{};
        float nearest = 1e9f;
        for (int k = 0; k < 6; ++k) {
            const Vec2 p = from + (to - from) * ((k + 0.5) / 6.0) + inward * 0.3;
            const float distance = jointDistance(setting.seed, mass, set, order, materialOf(p), depth);
            if (distance < nearest) nearest = distance, mouth = p;
        }
        if (nearest > 0.5f * float(cell) / 6.0f + 0.2f) continue;
        const float wide = 0.5f + 0.05f * length, high = 1.0f + 0.2f * length;
        out.push_back({mouth.x - run.x, mouth.z - run.z, mouth.x + run.x * length, mouth.z + run.z * length, water - 0.3f + 0.7f * high, wide, high, water, SeaVoidKind::Cave});
    }
}

// Sorted, merged spans of one vertical line; at most MostSpans are kept.
int spansAt(const SeaVoids& voids, Vec2 position, CaveSpan* spans, int capacity) {
    CaveSpan found[MostSpans];
    int count = 0;
    for (const SeaVoid& v : voids.voids) {
        const Vec2 a{v.ax, v.az}, along = Vec2{v.bx, v.bz} - a;
        const double reach = dot(along, along), t = reach > 0 ? saturate(dot(position - a, along) / reach) : 0.0;
        const double away = length(position - (a + along * t));
        if (away >= v.wide) continue;
        const float half = v.high * float(std::sqrt(1.0 - (away / v.wide) * (away / v.wide)));
        CaveSpan span{v.y - half, v.y + half};
        // Kept in rising order of their floors; one that touches another joins it.
        int at = 0;
        while (at < count && found[at].top < span.bottom) ++at;
        int end = at;
        while (end < count && found[end].bottom <= span.top) span.bottom = std::min(span.bottom, found[end].bottom), span.top = std::max(span.top, found[end].top), ++end;
        if (end == at) {
            if (count == MostSpans) continue;
            for (int k = count; k > at; --k) found[k] = found[k - 1];
            ++count;
        } else {
            for (int k = end; k < count; ++k) found[at + 1 + k - end] = found[k];
            count -= end - at - 1;
        }
        found[at] = span;
    }
    const int written = std::min(count, capacity);
    for (int k = 0; k < written; ++k) spans[k] = found[k];
    return written;
}

bool solidRock(Material material) {
    const MaterialDefinition& d = definitionOf(material);
    return d.phase == Phase::Solid && (d.origin == Origin::Igneous || d.origin == Origin::Metamorphic || d.origin == Origin::Sedimentary);
}

} // namespace

SeaVoids seaVoidsOver(const SiteTiles& tiles, const Setting& setting, double minX, double minZ, double maxX, double maxZ) {
    SeaVoids result;
    const double cell = specOf(Level::Site).cell;
    const int64_t lowX = int64_t(std::floor((minX - SeaVoidReach) / cell)), highX = int64_t(std::floor((maxX + SeaVoidReach) / cell));
    const int64_t lowZ = int64_t(std::floor((minZ - SeaVoidReach) / cell)), highZ = int64_t(std::floor((maxZ + SeaVoidReach) / cell));
    std::vector<SeaVoid> made;
    for (int64_t gz = lowZ; gz <= highZ; ++gz) {
        for (int64_t gx = lowX; gx <= highX; ++gx) {
            const SiteCell here = cellAt(tiles, gx, gz);
            if (!here.tile || !here.tile->shore || here.wet() || here.loose() > FaceCover) continue;
            const ShoreRecord& shore = *here.tile->shore;
            float struck = 0;
            for (int k = 0; k < WaveDirections; ++k) struck = std::max(struck, WaveQuantum * float(shore.height[here.at * WaveDirections + size_t(k)]));
            for (int side = 0; side < 4; ++side) {
                const int dx = NeighborX[side * 2], dz = NeighborZ[side * 2];
                const SiteCell beside = cellAt(tiles, gx + dx, gz + dz);
                if (!beside.tile || !beside.wet()) continue;
                const float water = beside.tile->water[beside.at];
                if (here.tile->now.rock[here.at] < water + FaceRise) continue;
                // The edge shared with the water, and the way from it into the rock.
                const double x0 = double(gx) * cell, z0 = double(gz) * cell;
                const Vec2 from{x0 + (dx > 0 ? cell : 0), z0 + (dz > 0 ? cell : 0)}, to{x0 + (dx >= 0 ? cell : 0), z0 + (dz >= 0 ? cell : 0)};
                const Vec2 inward{double(-dx), double(-dz)};
                made.clear();
                if (shore.notch[here.at] >= NotchLeast) {
                    const float swing = water <= float(here.tile->seaLevel) + SeaBand ? SeaSwing : LakeSwing;
                    made.push_back({from.x, from.z, to.x, to.z, water + 0.15f, std::min(shore.notch[here.at], NotchDeepest), std::clamp(0.4f * swing + 0.3f * struck, 0.4f, 1.6f), water,
                                    SeaVoidKind::Notch});
                }
                if (setting.planet && struck > 0) openCaves(setting, gx, gz, side, from, to, inward, water, struck, here.tile->now.datum[here.at], made);
                for (const SeaVoid& v : made)
                    if (std::max(v.ax, v.bx) + v.wide >= minX && std::min(v.ax, v.bx) - v.wide <= maxX && std::max(v.az, v.bz) + v.wide >= minZ && std::min(v.az, v.bz) - v.wide <= maxZ)
                        result.voids.push_back(v);
            }
        }
    }
    return result;
}

int seaVoidSpans(const SeaVoids& voids, Vec2 position, CaveSpan* spans, int capacity) { return spansAt(voids, position, spans, capacity); }

bool inSeaVoid(const SeaVoids& voids, double x, double y, double z) {
    CaveSpan spans[MostSpans];
    const int count = spansAt(voids, {x, z}, spans, MostSpans);
    for (int k = 0; k < count; ++k)
        if (y >= spans[k].bottom && y < spans[k].top) return true;
    return false;
}

void cutSeaVoids(const SeaVoids& voids, double x, double z, double bottom, double step, int count, Material* out, size_t stride) {
    if (voids.voids.empty()) return;
    CaveSpan spans[MostSpans];
    const int found = spansAt(voids, {x, z}, spans, MostSpans);
    for (int v = 0; v < found; ++v) {
        const int first = std::max(0, int(std::ceil((spans[v].bottom - bottom) / step - 0.5))), last = std::min(count - 1, int(std::ceil((spans[v].top - bottom) / step - 0.5)) - 1);
        for (int k = first; k <= last; ++k)
            if (solidRock(out[size_t(k) * stride])) out[size_t(k) * stride] = Material::Air;
    }
}

} // namespace derivean::sim

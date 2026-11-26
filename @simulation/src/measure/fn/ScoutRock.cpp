#include "measure/fn/Scout.hpp"

#include "material/constant/Materials.hpp"
#include "measure/fn/Format.hpp"
#include "simulation/fn/Query.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr float ShowsRock = 1.0f;     // m of loose cover under which a cliff still shows its rock
constexpr double FaceSlope = 0.35;    // rise over run of a local cell that holds a cliff band
constexpr int FormColumns = 4;        // roofed columns that make one formation rather than a stray voxel
constexpr double PatchEdge = 16.0;    // m, side of the squares a block of ground is sampled in
constexpr double PatchHeight = 160.0; // m, tallest column sampled over one of them
constexpr double FormDepth = 40.0;    // m below the lowest ground of a patch that slots and arches may still reach

// What one cell offers: negative when it does not qualify, else how close it is to a 45° face on level beds.
double sandstoneScore(const Mosaic& mosaic, int x, int z, double& slope, double& dip) {
    const Material rock = Material(mosaic.rock.at(x, z));
    if (rock != Material::Sandstone && rock != Material::Conglomerate) return -1;
    if (mosaic.flooded(x, z) || mosaic.ground.at(x, z) < mosaic.seaLevel) return -1;
    // A region cell averages the cliff with the benches around it, so it is held to a gentler slope and a thicker mantle.
    const bool coarse = mosaic.cell > 200;
    if (mosaic.cover.at(x, z) > (coarse ? 3 * ShowsRock : ShowsRock)) return -1;
    const auto gradient = [&](const Field<float>& field) {
        return std::hypot(field.at(x + 1, z) - field.at(x - 1, z), field.at(x, z + 1) - field.at(x, z - 1)) / (2 * mosaic.cell);
    };
    slope = gradient(mosaic.ground), dip = gradient(mosaic.datum);
    if (slope < (coarse ? 0.25 * FaceSlope : FaceSlope)) return -1;
    // Rock forms belong to cliff bands a viewer can stand under, with beds lying near level: a face
    // around 45° scores best, a wall of several hundred per cent no better than a bank.
    return 1.0 / (1.0 + std::abs(slope - 1.0) + 4 * dip);
}

bool solid(Material material) { return material != Material::Air && material != Material::Water; }

bool lithified(Material material) {
    const Origin origin = definitionOf(material).origin;
    return origin == Origin::Igneous || origin == Origin::Metamorphic || origin == Origin::Sedimentary;
}

} // namespace

std::vector<ScoutSite> findSandstone(const Mosaic& mosaic, int limit) {
    std::vector<ScoutSite> sites;
    const int side = mosaic.side();
    for (int z = 1; z < side - 1; ++z) {
        for (int x = 1; x < side - 1; ++x) {
            double slope = 0, dip = 0;
            const double score = sandstoneScore(mosaic, x, z, slope, dip);
            if (score < 0) continue;
            ScoutSite site;
            site.at = cellCentre(mosaic, x, z);
            site.ground = mosaic.ground.at(x, z);
            site.level = mosaic.level;
            site.score = score;
            site.reason = formatText("bare %s on a %.0f %% slope under %.1f m of cover; the layering dips %.1f m per 100 m", definitionOf(Material(mosaic.rock.at(x, z))).name.data(),
                                     100 * slope, double(mosaic.cover.at(x, z)), 100 * dip);
            // Seen from below the face.
            const Vec2 down = normalized({double(mosaic.ground.at(x - 1, z) - mosaic.ground.at(x + 1, z)), double(mosaic.ground.at(x, z - 1) - mosaic.ground.at(x, z + 1))});
            site.stand = site.at + down * std::max(150.0, 2.5 * mosaic.cell);
            site.lookUp = 8;
            sites.push_back(site);
        }
    }
    return keepApart(std::move(sites), limit, std::max(1500.0, 3 * mosaic.cell));
}

void countSandstone(const Mosaic& mosaic, int tileX, int tileZ, size_t& land, size_t& fit) {
    for (int z = std::max(1, tileZ * TileCells); z < std::min(mosaic.side() - 1, (tileZ + 1) * TileCells); ++z)
        for (int x = std::max(1, tileX * TileCells); x < std::min(mosaic.side() - 1, (tileX + 1) * TileCells); ++x) {
            if (mosaic.flooded(x, z) || mosaic.ground.at(x, z) < mosaic.seaLevel) continue;
            ++land;
            double slope, dip;
            fit += sandstoneScore(mosaic, x, z, slope, dip) >= 0;
        }
}

RockForms rockForms(World& world, Vec2 centre, double edge, double step) {
    RockForms forms;
    const int patch = int(PatchEdge / step), count = std::max(1, int(edge / PatchEdge)) * patch;   // whole patches only
    std::vector<uint8_t> roofed(size_t(count) * size_t(count), 0);
    std::vector<int64_t> beds(size_t(PatchHeight / step) + 1, 0);   // voxels of rock lying in beds of each thickness
    std::vector<Material> materials;
    int roofedColumns = 0, deepest = 0;
    // A cliff climbs faster than a block is tall, so the square is read patch by patch, each over
    // the heights its own ground spans.
    for (int pz = 0; pz < count; pz += patch) {
        for (int px = 0; px < count; px += patch) {
            const Vec2 corner{centre.x - 0.5 * edge + px * step, centre.z - 0.5 * edge + pz * step};
            float low = 1e30f, high = -1e30f;
            for (int j = 0; j <= 2; ++j)
                for (int i = 0; i <= 2; ++i) {
                    const float ground = surveyAt(world, {corner.x + 0.5 * i * PatchEdge, corner.z + 0.5 * j * PatchEdge}).ground;
                    low = std::min(low, ground), high = std::max(high, ground);
                }
            const int tall = std::min(int(PatchHeight / step), int((high - low + FormDepth + 12.0) / step));
            materials.assign(size_t(patch) * size_t(patch) * size_t(tall), Material::Air);
            sampleBlock(world, {corner.x, std::floor(low) - FormDepth, corner.z, step, patch, tall, patch}, materials.data());
            for (int z = 0; z < patch; ++z) {
                for (int x = 0; x < patch; ++x) {
                    const auto at = [&](int y) { return materials[(size_t(y) * size_t(patch) + size_t(z)) * size_t(patch) + size_t(x)]; };
                    bool rock = false, roof = false, capped = false;
                    int gap = 0, run = 0;
                    Material bed = Material::Air;
                    for (int y = tall - 1; y >= 0; --y) {
                        const Material here = at(y);
                        if (!solid(here)) {
                            if (rock) ++gap;
                            // Air ends a run without closing it: what stood above it is a ledge, not a whole bed.
                            run = 0, bed = Material::Air, capped = false;
                            continue;
                        }
                        // Rock again under a gap that itself lies under rock: the gap has a roof.
                        if (rock && gap > 0) roof = true, deepest = std::max(deepest, gap);
                        gap = 0;
                        rock = true;
                        if (here == bed) {
                            ++run;
                            continue;
                        }
                        // A bed is a run of one lithified rock with lithified rock right above and right below it.
                        if (run > 0 && capped && lithified(bed) && lithified(here)) beds[size_t(std::min(run, int(beds.size()) - 1))] += run;
                        capped = run > 0 && lithified(bed);
                        run = 1, bed = here;
                    }
                    forms.columns += rock;
                    if (roof) roofed[size_t(pz + z) * size_t(count) + size_t(px + x)] = 1, ++roofedColumns;
                }
            }
        }
    }
    if (forms.columns > 0) forms.roofedShare = double(roofedColumns) / forms.columns;
    forms.tallest = deepest * step;
    // The bed a point of rock most typically lies in: half of all bedded rock is in beds this thick or thinner.
    int64_t bedded = 0, counted = 0;
    for (const int64_t voxels : beds) bedded += voxels;
    for (size_t run = 1; run < beds.size() && bedded > 0; ++run) {
        counted += beds[run];
        if (2 * counted < bedded) continue;
        forms.bedThickness = double(run) * step;
        break;
    }

    // Patches of roofed columns in plan, and how far each lies from its nearest neighbour.
    std::vector<Vec2> patches;
    std::vector<int32_t> stack;
    for (int start = 0; start < count * count; ++start) {
        if (roofed[size_t(start)] != 1) continue;
        stack.assign(1, start);
        roofed[size_t(start)] = 2;
        double sumX = 0, sumZ = 0;
        int cells = 0;
        while (!stack.empty()) {
            const int cell = stack.back();
            stack.pop_back();
            sumX += cell % count, sumZ += cell / count, ++cells;
            for (const int offset : {1, -1, count, -count}) {
                const int other = cell + offset;
                if (other < 0 || other >= count * count || (std::abs(offset) == 1 && other / count != cell / count) || roofed[size_t(other)] != 1) continue;
                roofed[size_t(other)] = 2;
                stack.push_back(other);
            }
        }
        if (cells >= FormColumns) patches.push_back(Vec2{sumX / cells, sumZ / cells} * step);
    }
    forms.formations = int(patches.size());
    if (patches.size() < 2) return forms;
    double sum = 0, squares = 0;
    for (const Vec2& from : patches) {
        double nearest = 1e30;
        for (const Vec2& other : patches)
            if (&other != &from) nearest = std::min(nearest, length(other - from));
        sum += nearest, squares += nearest * nearest;
    }
    forms.spacing = sum / double(patches.size());
    forms.spacingCv = std::sqrt(std::max(0.0, squares / double(patches.size()) - forms.spacing * forms.spacing)) / forms.spacing;
    return forms;
}

} // namespace derivean::sim

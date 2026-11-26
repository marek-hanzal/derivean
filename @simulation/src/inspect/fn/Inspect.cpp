#include "inspect/fn/Inspect.hpp"

#include "inspect/fn/Provenance.hpp"
#include "landscape/fn/Forcing.hpp"
#include "measure/constant/Angle.hpp"
#include "ore/type/Ore.hpp"
#include "planet/fn/Heat.hpp"
#include "planet/fn/Sample.hpp"
#include "simulation/fn/Query.hpp"
#include "simulation/fn/Reader.hpp"

namespace derivean::sim {
namespace {

constexpr float ColumnStep = 0.5f;      // m between the samples of the listed column; the topmost one names the surface material

bool loose(Material material) { return definitionOf(material).origin == Origin::Unconsolidated || definitionOf(material).origin == Origin::Organic; }

// Nearest finished cell of a level under the point.
struct CellRef {
    std::shared_ptr<const Tile> tile;
    size_t index{};
};

CellRef cellAt(const World& world, Level level, Vec2 position) {
    const double cell = specOf(level).cell;
    const TileKey key = tileAt(level, position);
    const int x = std::clamp(int(std::floor(position.x / cell)) - key.x * TileCells, 0, TileCells - 1);
    const int z = std::clamp(int(std::floor(position.z / cell)) - key.z * TileCells, 0, TileCells - 1);
    return {world.atlas.find(key), size_t(z) * TileCells + size_t(x)};
}

InspectRates ratesAt(const World& world, Level level, Vec2 position) {
    InspectRates rates;
    rates.level = level;
    const Window window = windowAt(level, world.options.age);
    rates.window = window.end - window.start;
    const CellRef cell = cellAt(world, level, position);
    if (!cell.tile) return rates;
    rates.upliftNow = cell.tile->uplift[cell.index], rates.upliftTotal = cell.tile->uplifted[cell.index];
    rates.erosionNow = cell.tile->erosionRate[cell.index], rates.erosionTotal = cell.tile->eroded[cell.index];
    rates.depositionNow = cell.tile->depositionRate[cell.index], rates.depositionTotal = cell.tile->deposited[cell.index];
    return rates;
}

// The voxel column under the point as runs of one material, the first solid rock in it, and what
// lies at its top. The voxel surface may sit a little below the surveyed ground, so the surface
// material is the first sample that is not air.
void readColumn(World& world, Vec2 position, float ground, InspectRock& rock, Material& surface) {
    const int count = int(InspectDepth / ColumnStep);
    std::vector<Material> materials(size_t(count), Material::Air);
    sampleBlock(world, {position.x - 0.5 * ColumnStep, double(ground) - count * ColumnStep, position.z - 0.5 * ColumnStep, ColumnStep, 1, count, 1}, materials.data());
    bool solid = false;
    for (int i = count - 1; i >= 0; --i) {
        const Material material = materials[size_t(i)];
        const float top = float(count - 1 - i) * ColumnStep;
        if (rock.column.empty() || rock.column.back().material != material) rock.column.push_back({top, top + ColumnStep, material});
        else rock.column.back().bottom = top + ColumnStep;
        if (surface == Material::Air) surface = material;
        if (!solid && material != Material::Air && !loose(material)) rock.bed = material, solid = true;
    }
}

} // namespace

InspectRecord inspectAt(World& world, Vec2 position) {
    InspectRecord record;
    record.position = position;
    const SurfaceSample sample = surveyAt(world, position, Level::Ground);
    record.found = sample.found;
    record.crust = inspectCrust(world.planet, world.options.seed, position);
    if (!sample.found) return record;

    // Slope from the site surface one cell either side: the scale of a slope a viewer stands on,
    // above the stones and steps of the ground level.
    TileReader reader{world.atlas, Level::Site};
    const double cell = specOf(Level::Site).cell;
    const auto groundAt = [&](double dx, double dz) {
        SurfaceSample other;
        return readSurface(reader, {position.x + dx, position.z + dz}, other) ? other.ground : sample.ground;
    };
    const double fallX = (groundAt(-cell, 0) - groundAt(cell, 0)) / (2 * cell), fallZ = (groundAt(0, -cell) - groundAt(0, cell)) / (2 * cell);
    const double slope = std::hypot(fallX, fallZ);
    float steepest = 0;
    for (int ring = 1; ring <= 2; ++ring)
        for (int k = 0; k < 8; ++k) {
            const double reach = ring * cell * NeighborReach[k];
            steepest = std::max(steepest, float((groundAt(ring * cell * NeighborX[k], ring * cell * NeighborZ[k]) - groundAt(0, 0)) / reach));
        }

    record.surface.ground = sample.ground;
    record.surface.datum = sample.datum;
    record.surface.incision = sample.datum - sample.rock;
    record.surface.slope = float(std::atan(slope) * DegreesPerRadian);
    // North is -Z and east +X, so a fall towards +X reads 90°.
    const double aspect = std::atan2(fallX, -fallZ) * DegreesPerRadian;
    record.surface.aspect = slope > 1e-6 ? float(aspect < 0 ? aspect + 360 : aspect) : 0.0f;
    record.surface.level = sample.level;

    record.cover.coarse = sample.sediment[Coarse], record.cover.sand = sample.sediment[Sand], record.cover.fines = sample.sediment[Fines];
    record.cover.thickness = sample.ground - sample.rock;

    const Setting& setting = world.atlas.setting();
    const StrataColumn strata = strataAt(world.planet, planetPoint(setting.seed, position));
    const StrataUnit unit = unitAt(strata, std::max(0.0f, sample.datum - sample.rock), foldAt(setting.seed, strata, strata.material));
    record.rock.modelBed = sample.bedrock;
    record.rock.bed = sample.bedrock;
    readColumn(world, position, sample.ground, record.rock, record.cover.surface);
    record.rock.unit = unit.slot;
    record.rock.depositedAgo = unit.slot >= 0 ? world.planet.time - unit.laidDown : 0.0;
    record.rock.facies = unit.facies;
    record.rock.basement = strata.basement;
    record.rock.deformedAgo = world.planet.time - strata.deformed;
    record.rock.aboveUnconformity = unit.slot >= 0 && unit.laidDown >= strata.deformed;

    const Geotherm heat = geothermAt(world.planet, planetPoint(setting.seed, position));
    record.heat = {heat.gradient, heat.magmatic, heat.sinceMelt, heat.centre, rockTemperature(heat, sample.warmth, 100.0f)};
    const OreField ore = oreFieldAt(setting.seed, strata, strata.material);
    record.veins.veining = ore.veining;
    record.veins.sealedAgo = world.planet.time - ore.emplaced;
    record.veins.pipeRadius = ore.pipeRadius;
    record.veins.pipeAgo = world.planet.time - ore.pipeEmplaced;
    record.veins.cutting = cuttingRock(setting.seed, ore, strata.material, std::max(0.0f, sample.datum - sample.rock), unit.laidDown);
    record.rock.coverTotal = strata.coverTotal;
    record.rock.jointSpacing = definitionOf(record.rock.bed).jointSpacing;
    record.rock.bedding = definitionOf(record.rock.bed).bedding;

    record.rates = ratesAt(world, sample.level, position);
    record.regional = ratesAt(world, Level::Region, position);
    if (const CellRef region = cellAt(world, Level::Region, position); region.tile) {
        record.isostaticNow = region.tile->rebound[region.index];
        record.tectonicNow = region.tile->uplift[region.index] - region.tile->rebound[region.index];
    }

    record.water.rain = sample.rain;
    record.water.runoff = runoffOf(sample.rain, sample.warmth);
    record.water.discharge = sample.discharge;
    record.water.standing = sample.water > sample.ground;
    record.water.level = sample.water;
    record.water.depth = std::max(0.0f, sample.water - sample.ground);
    record.water.seaLevel = world.planet.seaLevel;
    record.air.warmth = sample.warmth;
    const Vec2 point = planetPoint(setting.seed, position);
    record.air.wind = {samplePlanet(world.planet.windX, point, Kernel::Linear), samplePlanet(world.planet.windZ, point, Kernel::Linear)};

    SurfaceEvidence evidence;
    evidence.surface = record.cover.surface;
    evidence.bed = record.rock.bed;
    for (int c = 0; c < SedimentClasses; ++c) evidence.cover[c] = sample.sediment[c];
    evidence.slope = float(slope);
    evidence.steepestAbove = steepest;
    evidence.discharge = sample.discharge;
    // A finer level starts on what its parent left, so the deposit under a point is the sum over the levels.
    evidence.depositionRate = record.rates.depositionNow, evidence.erosionRate = record.rates.erosionNow;
    for (int level = int(Level::Region); level <= int(sample.level); ++level) {
        const InspectRates rates = ratesAt(world, Level(level), position);
        evidence.deposited += rates.depositionTotal, evidence.eroded += rates.erosionTotal;
    }
    evidence.waterDepth = record.water.depth;
    evidence.aboveSea = float(sample.ground - world.planet.seaLevel);
    evidence.lake = record.water.standing && sample.water > world.planet.seaLevel + 0.5;
    record.provenance = explainSurface(evidence);
    return record;
}

} // namespace derivean::sim

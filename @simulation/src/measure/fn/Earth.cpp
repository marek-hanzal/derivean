#include "measure/fn/Earth.hpp"

#include "core/fn/Hash.hpp"
#include "landscape/type/Terrain.hpp"
#include "material/constant/Materials.hpp"
#include "measure/fn/Regularity.hpp"
#include "strata/type/Column.hpp"

namespace derivean::sim {
namespace {

constexpr double Bands[6] = {200, 500, 1000, 2000, 3000, 4000};
constexpr double Rough[6] = {5, 10, 20, 40, 80, 160};
constexpr double ShortWindow = 5000.0, LongWindow = 55000.0;   // m: local relief, and half a degree of arc

// Steepest way down over the planet grid from every cell; true where it ends above the sea.
std::vector<uint8_t> endsInland(const Planet& planet) {
    const int n = PlanetCells;
    std::vector<int32_t> next(size_t(n) * n, -1);
    for (int z = 0; z < n; ++z)
        for (int x = 0; x < n; ++x) {
            float lowest = planet.elevation.at(x, z);
            for (int k = 0; k < 8; ++k) {
                const int ox = wrapIndex(x + NeighborX[k], n), oz = wrapIndex(z + NeighborZ[k], n);
                if (planet.elevation.at(ox, oz) < lowest) lowest = planet.elevation.at(ox, oz), next[size_t(z) * n + size_t(x)] = oz * n + ox;
            }
        }
    std::vector<uint8_t> inland(next.size(), 2);   // 2 = not yet known
    std::vector<int32_t> path;
    for (int32_t start = 0; start < int32_t(next.size()); ++start) {
        path.clear();
        int32_t cell = start;
        while (inland[size_t(cell)] == 2 && next[size_t(cell)] >= 0) path.push_back(cell), cell = next[size_t(cell)];
        const uint8_t answer = inland[size_t(cell)] != 2 ? inland[size_t(cell)] : uint8_t(planet.elevation.cells[size_t(cell)] > planet.seaLevel);
        inland[size_t(cell)] = answer;
        for (const int32_t passed : path) inland[size_t(passed)] = answer;
    }
    return inland;
}

} // namespace

PlanetLikeness likenessOf(const Planet& planet) {
    PlanetLikeness likeness;
    const std::vector<uint8_t> inland = endsInland(planet);
    Field<uint8_t> dry(PlanetCells, PlanetCells);
    std::vector<float> heights;
    double sum = 0;
    size_t closed = 0, arid = 0;
    for (int z = 0; z < PlanetCells; ++z) {
        for (int x = 0; x < PlanetCells; ++x) {
            const size_t i = planet.elevation.index(x, z);
            const double above = planet.elevation.cells[i] - planet.seaLevel;
            if (above <= 0) continue;
            dry.cells[i] = 1;
            heights.push_back(float(above));
            sum += above;
            size_t band = 0;
            while (band < 6 && above >= Bands[band]) ++band;
            likeness.bands[band] += 1;
            closed += inland[i];
            const float rain = planet.precipitation.cells[i], demand = std::clamp(0.25f + 0.035f * planet.temperature.cells[i], 0.05f, 1.6f);
            arid += rain < 0.65f * demand;
            // The rock at the top of the layering under the middle of the cell.
            const StrataColumn column = strataAt(planet, planetCellCenter(x, z));
            const Material rock = bedAt(planet.seed, column, unitAt(column, 0, foldAt(planet.seed, column, column.material)), 0, column.material).rock;
            const Origin origin = definitionOf(rock).origin;
            likeness.sedimentary += origin == Origin::Sedimentary || origin == Origin::Unconsolidated;
            likeness.carbonate += rock == Material::Limestone || rock == Material::Chalk;
            likeness.metamorphic += origin == Origin::Metamorphic;
            const bool lava = rock == Material::Basalt || rock == Material::Andesite || rock == Material::Tuff;
            likeness.volcanic += origin == Origin::Igneous && lava;
            likeness.plutonic += origin == Origin::Igneous && !lava;
        }
    }
    const double land = double(std::max<size_t>(1, heights.size()));
    likeness.landShare = double(heights.size()) / double(planet.elevation.size());
    for (double& band : likeness.bands) band /= land;
    for (double* share : {&likeness.sedimentary, &likeness.carbonate, &likeness.metamorphic, &likeness.plutonic, &likeness.volcanic}) *share /= land;
    likeness.dryland = double(arid) / land, likeness.inland = double(closed) / land;
    likeness.meanHeight = sum / land;
    std::sort(heights.begin(), heights.end());
    for (int q = 0; q < 3 && !heights.empty(); ++q) likeness.quartiles[q] = heights[size_t((q + 1) * 0.25 * double(heights.size() - 1))];
    likeness.straightShore = straightShoreShare(dry, true);
    return likeness;
}

void addRelief(const Mosaic& mosaic, ReliefLikeness& sums) {
    const int side = mosaic.side();
    const auto window = [&](int x0, int z0, int cells, double& land, double& low, double& high, double& mean) {
        land = mean = 0, low = 1e30, high = -1e30;
        for (int z = z0; z < z0 + cells; ++z)
            for (int x = x0; x < x0 + cells; ++x) {
                low = std::min(low, double(mosaic.top(x, z))), high = std::max(high, double(mosaic.top(x, z)));
                if (!mosaic.dry(x, z)) continue;
                land += 1, mean += mosaic.ground.at(x, z) - mosaic.seaLevel;
            }
        if (land > 0) mean /= land;
    };
    double land, low, high, mean;
    const int brief = cellsIn(mosaic, ShortWindow), wide = cellsIn(mosaic, LongWindow);
    for (int z0 = 0; z0 + brief <= side; z0 += brief)
        for (int x0 = 0; x0 + brief <= side; x0 += brief) {
            window(x0, z0, brief, land, low, high, mean);
            sums.land += size_t(land);
            if (high - low > 200) sums.reliefOver200 += land;
        }
    for (int z = 0; z < side; ++z)
        for (int x = 0; x < side; ++x) sums.lakes += mosaic.lake(x, z);
    for (int z0 = 0; z0 + wide <= side; z0 += wide)
        for (int x0 = 0; x0 + wide <= side; x0 += wide) {
            window(x0, z0, wide, land, low, high, mean);
            if (land < 0.5 * wide * wide) continue;
            ++sums.windows;
            const double rough = (high - low) / (0.5 * wide * mosaic.cell / 1000.0);
            size_t bin = 0;
            while (bin < 6 && rough >= Rough[bin]) ++bin;
            sums.roughness[bin] += 1;
            if (rough < 5) sums.plains += 1;
            else if (mean < 200) (rough < 20 ? sums.lowlands : sums.hills) += 1;
            else if (mean < 500) sums.hills += 1;
            else if (rough > (mean > 2000 ? 40 : 20)) sums.mountains += 1;
            else sums.plateaus += 1;
        }
}

void finish(ReliefLikeness& sums) {
    const double land = double(std::max<size_t>(1, sums.land)), windows = double(std::max<size_t>(1, sums.windows));
    sums.reliefOver200 /= land;
    sums.lakes /= land;
    for (double* share : {&sums.plains, &sums.lowlands, &sums.hills, &sums.plateaus, &sums.mountains}) *share /= windows;
    for (double& bin : sums.roughness) bin /= windows;
}

} // namespace derivean::sim

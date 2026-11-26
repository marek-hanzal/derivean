#include "measure/fn/History.hpp"

#include "measure/fn/Regularity.hpp"

#include <algorithm>

namespace derivean::sim {

PlanetSummary summarize(const Planet& planet) {
    PlanetSummary summary;
    summary.time = planet.time;
    summary.seaLevel = planet.seaLevel;
    summary.ledger = planet.ledger;
    summary.highest = -1e30, summary.deepest = 1e30;
    std::vector<float> land;
    Field<uint8_t> dry(planet.elevation.width, planet.elevation.height);
    double height = 0;
    size_t high = 0;
    for (size_t i = 0; i < planet.elevation.size(); ++i) {
        const CrustColumn& column = planet.columns.cells[i];
        const double above = planet.elevation.cells[i] - planet.seaLevel;
        summary.highest = std::max(summary.highest, above);
        summary.deepest = std::min(summary.deepest, above);
        summary.thickest = std::max(summary.thickest, double(column.crust));
        summary.crustVolume += (double(column.crust) + column.load) * PlanetCellArea;
        summary.coverVolume += double(coverThickness(planet.records.cells[i].cover)) * PlanetCellArea;
        if (above <= 0) continue;
        dry.cells[i] = 1;
        land.push_back(float(above));
        height += above;
        high += above > 2000;
    }
    const double cells = double(planet.elevation.size());
    summary.meanCrust = summary.crustVolume / (cells * PlanetCellArea);
    summary.landShare = double(land.size()) / cells;
    summary.straightShore = straightShoreShare(dry, true);
    if (land.empty()) return summary;
    summary.meanLand = height / double(land.size());
    summary.above2km = double(high) / double(land.size());
    std::sort(land.begin(), land.end());
    summary.high99 = land[size_t(0.99 * double(land.size() - 1))];
    return summary;
}

} // namespace derivean::sim

#pragma once

#include "measure/type/Scout.hpp"

namespace derivean::sim {

// Landscape types of docs/reference-landscapes.md whose search recipe can be read off a window of a
// mosaic: heights, slopes, water, ice and the rock at the surface. The recipes there assume a
// height field of 30 m or finer, so they are asked of Local mosaics, with slopes that a 64 m cell
// averages somewhat down. Criteria no field of a tile carries (rock uplift, potential
// evaporation, bed thickness at depth) are left out and named in the reason.
struct CatalogueType {
    const char* id;      // heading number in the catalogue, such as "A1"
    const char* name;
};

inline constexpr CatalogueType CatalogueTypes[] = {
    {"A1", "collisional high range"}, {"B1", "old worn mountains"},     {"B2", "granite domes"},      {"C4", "canyon lands and stepped plateaus"},
    {"C6", "badlands"},               {"F1", "glaciated shield and lake country"}, {"F2", "fjords"}, {"G2", "lowland river: floodplain"},
    {"H1", "cliffed coast"},          {"J2", "open basins and footslopes"}, {"J3", "rolling upland and hill country"},
};

inline constexpr int CatalogueCount = int(sizeof(CatalogueTypes) / sizeof(CatalogueTypes[0]));

// The best window of every type that the mosaic holds, 5 km on a side; a type the mosaic does not
// hold has no entry. `type` of an entry indexes CatalogueTypes.
struct CatalogueSite {
    int type{};
    ScoutSite site;
};

std::vector<CatalogueSite> findCatalogue(const Mosaic& mosaic);

} // namespace derivean::sim

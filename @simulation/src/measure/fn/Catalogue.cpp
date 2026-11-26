#include "measure/fn/Catalogue.hpp"

#include "material/constant/Materials.hpp"
#include "measure/constant/Angle.hpp"
#include "measure/fn/Format.hpp"

#include <cmath>

namespace derivean::sim {
namespace {

constexpr double WindowSpan = 5000.0;   // m, the window the catalogue states relief over
constexpr double Block = 1000.0;    // m, the window of "relief per km"

// What one window offers. Shares are of its dry cells unless noted.
struct Features {
    double elevation{};      // m above the sea, mean of the dry cells
    double relief{};         // m, highest minus lowest surface
    double reliefPerKm{};    // m, mean relief of its square kilometres
    double tilt{};           // degrees, slope of the window as a whole
    double slope{};          // degrees, mean
    double under1{}, under3{}, under5{}, from12to25{}, from35to50{}, over35{};   // slope classes
    double dry{}, lake{}, sea{};   // shares of all cells
    double crystalline{}, plutonic{}, layered{}, weak{}, carbonate{};   // rock at the surface
    double bare{};           // cover thinner than half a metre
    double ice{}, iceAge{};  // m, most ice now and at the glacial maximum
    double flow{};           // m³/yr, largest discharge on dry ground
    double deepest{}, highest{};   // m against the sea
    double cliffed{};        // share of shore cells with ground 20 m above the sea one cell inland
    int shore{};             // shore cells
};

bool among(Material rock, std::initializer_list<Material> set) {
    for (const Material other : set)
        if (other == rock) return true;
    return false;
}

Features read(const Mosaic& m, int x0, int z0, int cells) {
    Features f;
    const int block = cellsIn(m, Block);
    double low = 1e30, high = -1e30, blockSum = 0;
    int land = 0, lakes = 0, seas = 0, blocks = 0, cliffs = 0;
    for (int bz = 0; bz + block <= cells; bz += block) {
        for (int bx = 0; bx + block <= cells; bx += block) {
            double blockLow = 1e30, blockHigh = -1e30;
            for (int z = z0 + bz; z < z0 + bz + block; ++z)
                for (int x = x0 + bx; x < x0 + bx + block; ++x) blockLow = std::min(blockLow, double(m.top(x, z))), blockHigh = std::max(blockHigh, double(m.top(x, z)));
            blockSum += blockHigh - blockLow, ++blocks;
        }
    }
    for (int z = z0; z < z0 + cells; ++z) {
        for (int x = x0; x < x0 + cells; ++x) {
            low = std::min(low, double(m.top(x, z))), high = std::max(high, double(m.top(x, z)));
            f.deepest = std::min(f.deepest, m.ground.at(x, z) - m.seaLevel), f.highest = std::max(f.highest, m.ground.at(x, z) - m.seaLevel);
            f.ice = std::max(f.ice, double(m.ice.at(x, z))), f.iceAge = std::max(f.iceAge, double(m.iceAge.at(x, z)));
            if (m.lake(x, z)) ++lakes;
            else if (!m.dry(x, z)) ++seas;
            if (!m.dry(x, z)) continue;
            ++land;
            f.elevation += m.ground.at(x, z) - m.seaLevel;
            const double degrees = std::atan(std::hypot(m.top(x + 1, z) - m.top(x - 1, z), m.top(x, z + 1) - m.top(x, z - 1)) / (2 * m.cell)) * DegreesPerRadian;
            f.slope += degrees;
            f.under1 += degrees < 1, f.under3 += degrees < 3, f.under5 += degrees < 5, f.over35 += degrees > 35;
            f.from12to25 += degrees >= 12 && degrees <= 25, f.from35to50 += degrees >= 35 && degrees <= 50;
            const Material rock = Material(m.rock.at(x, z));
            f.plutonic += rock == Material::Granite;
            f.crystalline += among(rock, {Material::Granite, Material::Gneiss, Material::Schist, Material::Quartzite, Material::Marble});
            f.layered += among(rock, {Material::Sandstone, Material::Conglomerate, Material::Mudstone, Material::Shale, Material::Limestone, Material::Chalk});
            f.weak += among(rock, {Material::Shale, Material::Mudstone, Material::Clay});
            f.carbonate += among(rock, {Material::Limestone, Material::Chalk, Material::Marble});
            f.bare += m.cover.at(x, z) < 0.5f;
            f.flow = std::max(f.flow, double(m.discharge.at(x, z)));
            const auto open = [&](int ox, int oz) { return !m.dry(ox, oz) && !m.lake(ox, oz); };
            for (int k = 0; k < 4; ++k) {
                const int dx = (k == 0) - (k == 1), dz = (k == 2) - (k == 3);
                if (!open(x + dx, z + dz)) continue;
                ++f.shore;
                cliffs += m.top(x - dx, z - dz) >= m.seaLevel + 20;
                break;
            }
        }
    }
    const double all = double(cells) * cells, dry = std::max(1, land);
    f.dry = land / all, f.lake = lakes / all, f.sea = seas / all;
    for (double* share : {&f.elevation, &f.slope, &f.under1, &f.under3, &f.under5, &f.from12to25, &f.from35to50, &f.over35, &f.crystalline, &f.plutonic, &f.layered,
                          &f.weak, &f.carbonate, &f.bare})
        *share /= dry;
    f.relief = high - low;
    f.reliefPerKm = blocks ? blockSum / blocks : 0.0;
    const int far = cells - 1;
    const double east = 0.5 * (m.top(x0 + far, z0) + m.top(x0 + far, z0 + far)) - 0.5 * (m.top(x0, z0) + m.top(x0, z0 + far));
    const double south = 0.5 * (m.top(x0, z0 + far) + m.top(x0 + far, z0 + far)) - 0.5 * (m.top(x0, z0) + m.top(x0 + far, z0));
    f.tilt = std::atan(std::hypot(east, south) / (far * m.cell)) * DegreesPerRadian;
    f.cliffed = f.shore ? double(cliffs) / f.shore : 0.0;
    return f;
}

// How well a window fits a type: zero or less when it does not, else larger for a clearer case.
double fit(int type, const Features& f) {
    const bool land = f.dry > 0.6;
    switch (type) {
        case 0: return land && f.elevation > 2000 && f.relief > 1200 && f.over35 >= 0.3 && f.under5 < 0.2 && (f.ice > 0 || f.iceAge > 0) && f.crystalline + f.carbonate >= 0.5 ? f.relief : 0;
        case 1: return land && f.relief >= 250 && f.relief <= 700 && f.slope >= 8 && f.slope <= 18 && f.over35 < 0.03 && f.crystalline >= 0.5 ? 1 + f.crystalline : 0;
        case 2: return land && f.plutonic >= 0.6 && f.bare >= 0.7 && f.slope >= 30 && f.relief >= 100 ? f.bare + f.plutonic : 0;
        case 3: return land && f.under3 >= 0.4 && f.relief >= 300 && f.layered >= 0.6 ? f.relief * f.under3 : 0;
        case 4: return land && f.weak >= 0.5 && f.reliefPerKm >= 20 && f.reliefPerKm <= 150 && f.from35to50 >= 0.15 && f.under5 >= 0.15 ? f.from35to50 + f.under5 : 0;
        case 5: return f.dry > 0.5 && f.crystalline >= 0.6 && f.iceAge > 0 && f.tilt < 0.5 && f.reliefPerKm >= 10 && f.reliefPerKm <= 80 && f.lake > 0.08 ? f.lake : 0;
        case 6: return f.sea >= 0.1 && f.sea <= 0.6 && f.deepest <= -200 && f.highest >= 500 ? f.highest - f.deepest : 0;
        case 7: return land && f.flow >= 1e8 && f.under1 >= 0.4 && f.relief < 100 ? f.under1 : 0;
        case 8: return f.shore >= 10 && f.cliffed >= 0.5 ? f.cliffed * f.shore : 0;
        case 9: return land && f.relief >= 100 && f.relief <= 600 && f.under5 >= 0.6 && f.from12to25 >= 0.1 && f.crystalline >= 0.5 ? f.under5 + f.from12to25 : 0;
        case 10: return land && f.elevation >= 200 && f.elevation <= 800 && f.relief >= 100 && f.relief <= 300 && f.slope >= 3 && f.slope <= 8 ? 1 + f.dry : 0;
    }
    return 0;
}

} // namespace

std::vector<CatalogueSite> findCatalogue(const Mosaic& mosaic) {
    std::vector<CatalogueSite> best;
    double score[CatalogueCount]{};
    const int cells = cellsIn(mosaic, WindowSpan), stride = std::max(1, cells / 2);
    if (cells < 16) return best;   // a window of a few cells has no slopes to speak of
    for (int z0 = 1; z0 + cells < mosaic.side() - 1; z0 += stride) {
        for (int x0 = 1; x0 + cells < mosaic.side() - 1; x0 += stride) {
            const Features f = read(mosaic, x0, z0, cells);
            for (int type = 0; type < CatalogueCount; ++type) {
                const double fits = fit(type, f);
                if (fits <= score[type]) continue;
                if (score[type] <= 0) best.push_back({type, {}});
                score[type] = fits;
                ScoutSite site;
                site.at = cellCentre(mosaic, x0 + cells / 2, z0 + cells / 2);
                site.ground = mosaic.top(x0 + cells / 2, z0 + cells / 2);
                site.level = mosaic.level;
                site.score = fits;
                site.reason = formatText("5 km window: mean height %.0f m, relief %.0f m (%.0f m per km), mean slope %.0f deg, %.0f %% over 35 deg, %.0f %% under 5 deg, "
                                         "lakes %.0f %%, sea %.0f %%, crystalline %.0f %%, layered %.0f %%, weak %.0f %%, bare %.0f %%, ice %.0f m now and %.0f m at the glacial maximum",
                                         f.elevation, f.relief, f.reliefPerKm, f.slope, 100 * f.over35, 100 * f.under5, 100 * f.lake, 100 * f.sea, 100 * f.crystalline,
                                         100 * f.layered, 100 * f.weak, 100 * f.bare, f.ice, f.iceAge);
                site.stand = site.at + Vec2{0.0, 0.4 * WindowSpan};
                site.lookUp = float(0.3 * f.relief);
                for (CatalogueSite& entry : best)
                    if (entry.type == type) entry.site = site;
            }
        }
    }
    return best;
}

} // namespace derivean::sim

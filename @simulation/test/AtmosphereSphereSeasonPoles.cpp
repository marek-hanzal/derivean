// The season's polar caps (SphereSeason, Codex DREV-CLAUDE-010-01): the belts' wind tapers to zero
// at both poles, so the polar bands' jet converges as the grid refines instead of growing as 1/θ.
#include "AtmosphereSphereFixture.hpp"

#include "atmosphere/constant/SphereAir.hpp"
#include "atmosphere/fn/LatitudeBands.hpp"
#include "atmosphere/fn/SphereClosures.hpp"
#include "atmosphere/fn/SphereSeason.hpp"
#include "chronology/type/Calendar.hpp"
#include "climate/type/Climate.hpp"
#include "geometry/fn/SphereGrid.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

using namespace derivean::sim;
using namespace derivean::sim::atmosphere_sphere_test;

namespace {

constexpr double Degrees = 180.0 / std::numbers::pi;

// With the temperature at its base each half's strength is exactly one, so the belts' wind depends on
// beltWind, the shift and the taper only; levels 5 to 8 (polar bands 5.8° to 0.73° wide), day 100.
// Both poles' wind is zero; equatorward of PolarCap the belts blow JetDrive · beltWind bit for bit and
// poleward less; the jet has no area integral (balancedJet). Each polar band's jet shrinks as the
// bands narrow, where the untapered wind's grew 3.0 times from level 5 to 8 (|u| / R tan(θ/2) with
// |u| → 1.18 m/s), and the north's matches Codex's comparison probe, 4.0935e-6 at level 6 and
// 2.5693e-6 at level 8 (to 1e-4), a first-order approach to the pole's own value (the differences
// nearly halve a level). Across PolarCap the jet has no step: at level 8 its second difference
// between bands from 65° to 85° stays under 10 % of its largest value there (measured 4.0 %, the
// untapered wind's 1.2 %), where a share only continuous, x, steps the jet and gives 28 %. A step
// keeps its size as the bands narrow while the taper's kink shrinks with them, so the bound is a
// level-8 one (level 7 reads 10.2 %). The taper's cosine is the literal for cos PolarCap.
void checkPolarCaps() {
    expect(std::abs(std::cos(SpherePolarCap / Degrees) - SpherePolarCapCosine) <= 1e-16, "the cap's cosine is not cos PolarCap");
    double before[2]{};
    for (int level = 5; level <= 8; ++level) {
        const SphereGrid grid = makeSphereGrid(level);
        const LatitudeBands bands = makeLatitudeBands(grid, grid.side);
        std::vector<double> base(size_t(bands.count));
        for (int b = 0; b < bands.count; ++b) base[size_t(b)] = 30 - 0.5 * std::abs(bands.centre[size_t(b)] * Degrees);
        const double day = 100, shift = SphereBeltShift * declinationOn(day - SphereBeltLag);
        const SphereSeason season = sphereSeasonOf(bands, day, base, base);
        const size_t count = size_t(bands.count);
        expect(season.eastward.front() == 0 && season.eastward.back() == 0, "a pole's zonal wind is not zero", double(level));
        int kept = 0, tapered = 0;
        for (size_t k = 1; k < count; ++k) {
            const double latitude = std::asin(bands.boundary[k]) * Degrees;
            const double belts = SphereJetDrive * beltWind(latitude - shift * std::cos(latitude / Degrees)).east;
            if (polarShareOf(bands.boundary[k]) == 1) {
                expect(std::abs(latitude) < SpherePolarCap + 1e-9, "the share is one poleward of PolarCap", latitude);
                expect(season.eastward[k] == belts, "the belts' wind moved equatorward of PolarCap", latitude);
                ++kept;
            } else {
                expect(std::abs(latitude) > SpherePolarCap - 1e-9, "the share is below one equatorward of PolarCap", latitude);
                expect(std::abs(season.eastward[k]) < std::abs(belts), "the belts' wind is not tapered poleward of PolarCap", latitude);
                ++tapered;
            }
        }
        expect(kept > 0 && tapered >= 2, "the caps' test reached too few boundaries", double(tapered));
        SphereClosures closures;
        closures.bands = bands;
        closures.jet = season.jet;
        expect(balancedJet(closures), "the season's jet has an area integral", double(level));
        const double polar[2] = {std::abs(season.jet.back()), std::abs(season.jet.front())};
        for (int pole = 0; pole < 2; ++pole) {
            expect(polar[pole] > 0, "a polar band has no jet", double(level));
            if (level > 5) expect(polar[pole] < before[pole], "a polar band's jet grows as the grid refines", polar[pole] / before[pole]);
            before[pole] = polar[pole];
        }
        if (level == 6) expect(std::abs(polar[0] / 4.0935e-6 - 1) < 1e-4, "the north's polar jet moved from Codex's probe at level 6", polar[0]);
        if (level == 8) {
            expect(std::abs(polar[0] / 2.5693e-6 - 1) < 1e-4, "the north's polar jet moved from Codex's probe at level 8", polar[0]);
            double top = 0, step = 0;
            for (size_t b = 1; b + 1 < count; ++b) {
                const double latitude = std::abs(bands.centre[b] * Degrees);
                if (latitude < 65 || latitude > 85) continue;
                top = std::max(top, std::abs(season.jet[b]));
                step = std::max(step, std::abs(season.jet[b + 1] - 2 * season.jet[b] + season.jet[b - 1]));
            }
            expect(top > 0 && step < 0.1 * top, "the jet steps across PolarCap", step / top);
        }
    }
}

} // namespace

void derivean::sim::atmosphere_sphere_test::checkSeasonPoles() { checkPolarCaps(); }

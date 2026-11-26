#include "LandscapeFixture.hpp"

#include "inspect/fn/Inspect.hpp"
#include "inspect/fn/Provenance.hpp"
#include "simulation/fn/Query.hpp"

using namespace derivean::sim;

namespace {

SurfaceEvidence evidence(Material surface, Material bed, float coarse, float sand, float fines) {
    SurfaceEvidence e;
    e.surface = surface, e.bed = bed;
    e.cover[Coarse] = coarse, e.cover[Sand] = sand, e.cover[Fines] = fines;
    e.slope = 0.05f, e.steepestAbove = 0.06f, e.aboveSea = 400;
    return e;
}

void expectOrigin(const SurfaceEvidence& e, SurfaceOrigin wanted, const char* message) {
    const Provenance found = explainSurface(e);
    expectLandscape(found.origin == wanted && !found.reason.empty(), message, double(found.origin));
}

// "Why is there sand exactly here?" has a different answer for each of these.
void checkProvenance() {
    expectOrigin(evidence(Material::Sand, Material::Sandstone, 0.1f, 0.7f, 0.0f), SurfaceOrigin::WeatheredInPlace, "a thin sandy mantle on sandstone is not weathered in place");

    SurfaceEvidence river = evidence(Material::Sand, Material::Granite, 0.5f, 3.0f, 0.2f);
    river.deposited = 2.5f, river.depositionRate = 1e-4f, river.discharge = 5e6f, river.slope = 0.004f, river.steepestAbove = 0.2f;
    expectOrigin(river, SurfaceOrigin::RiverDeposit, "sand laid down by a river is not a river deposit");

    SurfaceEvidence lake = evidence(Material::Sand, Material::Granite, 0.0f, 4.0f, 0.0f);
    lake.waterDepth = 3, lake.lake = true, lake.deposited = 4;
    expectOrigin(lake, SurfaceOrigin::LakeFloor, "sand under a lake is not a lake floor");
    lake.lake = false, lake.aboveSea = -3;
    expectOrigin(lake, SurfaceOrigin::SeaFloor, "sand under the sea is not a sea floor");

    SurfaceEvidence beach = evidence(Material::Sand, Material::Granite, 0.0f, 2.0f, 0.0f);
    beach.aboveSea = 0.8f;
    expectOrigin(beach, SurfaceOrigin::Shore, "sand within reach of the tide is not a shore");

    SurfaceEvidence talus = evidence(Material::Gravel, Material::Granite, 3.0f, 0.5f, 0.0f);
    talus.slope = 0.6f, talus.steepestAbove = 1.4f;
    expectOrigin(talus, SurfaceOrigin::SlopeDebris, "coarse debris under a rock face is not slope debris");

    SurfaceEvidence stray = evidence(Material::Sand, Material::Granite, 0.0f, 6.0f, 0.0f);
    stray.slope = 0.0f, stray.steepestAbove = 0.0f;
    expectOrigin(stray, SurfaceOrigin::Unexplained, "six metres of sand on flat granite with no record is called explained");

    expectOrigin(evidence(Material::Sandstone, Material::Sandstone, 0, 0, 0), SurfaceOrigin::ExposedRock, "bare rock is not exposed rock");
}

} // namespace

void testInspect() {
    checkProvenance();

    World& world = testWorld();
    for (const Vec2 offset : {Vec2{0, 0}, Vec2{3.3, -2.9}}) {
        const Vec2 at = testSpot() + offset;
        const InspectRecord record = inspectAt(world, at);
        const SurfaceSample survey = surveyAt(world, at, Level::Ground);
        expectLandscape(record.found, "the inspector found no surface where surveyAt has one");
        expectLandscape(record.surface.ground == survey.ground && record.surface.datum == survey.datum, "the inspector and surveyAt disagree on the ground", record.surface.ground - survey.ground);
        expectLandscape(std::abs(record.cover.thickness - (survey.ground - survey.rock)) < 1e-3f, "the inspector and surveyAt disagree on the cover", record.cover.thickness);
        expectLandscape(record.rock.modelBed == survey.bedrock, "the inspector and surveyAt disagree on the bed");
        expectLandscape(!record.rock.column.empty() && record.rock.column.back().bottom >= InspectDepth - 1, "the listed column stops short", record.rock.column.empty() ? 0 : record.rock.column.back().bottom);
        // Every listed run is what materialAt returns half a sample into it.
        for (const InspectBed& bed : record.rock.column)
            expectLandscape(materialAt(world, at.x, double(survey.ground) - bed.top - 0.25, at.z) == bed.material, "the listed column and materialAt disagree", bed.top);
        expectLandscape(record.crust.thickness > 0 && record.rates.window > 0 && record.regional.window > record.rates.window, "the record misses its planet or rate part");
        expectLandscape(record.heat.gradient > 0, "the record misses the heat from below", record.heat.gradient);
        expectLandscape(describe(record).size() > 40, "the flat rendering lost entries", double(describe(record).size()));
    }
}

// Each domain defines its checks in its own translation unit. A failed check reports to stderr and
// exits non-zero, so reaching the end of main means every check passed. Arguments pick the tests
// whose name contains any of them, ignoring case (`simulation-tests intervention coast`); none runs all.
#include <cctype>
#include <string>

void testMaterials();
void testAeolian();
void testClimate();
void testClouds();
void testAtmosphere();
void testSnow();
void testPlume();
void testIceSheet();
void testIceSheetLocal();
void testIceSheetSeam();
void testLandscapeRegularity();
void testLandscapePlane();
void testLandscapeWorld();
void testRelief();
void testReliefBeds();
void testReliefCover();
void testReliefGlaciatedSite();
void testInspect();
void testChart();
void testPlanetFuture();
void testHydrology();
void testGroundForms();
void testGroundTile();
void testIntervention();
void testOre();
void testRockMass();
void testGroundwaterBlock();
void testGroundwaterEdge();
void testGroundwaterTile();
void testGroundwaterSinking();
void testGroundwaterPavement();
void testGroundwaterRegion();
void testVolcanicLaws();
void testVolcanicLive();
void testCoast();
void testLevelConsistency();

namespace {

struct Test {
    const char* name;
    void (*run)();
};

// In the order they run.
constexpr Test Tests[] = {
    {"testMaterials", testMaterials},
    {"testAeolian", testAeolian},
    {"testClimate", testClimate},
    {"testClouds", testClouds},
    {"testAtmosphere", testAtmosphere},
    {"testSnow", testSnow},
    {"testPlume", testPlume},
    {"testIceSheet", testIceSheet},
    {"testIceSheetLocal", testIceSheetLocal},
    {"testIceSheetSeam", testIceSheetSeam},
    {"testLandscapeRegularity", testLandscapeRegularity},
    {"testLandscapePlane", testLandscapePlane},
    {"testRelief", testRelief},
    {"testReliefBeds", testReliefBeds},
    {"testLandscapeWorld", testLandscapeWorld},
    {"testReliefCover", testReliefCover},
    {"testReliefGlaciatedSite", testReliefGlaciatedSite},
    {"testInspect", testInspect},
    {"testChart", testChart},
    {"testPlanetFuture", testPlanetFuture},
    {"testHydrology", testHydrology},
    {"testGroundForms", testGroundForms},
    {"testGroundTile", testGroundTile},
    {"testIntervention", testIntervention},
    {"testOre", testOre},
    {"testRockMass", testRockMass},
    {"testGroundwaterBlock", testGroundwaterBlock},
    {"testGroundwaterEdge", testGroundwaterEdge},
    {"testGroundwaterTile", testGroundwaterTile},
    {"testGroundwaterSinking", testGroundwaterSinking},
    {"testGroundwaterPavement", testGroundwaterPavement},
    {"testGroundwaterRegion", testGroundwaterRegion},
    {"testVolcanicLaws", testVolcanicLaws},
    {"testVolcanicLive", testVolcanicLive},
    {"testCoast", testCoast},
    {"testLevelConsistency", testLevelConsistency},
};

std::string lower(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

} // namespace

int main(int argc, char** argv) {
    for (const Test& test : Tests) {
        bool chosen = argc < 2;
        for (int a = 1; a < argc && !chosen; ++a) chosen = lower(test.name).find(lower(argv[a])) != std::string::npos;
        if (chosen) test.run();
    }
    return 0;
}

// Each domain defines its checks in its own translation unit. A failed check reports to stderr and
// exits non-zero, so reaching the end of main means every check passed. Arguments pick the tests
// whose name contains any of them, ignoring case (`simulation-tests intervention coast`); none runs
// all. `-word` leaves out the tests whose name contains it; `--group name` keeps one group's tests,
// and `--groups` lists the groups.
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

void testBuildGraph();
void testMaterials();
void testGeometry();
void testAeolian();
void testClimate();
void testAstronomy();
void testAtmosphereSphere();
void testGeometrySphere();
void testGlobeKinematics();
void testGlobeCoupling();
void testGlobeUnion();
void testGlobePlan();
void testGlobeBoundary();
void testGlobeBalance();
void testGlobeDeterminism();
void testGlobeBridge();
void testGlobeEpochs();
void testClouds();
void testAtmosphere();
void testSnow();
void testPlume();
void testLandscapePlume();
void testIceSheet();
void testIceSheetLocal();
void testIceSheetSeam();
void testIceSheetHold();
void testLandscapeRegularity();
void testLandscapePlane();
void testLandscapeWorld();
void testRelief();
void testReliefBeds();
void testReliefCover();
void testReliefGlaciatedSite();
void testReliefLedger();
void testInspect();
void testChart();
void testPlanetFuture();
void testHydrology();
void testGroundForms();
void testGroundTile();
void testIntervention();
void testInterventionQueries();
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
void testHaze();
void testLevelConsistency();
void startFarFuture();   // LandscapeFixture.cpp

namespace {

struct Test {
    const char* name;
    void (*run)();
    const char* group;
};

// Groups split the suite by the state its tests share, so that `mise run test` runs one process per
// group side by side (`--group`) and no group builds what another builds too:
//   world    the shared young world (testWorld, seed 42 at 1.2 Gyr) and its Ground tiles at the focus
//   present  the present world (testWorldPresent, 4.6 Gyr)
//   intervention  the intervention test's own world and its one-thread twin
//   weather  the flat atmosphere stepped for simulated years, and the short weather checks
//   unit     everything that builds its own fixture (the plume's atmosphere too, to even the two
//            out), the far-future planet, and testCoast, which reads only the fixture's planet and
//            would otherwise add a minute on one thread to the longest group
// In the order they run.
constexpr Test Tests[] = {
    {"testBuildGraph", testBuildGraph, "unit"},
    {"testMaterials", testMaterials, "unit"},
    {"testGeometry", testGeometry, "unit"},
    {"testAeolian", testAeolian, "unit"},
    {"testClimate", testClimate, "weather"},
    {"testAstronomy", testAstronomy, "unit"},
    {"testAtmosphereSphere", testAtmosphereSphere, "unit"},
    {"testGeometrySphere", testGeometrySphere, "unit"},
    {"testGlobeKinematics", testGlobeKinematics, "unit"},
    {"testGlobeCoupling", testGlobeCoupling, "unit"},
    {"testGlobeUnion", testGlobeUnion, "unit"},
    {"testGlobePlan", testGlobePlan, "unit"},
    {"testGlobeBoundary", testGlobeBoundary, "unit"},
    {"testGlobeBalance", testGlobeBalance, "unit"},
    {"testGlobeDeterminism", testGlobeDeterminism, "unit"},
    {"testGlobeBridge", testGlobeBridge, "unit"},
    {"testGlobeEpochs", testGlobeEpochs, "unit"},
    {"testClouds", testClouds, "weather"},
    {"testAtmosphere", testAtmosphere, "weather"},
    {"testSnow", testSnow, "weather"},
    {"testPlume", testPlume, "unit"},
    {"testLandscapePlume", testLandscapePlume, "unit"},
    {"testIceSheet", testIceSheet, "unit"},
    {"testIceSheetLocal", testIceSheetLocal, "unit"},
    {"testIceSheetSeam", testIceSheetSeam, "unit"},
    {"testIceSheetHold", testIceSheetHold, "unit"},
    {"testLandscapeRegularity", testLandscapeRegularity, "unit"},
    {"testLandscapePlane", testLandscapePlane, "unit"},
    {"testRelief", testRelief, "unit"},
    {"testReliefBeds", testReliefBeds, "unit"},
    {"testLandscapeWorld", testLandscapeWorld, "world"},
    {"testReliefCover", testReliefCover, "world"},
    {"testReliefGlaciatedSite", testReliefGlaciatedSite, "present"},
    {"testReliefLedger", testReliefLedger, "present"},
    {"testInspect", testInspect, "world"},
    {"testChart", testChart, "world"},
    {"testPlanetFuture", testPlanetFuture, "unit"},
    {"testHydrology", testHydrology, "unit"},
    {"testGroundForms", testGroundForms, "unit"},
    {"testGroundTile", testGroundTile, "world"},
    {"testIntervention", testIntervention, "intervention"},
    {"testInterventionQueries", testInterventionQueries, "unit"},
    {"testOre", testOre, "unit"},
    {"testRockMass", testRockMass, "unit"},
    {"testGroundwaterBlock", testGroundwaterBlock, "unit"},
    {"testGroundwaterEdge", testGroundwaterEdge, "unit"},
    {"testGroundwaterTile", testGroundwaterTile, "world"},
    {"testGroundwaterSinking", testGroundwaterSinking, "unit"},
    {"testGroundwaterPavement", testGroundwaterPavement, "unit"},
    {"testGroundwaterRegion", testGroundwaterRegion, "world"},
    {"testVolcanicLaws", testVolcanicLaws, "unit"},
    {"testVolcanicLive", testVolcanicLive, "unit"},
    {"testCoast", testCoast, "unit"},
    {"testHaze", testHaze, "weather"},
    {"testLevelConsistency", testLevelConsistency, "world"},
};

std::string lower(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> words, skipped;
    const char* group = nullptr;
    for (int a = 1; a < argc; ++a) {
        if (std::strcmp(argv[a], "--groups") == 0) {
            for (const Test& test : Tests)
                if (&test == &Tests[0] || std::none_of(Tests, &test, [&](const Test& before) { return std::strcmp(before.group, test.group) == 0; }))
                    std::cout << test.group << '\n';
            return 0;
        }
        if (std::strcmp(argv[a], "--group") == 0 && a + 1 < argc) group = argv[++a];
        else if (std::strncmp(argv[a], "--", 2) == 0) {
            std::cerr << "unknown option " << argv[a] << '\n';
            return 2;
        } else if (argv[a][0] == '-' && argv[a][1]) skipped.push_back(lower(argv[a] + 1));
        else words.push_back(lower(argv[a]));
    }
    std::vector<const Test*> chosen;
    for (const Test& test : Tests) {
        const std::string name = lower(test.name);
        bool wanted = words.empty();
        for (const std::string& word : words) wanted = wanted || name.find(word) != std::string::npos;
        for (const std::string& word : skipped) wanted = wanted && name.find(word) == std::string::npos;
        if (group && std::strcmp(group, test.group) != 0) wanted = false;
        if (wanted) chosen.push_back(&test);
    }
    if (group && std::none_of(std::begin(Tests), std::end(Tests), [&](const Test& test) { return std::strcmp(group, test.group) == 0; })) {
        std::cerr << "no test in group " << group << '\n';
        return 2;
    }
    // The far-future planet runs alone for as long as most of a group; start it with the first test.
    for (const Test* test : chosen)
        if (test->run == testPlanetFuture) startFarFuture();
    for (const Test* test : chosen) test->run();
    return 0;
}

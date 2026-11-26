// Each domain defines its checks in its own translation unit. A failed check reports to stderr and
// exits non-zero, so reaching the end of main means every check passed.
void testMaterials();
void testClimate();
void testClouds();
void testAtmosphere();
void testSnow();
void testPlume();
void testLandscapeRegularity();
void testLandscapePlane();
void testLandscapeWorld();
void testRelief();
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
void testGroundwaterTile();

int main() {
    testMaterials();
    testClimate();
    testClouds();
    testAtmosphere();
    testSnow();
    testPlume();
    testLandscapeRegularity();
    testLandscapePlane();
    testRelief();
    testLandscapeWorld();
    testInspect();
    testChart();
    testPlanetFuture();
    testHydrology();
    testGroundForms();
    testGroundTile();
    testIntervention();
    testOre();
    testRockMass();
    testGroundwaterBlock();
    testGroundwaterTile();
    return 0;
}

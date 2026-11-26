// Each group of checks lives in its own translation unit.
void testWorld();
void testScenarios();
void testStreamedWorld();
void testWeather();
void testPlanet();

int main() {
    testWorld();
    testScenarios();
    testStreamedWorld();
    testWeather();
    testPlanet();
    return 0;
}

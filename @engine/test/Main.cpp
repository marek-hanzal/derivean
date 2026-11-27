#include <iostream>
#include <string_view>

// Each group of checks lives in its own translation unit.
void testWorld();
void testScenarios();
void testStreamedWorld();
void testWeather();
void testPlanet();
void testRequests();
void testMesh();
void testTables();

// `engine-tests mesh` runs only the tables' and the mesher's checks, which take seconds; without an argument
// everything runs.
int main(int argc, char** argv) {
    std::cout << std::unitbuf;   // progress shows in a log while the planet checks run for minutes
    testTables();
    testMesh();
    if (argc > 1 && std::string_view(argv[1]) == "mesh") return 0;
    testRequests();
    testWorld();
    testScenarios();
    testStreamedWorld();
    testWeather();
    testPlanet();
    return 0;
}

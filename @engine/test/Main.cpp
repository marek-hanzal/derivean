#include <iostream>

// Each group of checks lives in its own translation unit.
void testWorld();
void testScenarios();
void testStreamedWorld();
void testWeather();
void testPlanet();

int main() {
    std::cout << std::unitbuf;   // progress shows in a log while the planet checks run for minutes
    testWorld();
    testScenarios();
    testStreamedWorld();
    testWeather();
    testPlanet();
    return 0;
}

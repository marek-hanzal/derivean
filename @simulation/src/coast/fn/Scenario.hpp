#pragma once

#include "landscape/fn/Build.hpp"

namespace derivean::sim {

// Hand-built shores on which every process of the coast can be watched alone, as
// measure/fn/Plane.hpp does for the rivers. Sea lies to the north (low Z) of land that rises to a
// level upland; a lake lies in a hollow of such an upland.
enum class CoastScene : uint8_t {
    Straight,   // a straight shore; the eastern half of the land is of `other` rock, and joint zones cross both
    Bay,        // the same shore with a bay cut into it; loose cover lies west of the bay only
    Lake,       // a lake that fills a hollow, loose cover all round its shore
};

struct CoastSpec {
    CoastScene scene{CoastScene::Straight};
    Level level{Level::Local};
    Material rock{Material::Sandstone};
    Material other{Material::Sandstone};
    double windFrom{0};    // degrees, compass bearing the prevailing wind comes from
    double wind{7};        // m/s, its annual mean
    double swell{3};       // m, height of the sea that arrives across the open rim with that wind
    double sand{0};        // m of sand on the shore and on the bed before it
    double gravel{0};      // m of gravel there
    double mud{0};         // m of fines there
    double joints{0};      // 0..1 crushed share of the rock inside the joint zones; 0 for none
    double river{0};       // m³/yr of a river that reaches the middle of the shore from inland
    double upland{60};     // m the land rises to
    double shelf{15};      // m of water at which the sea floor levels off
    int steps{0};          // 0 runs the settling time of the level
    uint64_t seed{42};
};

Domain coastDomain(const CoastSpec& spec);

// Waves alone: each step floods and routes the domain and lets the coast work, nothing else moves.
void workCoast(Domain& domain, int steps);

// The whole step loop of the level, rivers and slopes included.
Domain replayCoast(const CoastSpec& spec, Workers* workers = nullptr);

} // namespace derivean::sim

#include "Arguments.hpp"

#include "inspect/fn/Inspect.hpp"
#include "measure/fn/Place.hpp"
#include "simulation/fn/Spawn.hpp"

#include <cstdio>

using namespace derivean::sim;

// terra inspect --x metres --z metres [--at shore|coast|landing]: everything the inspector knows about one point.
int inspectCommand(const Arguments& arguments) {
    World world({arguments.seed(), arguments.number("age", 4.6e9), unsigned(arguments.number("threads", 0))});
    Vec2 at{arguments.number("x", 0), arguments.number("z", 0)};
    const std::string where = arguments.text("at", "");
    if (where == "shore" || where == "coast") at = findShore(world.planet, at);
    if (where == "coast") at = coastNear(world, at);
    if (where == "landing") at = findLanding(world, at);
    const InspectRecord record = inspectAt(world, at);
    for (const auto& [label, value] : describe(record)) std::printf("%-52s %s\n", label.c_str(), value.c_str());
    return record.found ? 0 : 1;
}

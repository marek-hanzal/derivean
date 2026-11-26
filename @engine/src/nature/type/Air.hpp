#pragma once

// The simulation names a `Sand` twice in one namespace: a sediment class
// (landscape/type/Terrain.hpp) and an ash class (atmosphere/type/Plume.hpp). A file that sees both
// does not compile, and every engine file that knows the game sees both. Until the simulation
// scopes one of them, the engine includes the atmosphere only through this header: after the
// world, and with the ash class under another name.
#include "simulation/type/World.hpp"

#define Sand SandAsh
#include "atmosphere/type/Atmosphere.hpp"
#undef Sand

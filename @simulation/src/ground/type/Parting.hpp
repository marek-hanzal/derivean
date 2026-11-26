#pragma once

#include "material/constant/Materials.hpp"

namespace derivean::sim {

// A bed is not one slab. Bedding planes divide it into layers of its own rock that differ in how
// well they hold together, some with a seam of weak rock at their base. They are seeded geometry
// attached to the crust, as the beds themselves are, and finer than the planes a rock mass parts
// along; the joints that cut the bed are the rock mass's own. What the ground makes of both
// (ledges, steps, knobs, the blocks a face sheds) is simulated.
struct Parting {
    float top{}, bottom{};              // m below the datum
    float strength{1};                  // factor on the bed's resistance: above one stands out, below one is stripped
    Material rock{Material::Bedrock};   // the bed's rock, or the weak rock of a seam
    bool seam{};
};

// What a face breaks into: the median intermediate axis of the pieces in metres, and how flat they
// are (thickness over width). A mass releases blocks as large as its joints and bedding planes cut
// them; few survive the fall whole, and the finer layering of bedded rock splits them into slabs.
struct BlockHabit {
    float size{};
    float flatness{1};
};

} // namespace derivean::sim

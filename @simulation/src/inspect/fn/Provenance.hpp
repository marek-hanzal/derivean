#pragma once

#include "inspect/type/Record.hpp"

namespace derivean::sim {

// The measured facts a surface material is explained from. Everything here is read from the
// finished tiles; the explanation never looks at where the point is.
struct SurfaceEvidence {
    Material surface{Material::Air};   // material at the top of the ground
    Material bed{Material::Bedrock};   // solid rock under the cover
    float cover[SedimentClasses]{};    // m of coarse, sand and fines
    float slope{};          // rise over run at the point
    float steepestAbove{};  // rise over run of the steepest climb to higher ground within two cells
    float discharge{};      // m³/yr
    float depositionRate{}, deposited{};   // m/yr now, m over the replay
    float erosionRate{}, eroded{};
    float waterDepth{};     // m of standing water over the ground; 0 or less when dry
    float aboveSea{};       // m, ground minus sea level
    bool lake{};            // the standing water is not the sea
};

Provenance explainSurface(const SurfaceEvidence& evidence);

} // namespace derivean::sim

#pragma once

#include "material/definition/Air.hpp"
#include "material/definition/Andesite.hpp"
#include "material/definition/Basalt.hpp"
#include "material/definition/Bedrock.hpp"
#include "material/definition/Chalk.hpp"
#include "material/definition/Clay.hpp"
#include "material/definition/Coal.hpp"
#include "material/definition/Conglomerate.hpp"
#include "material/definition/Gneiss.hpp"
#include "material/definition/Granite.hpp"
#include "material/definition/Gravel.hpp"
#include "material/definition/Ice.hpp"
#include "material/definition/IronFormation.hpp"
#include "material/definition/Kimberlite.hpp"
#include "material/definition/Leaf.hpp"
#include "material/definition/Limestone.hpp"
#include "material/definition/Magma.hpp"
#include "material/definition/Marble.hpp"
#include "material/definition/Moss.hpp"
#include "material/definition/Mudstone.hpp"
#include "material/definition/OreVein.hpp"
#include "material/definition/Peridotite.hpp"
#include "material/definition/Quartzite.hpp"
#include "material/definition/Regolith.hpp"
#include "material/definition/RockSalt.hpp"
#include "material/definition/Sand.hpp"
#include "material/definition/Sandstone.hpp"
#include "material/definition/Schist.hpp"
#include "material/definition/Scree.hpp"
#include "material/definition/Shale.hpp"
#include "material/definition/Silt.hpp"
#include "material/definition/Sinter.hpp"
#include "material/definition/Snow.hpp"
#include "material/definition/Soil.hpp"
#include "material/definition/Till.hpp"
#include "material/definition/Timber.hpp"
#include "material/definition/Travertine.hpp"
#include "material/definition/Tuff.hpp"
#include "material/definition/Water.hpp"

#include <array>
#include <cstddef>

namespace derivean::sim {

// The order is the stored voxel id. Append new materials; never reorder existing ones.
#define DERIVEAN_MATERIALS(X) \
    X(Air) X(Moss) X(Soil) X(Clay) X(Shale) X(Chalk) X(Basalt) X(Timber) X(Leaf) X(Bedrock) X(Sand) X(Water) \
    X(Granite) X(Gneiss) X(Schist) X(Quartzite) X(Marble) X(Andesite) X(Tuff) X(Peridotite) \
    X(Sandstone) X(Conglomerate) X(Mudstone) X(Limestone) X(RockSalt) X(Coal) \
    X(Scree) X(Gravel) X(Silt) X(Till) X(Regolith) X(Snow) X(Ice) X(Magma) \
    X(Kimberlite) X(IronFormation) X(OreVein) \
    X(Sinter) X(Travertine)

enum class Material : uint8_t {
#define X(name) name,
    DERIVEAN_MATERIALS(X)
#undef X
    Count
};

inline constexpr int MaterialCount = int(Material::Count);

inline constexpr std::array<const MaterialDefinition*, MaterialCount> MaterialDefinitions{
#define X(name) &definition::name,
    DERIVEAN_MATERIALS(X)
#undef X
};

inline constexpr const MaterialDefinition& definitionOf(Material material) {
    return *MaterialDefinitions[size_t(material)];
}

} // namespace derivean::sim

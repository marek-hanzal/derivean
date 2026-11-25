#include "demo/type/ProvingGrounds.hpp"

namespace derivean {
namespace {

void trough(VoxelWorld& world, int floor, int front, int back, Material wall) {
    fillBox(world, {{-46, floor, back}, {46, floor + 2, front}}, wall);
    fillBox(world, {{-46, floor + 2, back}, {-44, floor + 11, front}}, wall);
    fillBox(world, {{44, floor + 2, back}, {46, floor + 11, front}}, wall);
    fillBox(world, {{-44, floor + 2, back}, {44, floor + 11, back + 2}}, wall);
    fillBox(world, {{-44, floor + 2, front - 2}, {44, floor + 6, front}}, wall);
}

} // namespace

void buildProvingGrounds(VoxelWorld& world, WaterState& water) {
    // All coordinates describe this fixture; the simulation only sees ordinary material.
    fillBox(world, {{-72, -6, -27}, {72, -1, 35}}, Material::Basalt);
    fillBox(world, {{-72, -1, -27}, {72, 0, 35}}, Material::Shale);
    fillBox(world, {{-53, -7, -60}, {-46, 19, -27}}, Material::Basalt);
    fillBox(world, {{46, -7, -60}, {53, 19, -27}}, Material::Basalt);
    fillBox(world, {{-50, -7, -49}, {50, -5, -29}}, Material::Basalt);
    fillBox(world, {{-49, 18, -53}, {49, 20, -29}}, Material::Basalt);

    trough(world, -7, -28, -53, Material::Basalt);
    trough(world, 18, -30, -51, Material::Shale);
    fillBox(world, {{-44, -7, -28}, {44, 0, -27}}, Material::Basalt);

    // Exposed panels make the two bottoms identifiable from the walk deck.
    fillBox(world, {{-35, 18, -37}, {36, 20, -30}}, Material::Clay);
    fillBox(world, {{-35, -7, -35}, {36, -5, -28}}, Material::Chalk);

    // Finite magazines separated by removable gates. The upper gate is clay and the lower timber.
    fillBox(world, {{-23, 20, -49}, {-22, 29, -32}}, Material::Clay);
    fillBox(world, {{22, -5, -51}, {23, 4, -30}}, Material::Timber);
    seedWater(water, world, {{-44, 20, -49}, {-23, 24, -32}});
    seedWater(water, world, {{23, -5, -51}, {44, -1, -30}});

    // Build material samples and the loose bank are part of the same destructible structure.
    fillBox(world, {{26, 0, -15}, {53, 2, -5}}, Material::Shale);
    fillBox(world, {{27, 2, -15}, {52, 8, -5}}, Material::Sand);
    for (int i = 0; i < 4; ++i)
        fillBox(world, {{-69 + i * 5, 0, -20}, {-64 + i * 5, 1 + i, -11}}, Material::Chalk);
}

} // namespace derivean

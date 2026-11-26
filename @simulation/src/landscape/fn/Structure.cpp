#include "landscape/fn/Build.hpp"

#include "rockmass/type/RockMass.hpp"

namespace derivean::sim {

// How the rock under each cell is broken, from the one structural field every level reads
// (rockmass/type/RockMass.hpp): `sound` is the cohesion of the jointed mass against what the
// rock's kind usually has, lower where its joints stand close and at the fault that bounds its
// domain; `weakness` gains the share of the cell that lies in broken rock along the planes a cell
// of this width can tell apart. Water, weather and failure all take such ground first, so valleys
// and faces follow the planes instead of the grid. It is read once, at the bed the surface starts
// in: the planes stand steep, and a cell is wider than their lean over the depth it cuts.
void readStructure(Domain& domain, const Setting& setting) {
    const int size = domain.size;
    domain.sound.assign(domain.cells(), 1.0f);
    forRows(domain, size, [&](int begin, int end) {
        for (int z = begin; z < end; ++z) {
            for (int x = 0; x < size; ++x) {
                const size_t i = domain.index(x, z);
                const StrataColumn& column = domain.strataOf(i);
                const float depth = std::max(0.0f, domain.ground.datum[i] - domain.ground.rock[i]);
                const Vec2 material = domain.materialOf(i);
                const StrataUnit unit = unitAt(column, depth, domain.fold[i]);
                const Bed bed = bedAt(setting.seed, column, unit, depth, material);
                const RockMass mass = rockMassAt(setting.seed, column, bed, unit.laidDown, material);
                const float usual = definitionOf(bed.rock).massCohesion;
                if (usual > 0) domain.sound[i] = std::clamp(mass.cohesion / usual, 0.05f, 4.0f);
                domain.weakness[i] = std::max(domain.weakness[i], fractureShare(setting.seed, mass, material, depth, float(domain.cell)));
            }
        }
    });
}

} // namespace derivean::sim

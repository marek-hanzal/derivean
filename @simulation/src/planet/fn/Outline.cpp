#include "planet/type/Planet.hpp"

namespace derivean::sim {
namespace {

constexpr int Rings = int(ContactReach / float(PlanetCell)) + 1;   // cells the search spreads from a contact
constexpr int StepX[8] = {1, -1, 0, 0, 1, -1, 1, -1};
constexpr int StepZ[8] = {0, 0, 1, -1, 1, 1, -1, -1};

// Offset in cells from a cell to the nearest cell of another plate; zero while none is known.
struct Offset {
    int8_t x{}, z{};
    int squared() const { return x * x + z * z; }
    bool known() const { return x != 0 || z != 0; }
};

} // namespace

// Plates are whatever cells they own, so their contacts are read off the grid: every cell within
// reach learns the nearest cell of another plate, spreading ring by ring from the outlines.
void traceContacts(Planet& planet) {
    const int n = PlanetCells;
    const size_t cells = planet.owner.size();
    const uint16_t* owner = planet.owner.cells.data();
    const std::vector<Plate>& plates = planet.epoch.plates;
    std::vector<Offset> toward(cells);
    std::vector<int32_t> frontier, next;
    const auto wrapped = [n](int index) { return index < 0 ? index + n : index >= n ? index - n : index; };
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const int cell = z * n + x;
            // Where plates meet at a corner the contact that closes fastest is the one that acts.
            double best = 0;
            for (int k = 0; k < 4; ++k) {
                const int foreign = wrapped(z + StepZ[k]) * n + wrapped(x + StepX[k]);
                if (owner[foreign] == owner[cell]) continue;
                const Vec2 relative = plates[owner[cell]].velocity - plates[owner[foreign]].velocity;
                const double closing = relative.x * StepX[k] + relative.z * StepZ[k];
                if (toward[size_t(cell)].known() && closing <= best) continue;
                toward[size_t(cell)] = {int8_t(StepX[k]), int8_t(StepZ[k])};
                best = closing;
            }
            if (toward[size_t(cell)].known()) frontier.push_back(cell);
        }
    }
    for (int ring = 1; ring < Rings && !frontier.empty(); ++ring) {
        next.clear();
        for (const int cell : frontier) {
            const int x = cell % n, z = cell / n;
            const Offset from = toward[size_t(cell)];
            for (int k = 0; k < 8; ++k) {
                const int inner = wrapped(z + StepZ[k]) * n + wrapped(x + StepX[k]);
                if (owner[inner] != owner[cell]) continue;
                const Offset candidate{int8_t(from.x - StepX[k]), int8_t(from.z - StepZ[k])};
                const Offset known = toward[size_t(inner)];
                if (known.known() && known.squared() <= candidate.squared()) continue;
                if (!known.known()) next.push_back(inner);
                toward[size_t(inner)] = candidate;
            }
        }
        frontier.swap(next);
    }

    for (const uint32_t cell : planet.contactZone) planet.contact.cells[cell] = ContactCell{};
    planet.contactZone.clear();
    for (size_t cell = 0; cell < cells; ++cell) {
        const Offset offset = toward[cell];
        if (!offset.known()) continue;
        const int foreign = wrapped(int(cell) / n + offset.z) * n + wrapped(int(cell) % n + offset.x);
        const Vec2 normal = normalized(Vec2{double(offset.x), double(offset.z)});
        const Vec2 relative = plates[owner[cell]].velocity - plates[owner[foreign]].velocity;
        ContactCell& contact = planet.contact.cells[cell];
        // The contact runs between the two cells, half a cell short of the foreign one.
        contact.distance = float((std::sqrt(double(offset.squared())) - 0.5) * PlanetCell);
        if (contact.distance >= ContactReach) continue;
        contact.convergence = float(dot(relative, normal));
        contact.shear = float(dot(relative, perpendicular(normal)));
        contact.normalX = float(normal.x), contact.normalZ = float(normal.z);
        planet.contactZone.push_back(uint32_t(cell));
    }
}

} // namespace derivean::sim

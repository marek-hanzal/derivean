#include "GroundwaterFixture.hpp"

#include "groundwater/fn/Beds.hpp"

#include <cstdio>

using namespace derivean::sim;

namespace {

constexpr uint32_t ShaleUnit = 9;

// A scarp facing east: sandstone on shale on granite, beds falling half a metre in a hundred to
// the east, the ground falling thirty. The shale comes to daylight between columns 127 and 128.
// Rain soaks in on the 28 columns west of there.
void testContactSprings() {
    constexpr int Lip = 128;
    WaterField field = handField([](int x, int) {
        const float east = (Lip - x - 0.5f) * float(WaterCell);
        HandColumn hand;
        hand.ground = 500.0f + 0.3f * east;
        hand.rain = x >= Lip - 28 && x < Lip ? 0.3f : 0.0f;
        const float shaleTop = std::min(hand.ground, 500.0f + 0.005f * east), shaleBottom = 470.0f + 0.005f * east;
        if (hand.ground > shaleTop + 0.1f) hand.layers[hand.count++] = handLayer(Material::Sandstone, hand.ground, shaleTop, 3e-5f, 7);
        if (shaleTop > shaleBottom + 1.0f) hand.layers[hand.count++] = handLayer(Material::Shale, shaleTop, shaleBottom, 1e-10f, ShaleUnit);
        hand.layers[hand.count++] = handLayer(Material::Granite, std::min(hand.ground, shaleBottom), hand.ground - 250.0f, 1e-8f);
        return hand;
    });
    for (AquiferColumn& column : field.column) column.dipX = 0.005f;
    const std::shared_ptr<GroundwaterTile> tile = solveField(field);

    int contact = 0;
    double yield = 0;
    for (size_t i = 0; i < tile->springs.size(); ++i) {
        const Spring& spring = tile->springs[i];
        const SpringRecord& record = tile->records[i];
        if (record.kind != SpringKind::Contact) continue;
        ++contact, yield += spring.discharge;
        expectWater(std::abs(spring.position.x - Lip * WaterCell) < WaterCell, "a contact spring lies away from the outcrop of the tight bed", spring.position.x);
        expectWater(std::abs(spring.elevation - 500.0f) < 12.0f, "a contact spring is not at the height of the tight bed's lip", spring.elevation);
        expectWater(record.aquifer == Material::Sandstone && record.floor == Material::Shale, "a contact spring does not name its aquifer and the bed that turns it out");
        expectWater(record.path > 300 && record.catchment > 10 * WaterCellArea, "a contact spring has no catchment above it", record.catchment);
    }
    const WaterBudget& budget = tile->budget;
    expectWater(contact >= TileCells - 4, "the outcrop of a tight bed under an aquifer is not a line of springs", contact);
    expectWater(yield * PerYear > 0.9 * budget.recharge, "the spring line does not yield the recharge of the aquifer above it", yield * PerYear / budget.recharge);
    expectWater(std::abs(imbalance(budget)) < 1e-6, "the water of a perched aquifer is not all accounted for", imbalance(budget));
    for (int x = Lip - 20; x < Lip - 2; ++x) {
        const size_t cell = size_t(128) * TileCells + size_t(x);
        expectWater(tile->state[cell] & HasPerched, "sandstone on shale holds no perched water");
        expectWater(tile->table[cell] > tile->deepTable[cell] + 5.0f, "perched water does not stand above the water under its bed", tile->table[cell] - tile->deepTable[cell]);
    }
    std::printf("ground water: %d contact springs along %d cells of outcrop yield %.1f l/s, %.1f %% of the recharge above them\n", contact, TileCells, 1000 * yield,
                100 * yield * PerYear / budget.recharge);
}

// Rock at one point of a segment's surroundings: the least distance from a point to its axis.
double axisDistance(const Conduit& c, double x, double y, double z) {
    const double dx = c.bx - c.ax, dy = c.by - c.ay, dz = c.bz - c.az, length2 = dx * dx + dy * dy + dz * dz;
    const double t = length2 > 0 ? std::clamp(((x - c.ax) * dx + (y - c.ay) * dy + (z - c.az) * dz) / length2, 0.0, 1.0) : 0.0;
    return std::sqrt((x - c.ax - t * dx) * (x - c.ax - t * dx) + (y - c.ay - t * dy) * (y - c.ay - t * dy) + (z - c.az - t * dz) * (z - c.az - t * dz));
}

// A limestone plateau on shale beside a river cut sixty metres into it. Rain soaks in everywhere
// and a stream from elsewhere goes underground eleven kilometres from the river.
void testLimestoneBlock() {
    constexpr int Valley = 200, SinkX = 30, SinkZ = 128;
    WaterField field = handField([](int x, int) {
        HandColumn hand;
        hand.ground = x < Valley ? 300.0f : 240.0f;
        hand.rain = 0.4f;
        hand.layers[hand.count++] = handLayer(Material::Limestone, hand.ground, 200.0f, 3e-5f, 5);
        hand.layers[hand.count++] = handLayer(Material::Shale, 200.0f, 190.0f, 1e-10f, ShaleUnit);
        hand.layers[hand.count++] = handLayer(Material::Granite, 190.0f, 50.0f, 1e-8f);
        return hand;
    });
    RockMass mass;
    mass.sets[0] = {0.3, 0, 6.0f, 0.8f}, mass.sets[1] = {0.3 + 1.5707963, 0, 6.0f, 0.8f};
    mass.domain = 1234;
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            AquiferColumn& column = field.column[field.index(x, z)];
            column.fissured = true;
            field.fissures[field.index(x, z)] = fissuresAt(field.seed, mass, field.centre(x, z), 80.0f);
            if (x >= Valley) column.drain = Drain::Stream, column.stream = 1e9f, column.recharge = 0;
        }
    AquiferColumn& sink = field.column[field.index(SinkX, SinkZ)];
    sink.stream = 3.0e6f, sink.swallow = 1.0e5f;
    const std::shared_ptr<GroundwaterTile> tile = solveField(field);
    const CaveField& caves = *tile->caves;
    const WaterBudget& budget = tile->budget;
    expectWater(budget.sunk > 2.9e6, "a stream on bare limestone does not go underground", budget.sunk);
    expectWater(std::abs(imbalance(budget)) < 1e-6, "the water of a limestone block is not all accounted for", imbalance(budget));

    // The stream comes out again: the spring its shaft drains to stands at the river and yields more than went down.
    const ConduitRecord* shaft = nullptr;
    double trunk = 0, straight = 0, widest = 0;
    for (size_t i = 0; i < caves.conduits.size(); ++i) {
        const Conduit& c = caves.conduits[i];
        const ConduitRecord& record = tile->passages[i];
        const double run = std::hypot(c.bx - c.ax, c.bz - c.az);
        widest = std::max(widest, double(c.radius));
        if (run < 1e-6 && std::abs(c.ax - field.centre(SinkX, SinkZ).x) < WaterCell && std::abs(c.az - field.centre(SinkX, SinkZ).z) < WaterCell) shaft = &record;
        if (c.radius < 0.5f || run < 1.0 || record.level) continue;
        trunk += run;
        // Along a joint zone a passage keeps the zone's direction.
        if (record.order != 2) continue;
        const double bearing = std::atan2(c.bz - c.az, c.bx - c.ax) - record.strike, off = std::abs(std::remainder(bearing, 3.14159265358979));
        if (off < 0.26) straight += run;
    }
    expectWater(shaft && shaft->spring != 0, "no shaft takes the sinking stream down to a conduit that has an outlet");
    const Spring* rising = nullptr;
    const SpringRecord* why = nullptr;
    for (size_t i = 0; i < tile->springs.size(); ++i)
        if (shaft && tile->springs[i].id == shaft->spring) rising = &tile->springs[i], why = &tile->records[i];
    expectWater(rising && why->kind == SpringKind::Karst, "the stream that sinks does not come out again at a karst spring");
    expectWater(rising->position.x > (Valley - 2) * WaterCell, "the karst spring is not at the river", rising->position.x);
    expectWater(rising->discharge * PerYear > 3.0e6, "the karst spring yields less than the stream that sank", rising->discharge * PerYear);
    expectWater(why->passages > (Valley - SinkX) * WaterCell, "no passage runs through from the sink to the spring", why->passages);
    expectWater(widest > 1.0 && trunk > 20e3, "flow has not gathered into conduits a person could walk", trunk);
    expectWater(straight > 0.6 * trunk, "passages do not follow the joint zones", straight / std::max(1.0, trunk));
    // Cold water that met only the air's own gas lays nothing down, and nothing comes out above the river's level but at the river.
    for (size_t i = 0; i < tile->springs.size(); ++i) {
        expectWater(tile->springs[i].carbonate == 0 && tile->springs[i].silica == 0, "a cold limestone spring brings mineral to lay down", tile->springs[i].carbonate);
        expectWater(tile->records[i].aquifer == Material::Limestone && tile->records[i].floor == Material::Shale, "a karst spring does not name the rock it comes from");
        expectWater(tile->springs[i].position.x > (Valley - 2) * WaterCell, "water of the plateau comes out on the plateau, above its water table", tile->springs[i].position.x);
        // However far the joint plane it rises on lies from the cell's centre, the vent is in the cell its water left the ground in.
        const int32_t cell = tile->records[i].cell;
        const double cornerX = double(int64_t(field.key.x) * TileCells + cell % TileCells) * WaterCell, cornerZ = double(int64_t(field.key.z) * TileCells + cell / TileCells) * WaterCell;
        const Vec2 at = tile->springs[i].position;
        expectWater(at.x >= cornerX && at.x < cornerX + WaterCell && at.z >= cornerZ && at.z < cornerZ + WaterCell, "a karst vent lies outside the cell its water left the ground in", at.x - cornerX);
    }

    // The voids are there for the voxel column, and a point is in a cave exactly where a span holds it.
    int spanned = 0;
    for (size_t i = 0; i < caves.conduits.size(); i += 7) {
        const Conduit& c = caves.conduits[i];
        for (const double part : {0.13, 0.5, 0.91}) {
            const double x = c.ax + (c.bx - c.ax) * part + 0.37 * c.radius, z = c.az + (c.bz - c.az) * part - 0.21 * c.radius, axis = c.ay + (c.by - c.ay) * part;
            CaveSpan spans[8];
            const int count = caveSpans(caves, {x, z}, spans, 8);
            bool holds = false;
            for (int s = 0; s < count; ++s) {
                holds = holds || (axis >= spans[s].bottom && axis < spans[s].top);
                expectWater(s == 0 || spans[s].bottom > spans[s - 1].top, "cave spans overlap or are out of order", spans[s].bottom);
            }
            expectWater(holds, "a vertical line through a conduit finds no void at its axis", axis);
            ++spanned;
            for (double y = axis - 14; y < axis + 14; y += 0.173) {
                bool inSpan = false, inTube = false;
                for (int s = 0; s < count; ++s) inSpan = inSpan || (y >= spans[s].bottom && y < spans[s].top);
                expectWater(inCave(caves, x, y, z) == inSpan, "inCave and caveSpans disagree", y);
                // Against the tubes themselves, away from their skins where rounding decides.
                bool nearSkin = false;
                for (const Conduit& other : caves.conduits) {
                    if (std::abs(other.ax - x) > 200 || std::abs(other.az - z) > 200) continue;
                    const double away = axisDistance(other, x, y, z);
                    inTube = inTube || away < other.radius;
                    nearSkin = nearSkin || std::abs(away - other.radius) < 1e-3;
                }
                expectWater(nearSkin || inTube == inSpan, "a cave span is not the inside of the conduits", y);
            }
        }
    }
    std::printf("ground water: limestone block: %.1f km of passages over 0.5 m (%.0f %% along joint zones), widest %.1f m; the sunk stream (%.0f l/s) rises %.1f km away in a spring of %.0f l/s; %d columns checked\n",
                trunk / 1000, 100 * straight / trunk, widest, budget.sunk / PerYear * 1000, (rising->position.x - field.centre(SinkX, SinkZ).x) / 1000, 1000.0 * rising->discharge, spanned);
}

// A bowl of permeable rock two kilometres across with a fault's crushed rock under its middle.
// With the heat of a magma body below, what rises there boils; without it the spring is only warm.
Spring faultSpring(float gradient, float magmatic, SpringRecord& why) {
    constexpr int Middle = 128;
    WaterField field = handField([](int x, int z) {
        const float away = float(std::hypot(x - Middle, z - Middle));
        HandColumn hand;
        hand.ground = 500.0f + 0.15f * float(WaterCell) * away;
        hand.rain = away < 30 ? 0.3f : 0.0f;
        hand.layers[hand.count++] = handLayer(Material::Tuff, hand.ground, hand.ground - 100.0f, 1e-4f);
        hand.layers[hand.count++] = handLayer(Material::Andesite, hand.ground - 100.0f, hand.ground - (away < 1.5f ? 3000.0f : 220.0f), away < 1.5f ? 2e-6f : 1e-8f);
        return hand;
    });
    for (int z = 0; z < TileCells; ++z)
        for (int x = 0; x < TileCells; ++x) {
            AquiferColumn& column = field.column[field.index(x, z)];
            column.heat.gradient = gradient, column.heat.magmatic = magmatic;
            column.crush = std::hypot(x - Middle, z - Middle) < 1.5 ? 1.0f : 0.0f;
        }
    const std::shared_ptr<GroundwaterTile> tile = solveField(field);
    expectWater(std::abs(imbalance(tile->budget)) < 1e-6, "the water of a fault spring is not all accounted for", imbalance(tile->budget));
    size_t best = 0;
    expectWater(!tile->springs.empty(), "a bowl of permeable rock has no spring at its bottom");
    for (size_t i = 0; i < tile->springs.size(); ++i)
        if (tile->springs[i].discharge > tile->springs[best].discharge) best = i;
    why = tile->records[best];
    return tile->springs[best];
}

void testHeat() {
    SpringRecord hotWhy, coldWhy;
    const Spring hot = faultSpring(0.16f, 0.8f, hotWhy), cold = faultSpring(0.022f, 0.0f, coldWhy);
    expectWater(hotWhy.kind == SpringKind::Fault && hotWhy.depth > 500, "water rising on a fault did not go deep", hotWhy.depth);
    expectWater(hot.temperature > 90 && hot.steamShare > 0.05f, "a spring over hot rock does not boil", hot.temperature);
    expectWater(hot.temperature <= 100.0f - (hot.elevation / 300.0f) + 0.01f, "a spring is hotter than water boils at its height", hot.temperature);
    expectWater(hot.silica > 0.05f, "boiling water from depth brings no silica to lay down", hot.silica);
    expectWater(cold.temperature < 50 && cold.steamShare == 0 && cold.silica == 0, "a spring with no heat under it is hot", cold.temperature);
    expectWater(std::abs(cold.discharge - hot.discharge) < 1e-6f, "heat changed how much water a spring yields", cold.discharge - hot.discharge);
    std::printf("ground water: fault spring of %.0f l/s from %.0f m down: %.0f C with %.0f %% steam and %.2f kg/m3 of silica to lay down over rock of %.0f C; %.0f C and none over ordinary crust\n",
                1000.0 * hot.discharge, hotWhy.depth, hot.temperature, 100.0 * hot.steamShare, hot.silica, hotWhy.reservoir, cold.temperature);
}

} // namespace

void testGroundwaterBlock() {
    testContactSprings();
    testLimestoneBlock();
    testHeat();
}

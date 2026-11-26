#include "volcanic/fn/Volcanic.hpp"

namespace derivean::sim {
namespace {

constexpr float AshFloor = 0.01f;     // m of ash below which a blanket is not followed
constexpr float Shortest = 0.2f;      // share of the cooling length that the most slowly fed flow of an eruption gets

// Moves what the laws laid since the last call into `into` and marks whose it is; returns its volume in m³.
double harvest(Emplacement& field, Laying& laying, std::vector<float>& into, int vent) {
    double volume = 0;
    for (const int32_t at : field.fresh) {
        const size_t i = size_t(at);
        if (field.laid[i] <= 0) continue;
        if (field.rest[i] != Rest::Open) {
            field.passed += double(field.laid[i]) * field.cell * field.cell;
            field.laid[i] = 0;
            continue;
        }
        into[i] += field.laid[i];
        // What filled standing water up to its level was laid under it.
        laying.wet[i] += std::clamp(field.water[i] - (field.ground[i] - field.laid[i]), 0.0f, field.laid[i]);
        volume += field.laid[i];
        laying.vent[i] = uint16_t(vent);
        field.laid[i] = 0;
        laying.any = true;
    }
    field.fresh.clear();
    return volume * field.cell * field.cell;
}

// The blanket of ash is thin against any slope it lies on: it stays where it falls.
double layAsh(const Domain& domain, Emplacement& field, Laying& laying, const Fall& fall, int vent) {
    const float crest = float(fallThickness(fall, fall.vent + fall.stretch * 1e-3));
    if (crest <= AshFloor) return 0;
    // Downwind it reaches farthest; the box around that ellipse is all that is visited.
    const double far = fall.length * std::log(crest / AshFloor) / (1.0 - length(fall.stretch));
    const int x0 = std::max(0, int((fall.vent.x - far - domain.origin.x) / domain.cell)), x1 = std::min(domain.size - 1, int((fall.vent.x + far - domain.origin.x) / domain.cell));
    const int z0 = std::max(0, int((fall.vent.z - far - domain.origin.z) / domain.cell)), z1 = std::min(domain.size - 1, int((fall.vent.z + far - domain.origin.z) / domain.cell));
    double laid = 0;
    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            const size_t i = domain.index(x, z);
            const float thickness = float(fallThickness(fall, domain.center(x, z)));
            if (thickness < AshFloor || field.rest[i] != Rest::Open) continue;
            laying.ash[i] += thickness;
            laying.wet[i] += std::clamp(field.water[i] - field.ground[i], 0.0f, thickness);
            field.ground[i] += thickness;
            if (laying.vent[i] == NoVent) laying.vent[i] = uint16_t(vent);
            laid += double(thickness) * domain.cell * domain.cell;
            laying.any = true;
        }
    }
    return laid;
}

} // namespace

Emplacement fieldOf(const Domain& domain) {
    Emplacement field;
    field.resize(domain.size, domain.size);
    field.cell = domain.cell;
    field.origin = domain.origin;
    const bool flooded = domain.taken > 0;   // before the first step no water has been routed yet
    for (size_t i = 0; i < domain.cells(); ++i) {
        field.ground[i] = domain.ground.rock[i] + domain.sedimentAt(i);
        field.water[i] = std::max(float(domain.seaLevel), flooded ? domain.level[i] : -1e9f);
        field.rest[i] = domain.hold[i] == Free ? Rest::Open : domain.hold[i] == Settled ? Rest::Passing : Rest::Closed;
    }
    return field;
}

void emplace(Domain& domain, Emplacement& field, Laying& laying, int vent, const Outburst& outburst) {
    VolcanicLedger& ledger = domain.volcanic.ledger;
    const Vec2 site = outburst.site;
    const int x = int(std::floor((site.x - domain.origin.x) / domain.cell)), z = int(std::floor((site.z - domain.origin.z) / domain.cell));
    const bool inside = x >= 0 && z >= 0 && x < domain.size && z < domain.size;
    const size_t near = domain.index(std::clamp(x, 0, domain.size - 1), std::clamp(z, 0, domain.size - 1));
    const float repose = ReposeTan[Sand];
    const double before = field.lost + field.passed;
    ledger.erupted += outburst.fall + outburst.lava + outburst.flows + outburst.ash;
    // Lava from a vent outside the domain is not followed: none of it can reach the tile's own cells.
    const auto pour = [&](const Rheology& rheology, double part, std::vector<float>& into, double& booked) {
        if (part <= 0) return;
        if (inside) pourLava(field, rheology, x, z, part);
        else ledger.left += part;
        booked += harvest(field, laying, into, vent);
    };
    // Fall around the vent and lava in turn: that is what layers a cone.
    for (int pulse = 0; pulse < outburst.pulses; ++pulse) {
        if (outburst.fall > 0) {
            buildCone(field, coneFall(site, outburst.fall / outburst.pulses), repose);
            ledger.cone += harvest(field, laying, laying.fall, vent);
        }
        // Flows of one eruption differ in how fast they are fed, and so in how far they get before
        // they set: the short ones pile up around the vent, the long ones make the foot.
        const float reach = Shortest + (1.0f - Shortest) * float((pulse * 5) % outburst.pulses + 1) / float(outburst.pulses);
        Rheology current = outburst.current, lava = outburst.rheology;
        current.coolLength *= reach, lava.coolLength *= reach;
        pour(current, outburst.flows / outburst.pulses, laying.flows, ledger.cone);
        pour(lava, outburst.lava / outburst.pulses, laying.lava, ledger.lava);
    }
    if (outburst.ash > 0) {
        const double laid = layAsh(domain, field, laying, ashFall(site, outburst.ash, {domain.windX[near], domain.windZ[near]}), vent);
        ledger.ash += laid;
        ledger.drifted += outburst.ash - laid;
    }
    if (outburst.founders > 0) {
        const std::vector<float> drop = founder(field, site, outburst.founders, domain.volcanic.vents[size_t(vent)].magma);
        for (size_t i = 0; i < drop.size(); ++i) {
            if (drop[i] <= 0 || field.rest[i] != Rest::Open) continue;
            laying.drop[i] += drop[i];
            field.ground[i] -= drop[i];
            laying.any = true;
        }
    }
    ledger.left += field.lost + field.passed - before;
}

} // namespace derivean::sim

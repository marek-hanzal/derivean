#include "volcanic/fn/Emplace.hpp"
#include "volcanic/fn/Proving.hpp"
#include "volcanic/fn/Volcanic.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace derivean::sim;

namespace {

constexpr double Km3 = 1e9;

void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Volcanic pile: " << message << " (" << value << ")\n";
    std::exit(1);
}

// What a pile is made of is recorded as it is laid: its lava, what the hot currents laid, and
// what came to lie under standing water.
void testRecords() {
    for (const bool sea : {false, true}) {
        ProvingSpec spec;
        spec.ground.level = Level::Region, spec.ground.slope = 0.002, spec.ground.rock = Material::Granite;
        spec.magma = Magma::Andesite;
        Domain domain = planeDomain(spec.ground);
        domain.volcanic.vents = {provingVent(spec, domain)};
        domain.seaLevel = sea ? 1e5 : -1e5;
        Emplacement field = fieldOf(domain);
        Laying laying;
        for (auto* layer : {&laying.lava, &laying.fall, &laying.flows, &laying.ash, &laying.wet, &laying.drop, &laying.sink}) layer->assign(domain.cells(), 0.0f);
        laying.vent.assign(domain.cells(), NoVent);
        Outburst outburst;
        outburst.site = domain.volcanic.vents.front().centre;
        outburst.fall = 0.2 * Km3, outburst.lava = 0.4 * Km3, outburst.flows = 0.2 * Km3, outburst.ash = 0.2 * Km3;
        outburst.rheology = lavaOf(Magma::Andesite), outburst.current = BlockFlow, outburst.pulses = 4;
        emplace(domain, field, laying, 0, outburst);
        // The usual eruption's block-and-ash currents lie with its fall; a chamber-emptying one's are hot pumice.
        double usual = 0;
        for (float part : laying.flows) usual += part;
        expect(usual == 0, "the currents of a usual eruption are laid as hot pumice", usual);
        Outburst blast;
        blast.site = outburst.site, blast.flows = 20 * Km3, blast.current = AshFlow, blast.founders = 0.2 * Km3;
        emplace(domain, field, laying, 0, blast);
        collapse(domain, field, laying, 0, blast);
        double laid = 0, wet = 0, flows = 0, recorded = 0, all = 0, thick = 0;
        for (const float part : laying.flows) thick += part >= Welding ? part : 0.0f;
        for (size_t i = 0; i < domain.cells(); ++i) laid += laying.lava[i] + laying.fall[i] + laying.flows[i] + laying.ash[i], wet += laying.wet[i];
        settleLaying(domain, laying);
        for (size_t i = 0; i < domain.cells(); ++i) flows += domain.volcanic.flowLaid[i], recorded += domain.volcanic.wetLaid[i], all += domain.volcanic.allLaid[i];
        // The hot currents weld where they lay thick and dry; under the sea they quench.
        if (sea) expect(flows == 0, "hot currents laid under the sea are recorded as welded", flows / std::max(all, 1e-9));
        else expect(flows > 0 && flows < all && std::abs(flows - thick) < 1e-3 * thick, "the hot currents that welded are not those laid thick enough", flows / std::max(thick, 1e-9));
        if (sea) expect(wet > 0.99 * laid && recorded > 0.9 * all, "what a vent under the sea laid is not recorded as laid under water", recorded / std::max(all, 1e-9));
        else expect(wet == 0 && recorded == 0, "what a vent on dry land laid is recorded as laid under water", wet);
    }
}

// What each member of a pile is, from what the cell's eruptions laid and where.
void testMembers() {
    const auto count = [](PilePoint point, Material rock, bool sheet = false) {
        int found = 0;
        for (float height = 0.5f; height < 3400.0f; height += 1.0f) {
            const PileBed bed = pileBed(42, point, height);
            found += bed.rock == rock && (!sheet || bed.sheet);
        }
        return found;
    };
    // A third lava, the rest hot currents: dry andesite beds between flows are thick enough to weld.
    const PilePoint dry{3400.0f, 0.3f, Material::Andesite, 7, 0.7f, 0.0f, 0.0f};
    expect(count(dry, Material::Ignimbrite, true) > 1000, "thick dry currents between andesite flows do not weld into columned sheets", count(dry, Material::Ignimbrite));
    expect(count(dry, Material::Andesite, true) > 500, "the dry flows are not columned andesite", count(dry, Material::Andesite));
    // The same laid under water: no welding and no lava sheets, glass instead.
    PilePoint wet = dry;
    wet.wet = 1.0f;
    expect(count(wet, Material::Ignimbrite) == 0 && count(wet, Material::Andesite) == 0, "currents weld or andesite sets as sheets under water", count(wet, Material::Ignimbrite));
    expect(count(wet, Material::Hyaloclastite) > 500, "andesite laid under water does not shatter to glass", count(wet, Material::Hyaloclastite));
    // Basalt flows quenched in water are pillows; its thin beds never weld.
    PilePoint pillows{3400.0f, 0.6f, Material::Basalt, 7, 0.4f, 0.0f, 1.0f};
    expect(count(pillows, Material::PillowLava) > 800 && count(pillows, Material::Basalt) == 0, "basalt laid under water is not pillow lava", count(pillows, Material::PillowLava));
    expect(count(pillows, Material::Hyaloclastite) > 300, "pillows shed no glass breccia", count(pillows, Material::Hyaloclastite));
    pillows.wet = 0;
    expect(count(pillows, Material::Ignimbrite) == 0, "beds between basalt flows weld though thinner than welding needs", count(pillows, Material::Ignimbrite));
    // A cone's fall is lapilli, the ash beyond it tuff, in their shares.
    const PilePoint cone{3400.0f, 0.0f, Material::Andesite, 7, 0.0f, 0.5f, 0.0f};
    const int lapilli = count(cone, Material::LapilliTuff), tuff = count(cone, Material::Tuff);
    expect(lapilli > 1000 && tuff > 1000, "fall and ash are not laid as lapilli tuff and tuff in their shares", double(lapilli) / std::max(1, tuff));
}

} // namespace

// What a pile records as it is laid, and the members the voxels draw from that.
void testVolcanicPile() {
    testRecords();
    testMembers();
}

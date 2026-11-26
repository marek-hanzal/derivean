#include "volcanic/fn/Volcanic.hpp"

#include "volcanic/fn/Minor.hpp"

namespace derivean::sim {
namespace {

constexpr int MostPulses = 10;          // parts in which a step's fall and lava alternate
constexpr double AshReach = 70000.0;    // m beyond a domain from which a vent's ash is still followed
constexpr double FloodVolume = 1e9;     // m³ of lava in one part of an eruption above which its sheets inflate
constexpr double MostInflation = 5.0;   // times the usual thickness of a flow unit

Laying emptyLaying(const Domain& domain) {
    Laying laying;
    const size_t cells = domain.cells();
    for (auto* layer : {&laying.lava, &laying.fall, &laying.ash, &laying.drop}) layer->assign(cells, 0.0f);
    laying.vent.assign(cells, 255);
    return laying;
}

bool within(const Domain& domain, Vec2 site, double reach) {
    const double span = domain.size * domain.cell;
    return site.x > domain.origin.x - reach && site.z > domain.origin.z - reach && site.x < domain.origin.x + span + reach && site.z < domain.origin.z + span + reach;
}

// What a vent brings up in one step of a region: the largest single eruption apart if it emptied
// the chamber, the rest as the usual turn of fall and lava.
void eruptEpisode(Domain& domain, Emplacement& field, Laying& laying, int index, const Episode& episode) {
    const Vent& vent = domain.volcanic.vents[size_t(index)];
    const bool empties = episode.paroxysm >= CalderaVolume[size_t(vent.magma)];
    const double volume = episode.volume - minorVolume(vent, episode) - (empties ? episode.paroxysm : 0.0);
    if (volume > 0) {
        const Style& style = styleOf(vent.magma);
        Outburst outburst;
        outburst.site = episode.site;
        outburst.fall = volume * style.cone;
        outburst.lava = volume * style.lava;
        outburst.flows = volume * style.flows;
        outburst.ash = volume * style.ash;
        outburst.rheology = lavaOf(vent.magma);
        outburst.current = BlockFlow;
        outburst.pulses = std::clamp(int(2.0 + std::log2(1.0 + volume / 1e9)), 2, MostPulses);
        // Fluid melt that comes up fast travels as thick, inflated sheets and holds a gentler
        // slope: great outpourings build plains, small ones shields.
        if (vent.magma == Magma::Basalt) outburst.rheology.sheet *= float(std::clamp(std::sqrt(outburst.lava / outburst.pulses / FloodVolume), 1.0, MostInflation));
        emplace(domain, field, laying, index, outburst);
    }
    if (!empties) return;
    // The step ends with it: what the vent brings up afterwards belongs to later steps and fills the pit.
    Outburst blast;
    blast.site = episode.site;
    blast.fall = episode.paroxysm * ParoxysmStyle.cone;
    blast.ash = episode.paroxysm * ParoxysmStyle.ash;
    blast.flows = episode.paroxysm - blast.fall - blast.ash;
    blast.current = AshFlow;
    blast.founders = episode.paroxysm;
    emplace(domain, field, laying, index, blast);
}

} // namespace

void erupt(Domain& domain, FlowScratch&, const Setting& setting) {
    VolcanicDomain& volcanic = domain.volcanic;
    if (volcanic.vents.empty()) return;
    const bool region = domain.key.level == Level::Region;
    if (!region && domain.key.level != Level::Local) return;
    // A region lives through the dates themselves. A local tile settles without dates, but its
    // settling ends at the present, so its last steps stand for the time just before it: an eruption
    // that many years old is built that many years before the end, and erosion has that long to work on it.
    const double begin = domain.time, end = domain.time + domain.step;
    Laying laying;
    Emplacement field;
    const auto ready = [&] {
        if (!field.cells()) field = fieldOf(domain), laying = emptyLaying(domain);
    };
    std::vector<MinorEvent> minor;
    for (size_t index = 0; index < volcanic.vents.size(); ++index) {
        const Vent& vent = volcanic.vents[index];
        if (!region) {
            minor.clear();
            minorEvents(setting.seed, vent, begin, end, minor);
            for (const MinorEvent& event : minor) {
                if (!within(domain, event.site, 0)) continue;
                ready();
                Outburst outburst;
                outburst.site = event.site;
                outburst.fall = event.fall;
                outburst.lava = event.lava;
                outburst.rheology = event.rheology;
                emplace(domain, field, laying, int(index), outburst);
            }
            continue;
        }
        for (const Episode& episode : vent.episodes) {
            if (std::abs(episode.time - domain.time) > 0.5 * domain.step || !within(domain, episode.site, AshReach)) continue;
            ready();
            eruptEpisode(domain, field, laying, int(index), episode);
        }
    }
    if (field.cells()) settleLaying(domain, laying);
}

} // namespace derivean::sim

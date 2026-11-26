#include "volcanic/fn/Proving.hpp"

#include "landscape/fn/Build.hpp"

namespace derivean::sim {

Vent provingVent(const ProvingSpec& spec, const Domain& domain) {
    const double middle = 0.5 * domain.size * domain.cell;
    Vent vent;
    vent.id = spec.ground.seed + 1;
    vent.centre = domain.origin + Vec2{middle, middle} + spec.site;
    vent.magma = spec.magma;
    vent.vigor = 1;
    vent.begin = domain.startTime;
    vent.until = domain.endTime;
    for (int k = 0; k < spec.episodes; ++k) {
        Episode episode{domain.startTime + k * RegionStep, spec.volume, k + 1 == spec.episodes ? spec.paroxysm : 0.0, vent.centre};
        if (episode.time >= domain.endTime) break;
        vent.episodes.push_back(episode);
        vent.erupted += episode.volume;
    }
    vent.feeders.push_back({vent.centre, 150.0f, vent.begin, vent.begin + spec.episodes * RegionStep, vent.erupted});
    return vent;
}

Domain replayProving(const ProvingSpec& spec, Workers* workers) {
    PlaneSpec ground = spec.ground;
    const int perEpisode = int(std::llround(RegionStep / specOf(ground.level).step));
    ground.steps = spec.episodes * perEpisode + spec.quiet;
    Domain domain = planeDomain(ground);
    domain.workers = workers;
    domain.volcanic.vents.push_back(provingVent(spec, domain));
    if (spec.valley > 0) {
        const Vec2 fall{std::cos(ground.azimuth), std::sin(ground.azimuth)};
        const Vec2 vent = domain.volcanic.vents.front().centre;
        for (int z = 0; z < domain.size; ++z) {
            for (int x = 0; x < domain.size; ++x) {
                const size_t i = domain.index(x, z);
                const float cut = float(spec.valley * std::max(0.0, 1.0 - std::abs(cross(fall, domain.center(x, z) - vent)) / spec.valleyWidth));
                domain.ground.rock[i] -= cut, domain.ground.datum[i] -= cut, domain.trend[i] -= cut;
            }
        }
        domain.settledFrom = domain.settledTo = domain.ground;
    }
    // As measure/fn/PlaneDomain.cpp: a step asks the planet for the sea level and its date only.
    Planet planet;
    planet.seaLevel = domain.seaLevel;
    planet.time = domain.endTime;
    Backdrop backdrop;
    const Setting setting{ground.seed, domain.endTime, &planet, &backdrop};
    FlowScratch scratch;
    for (int step = 0; step < ground.steps; ++step) stepDomain(domain, scratch, setting);
    return domain;
}

} // namespace derivean::sim

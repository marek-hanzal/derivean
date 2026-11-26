#include "volcanic/fn/Minor.hpp"

#include "core/fn/Hash.hpp"
#include "landscape/type/Level.hpp"
#include "volcanic/fn/Source.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t MinorStream = 0x4d494e52;
constexpr double Share = 0.03;
constexpr double Typical[2] = {8e7, 4e8};        // m³ of one such eruption, by Magma
constexpr int Most = 8;                          // of them in one episode
constexpr double ConeShare[2] = {0.3, 0.6};      // of its volume that falls as fragments
constexpr double SummitShare[2] = {0.25, 0.6};   // of them that come up the main conduit
constexpr double FlankReach[2] = {6000.0, 7000.0};   // m from the conduit to the farthest of the others
// A small eruption's lava sets within sight of its vent, however fluid: it is fed too slowly to keep
// its heat. This also keeps a flow inside the halo of a local tile, so both sides of an edge see it whole.
constexpr float ShortCooling = 1700.0f;

} // namespace

// A small eruption keeps its size however productive the vent: a busier vent has more of them, up
// to as many as the local level builds in one episode, and the region keeps the rest. A chamber-
// emptying eruption is the region's whole; the share is of what the episode brings up besides it.
double minorVolume(const Vent& vent, const Episode& episode) {
    if (vent.until - episode.time > specOf(Level::Local).span) return 0.0;
    const double usual = episode.volume - (emptiesChamber(vent, episode.paroxysm) ? episode.paroxysm : 0.0);
    return usual > 0 ? std::min(Share * usual, Most * Typical[size_t(vent.magma)]) : 0.0;
}

double minorSize(Magma magma) { return Typical[size_t(magma)]; }

MinorStyle minorStyle(Magma magma, bool summit) {
    MinorStyle style;
    style.cone = ConeShare[size_t(magma)];
    // Viscous melt that comes up beside the main conduit has lost its gas: a dome.
    style.lava = magma == Magma::Andesite && !summit ? DomeLava : lavaOf(magma);
    style.lava.coolLength = std::min(style.lava.coolLength, ShortCooling);
    if (magma == Magma::Andesite && !summit) style.cone = 0;
    return style;
}

void minorEvents(uint64_t seed, const Vent& vent, double from, double to, std::vector<MinorEvent>& into) {
    const uint64_t base = stream(seed, MinorStream) ^ vent.id;
    const size_t magma = size_t(vent.magma);
    const size_t first = into.size();
    for (const Episode& episode : vent.episodes) {
        if (episode.time > to || episode.time + RegionStep <= from) continue;
        const double volume = minorVolume(vent, episode);
        if (volume <= 0) continue;
        const int64_t step = int64_t(std::llround(episode.time / RegionStep));
        const int count = std::clamp(int(std::lround(volume / Typical[magma])), 1, Most);
        // Fissures of one episode share a trend: its cones stand in a row.
        const double trend = 3.141592653589793 * unit(base, step, 1);
        for (int k = 0; k < count; ++k) {
            MinorEvent event;
            event.time = std::min(vent.until, episode.time + RegionStep * unit(base, step, k, 2));
            if (event.time <= from || event.time > to) continue;
            const bool summit = unit(base, step, k, 3) < SummitShare[magma];
            const double along = summit ? 150.0 * (unit(base, step, k, 4) - 0.5) : FlankReach[magma] * (2.0 * unit(base, step, k, 4) - 1.0);
            const double across = (summit ? 150.0 : 500.0) * (unit(base, step, k, 5) - 0.5);
            event.site = episode.site + Vec2{std::cos(trend), std::sin(trend)} * along + Vec2{-std::sin(trend), std::cos(trend)} * across;
            const MinorStyle style = minorStyle(vent.magma, summit);
            event.fall = volume / count * style.cone;
            event.lava = volume / count - event.fall;
            event.rheology = style.lava;
            into.push_back(event);
        }
    }
    std::sort(into.begin() + std::ptrdiff_t(first), into.end(), [](const MinorEvent& a, const MinorEvent& b) { return a.time < b.time; });
}

} // namespace derivean::sim

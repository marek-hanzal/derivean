#include "measure/fn/Scout.hpp"

#include <functional>
#include <set>

namespace derivean::sim {
namespace {

constexpr int Tries = 5;   // Region candidates looked at per kind before giving up on finding `keep` of them

using Detector = std::function<std::vector<ScoutSite>(const Mosaic&, int)>;

struct Search {
    World& world;
    ScoutReport& report;
    double radius;
    int keep;
    std::set<std::pair<int, int>> counted;   // Local tiles already added to the sandstone count
    std::vector<ScoutSite> lakes;            // basins met in any Local mosaic, whatever it was built for
    std::vector<ScoutSite> cliffs;           // and steep bare sandstone
    std::vector<ScoutSite> cuts;             // and gorges, which a Region cell is too wide to show
    std::vector<ScoutSite> cities;           // and rock-city ground
    std::vector<ScoutSite> summits;          // and cold crystalline summits

    Mosaic local(Vec2 at, double reach) {
        Mosaic mosaic = gatherAround(world, Level::Local, at, reach);
        for (int tz = 0; tz < mosaic.tiles; ++tz)
            for (int tx = 0; tx < mosaic.tiles; ++tx)
                if (counted.insert({mosaic.first.x + tx, mosaic.first.z + tz}).second) countSandstone(mosaic, tx, tz, report.localLand, report.localSandstone);
        for (ScoutSite& lake : findBasins(mosaic, 4)) {
            frameSite(mosaic, lake, 1200, 350);
            lakes.push_back(lake);
        }
        for (ScoutSite& cliff : findSandstone(mosaic, 2)) {
            frameSite(mosaic, cliff, 250, 80);
            cliffs.push_back(cliff);
        }
        for (ScoutSite& cut : findGorges(mosaic, 2)) {
            frameSite(mosaic, cut, 700, 250);
            cuts.push_back(cut);
        }
        for (ScoutSite& city : findRockCities(mosaic, world.planet, 2)) {
            frameSite(mosaic, city, 400, 120);
            cities.push_back(city);
        }
        for (ScoutSite& summit : findColdSummits(mosaic, 2)) {
            frameSite(mosaic, summit, 4000, 600);
            summits.push_back(summit);
        }
        for (CatalogueSite& found : findCatalogue(mosaic)) {
            frameSite(mosaic, found.site, 4000, 800);
            CatalogueSite* kept = nullptr;
            for (CatalogueSite& other : report.catalogue)
                if (other.type == found.type) kept = &other;
            if (!kept) report.catalogue.push_back(found);
            else if (found.site.score > kept->site.score) *kept = found;
        }
        return mosaic;
    }

    // Nearer places first among equals: a site at the edge of the search loses half its score's size.
    std::vector<ScoutSite> ranked(std::vector<ScoutSite> sites) {
        for (ScoutSite& site : sites) site.score -= 0.5 * std::abs(site.score) * std::min(1.0, length(site.at - report.start) / radius);
        std::sort(sites.begin(), sites.end(), [](const ScoutSite& a, const ScoutSite& b) { return a.score > b.score; });
        return sites;
    }

    // Region candidates of one kind, confirmed by the same detector on Local tiles around each.
    std::vector<ScoutSite> confirm(const Mosaic& region, const Detector& detect, double reach, double airBack, double airUp) {
        std::vector<ScoutSite> kept;
        int tried = 0;
        for (const ScoutSite& candidate : ranked(detect(region, 2 * Tries))) {
            if (int(kept.size()) >= keep || tried++ >= Tries) break;
            const Mosaic mosaic = local(candidate.at, reach);
            // The best the same detector finds around the candidate that is not a site already kept.
            for (ScoutSite& found : detect(mosaic, 4)) {
                bool taken = length(found.at - candidate.at) > 2 * reach;
                for (const ScoutSite& other : kept) taken = taken || length(other.at - found.at) < 3000;
                if (taken) continue;
                frameSite(mosaic, found, airBack, airUp);
                kept.push_back(found);
                break;
            }
        }
        // Where 64 m cells confirm too few, the best of the Region's own stand in and say so.
        for (const ScoutSite& candidate : ranked(detect(region, keep))) {
            if (int(kept.size()) >= keep) break;
            bool taken = false;
            for (const ScoutSite& other : kept) taken = taken || length(other.at - candidate.at) < 8000;
            if (taken) continue;
            ScoutSite site = candidate;
            site.reason += " (region level only: not found again at 64 m)";
            frameSite(region, site, 4 * airBack, 2 * airUp);
            kept.push_back(site);
        }
        return kept;
    }
};

} // namespace

std::vector<ScoutSite> keepApart(std::vector<ScoutSite> sites, int limit, double apart) {
    std::sort(sites.begin(), sites.end(), [](const ScoutSite& a, const ScoutSite& b) { return a.score > b.score; });
    std::vector<ScoutSite> kept;
    for (const ScoutSite& site : sites) {
        if (int(kept.size()) >= limit) break;
        bool crowded = false;
        for (const ScoutSite& other : kept) crowded = crowded || length(site.at - other.at) < apart;
        if (!crowded) kept.push_back(site);
    }
    return kept;
}

ScoutReport scoutAround(World& world, Vec2 start, double radius, int keep) {
    ScoutReport report;
    report.start = start;
    Search search{world, report, radius, keep, {}, {}, {}, {}, {}, {}};
    size_t before = world.atlas.finished();
    const Mosaic region = gatherAround(world, Level::Region, start, radius);
    report.regionTiles = world.atlas.finished() - before;
    before = world.atlas.finished();

    report.coast = search.confirm(region, findCoasts, 8000, 1500, 250);
    report.valley = search.confirm(region, findValleys, 5000, 1800, 450);
    for (const ScoutSite& candidate : search.ranked(findGorges(region, Tries))) search.local(candidate.at, 3000);
    const Planet& planet = world.planet;
    for (const ScoutSite& candidate : search.ranked(findRockCities(region, planet, Tries))) search.local(candidate.at, 3000);
    for (const ScoutSite& candidate : search.ranked(findColdSummits(region, Tries))) search.local(candidate.at, 4000);
    report.hot = search.confirm(region, [&](const Mosaic& mosaic, int limit) { return findHotGround(mosaic, planet, limit); }, 2500, 1500, 400);
    report.plain = search.confirm(region, findPlains, 7000, 300, 120);
    for (const ScoutSite& candidate : search.ranked(findSandstone(region, Tries))) search.local(candidate.at, 3000);

    // A pass with its two summits needs 10 km of ground on either side. Where the Local tiles
    // around a Region pass show one of their own it is taken; otherwise the Region's stands, placed on the Local ground.
    int tried = 0;
    for (const ScoutSite& candidate : search.ranked(findSaddles(region, 2 * Tries))) {
        if (int(report.saddle.size()) >= keep || tried++ >= Tries) break;
        const Mosaic mosaic = search.local(candidate.at, 6000);
        ScoutSite site = candidate;
        for (const ScoutSite& found : findSaddles(mosaic, 4))
            if (length(found.at - candidate.at) < 4000) {
                site = found;
                break;
            }
        const int x = cellX(mosaic, site.at), z = cellZ(mosaic, site.at);
        if (mosaic.flooded(x, z)) continue;
        frameSite(mosaic, site, 5000, 900);
        report.saddle.push_back(site);
    }

    // Lakes too small for a Region cell only show at 64 m: every Local mosaic built above was searched for them.
    for (const ScoutSite& candidate : search.ranked(findBasins(region, Tries))) search.local(candidate.at, 4000);
    report.basin = keepApart(search.ranked(std::move(search.lakes)), keep, 3000);
    report.gorge = keepApart(search.ranked(std::move(search.cuts)), keep, 5000);
    report.city = keepApart(search.ranked(std::move(search.cities)), keep, 3000);
    report.summit = keepApart(search.ranked(std::move(search.summits)), keep, 6000);
    report.localTiles = world.atlas.finished() - before;
    before = world.atlas.finished();

    // Whether a cliff carries rock forms only shows in the voxels. Twice as many cliffs as are
    // wanted are sampled and those with the most rock over air are kept.
    std::vector<std::pair<ScoutSite, RockForms>> sampled;
    for (const ScoutSite& site : keepApart(search.ranked(std::move(search.cliffs)), 2 * keep, 3000)) sampled.push_back({site, rockForms(world, site.at)});
    std::stable_sort(sampled.begin(), sampled.end(), [](const auto& a, const auto& b) { return a.second.roofedShare > b.second.roofedShare; });
    for (size_t i = 0; i < sampled.size() && int(i) < keep; ++i) report.sandstone.push_back(sampled[i].first), report.forms.push_back(sampled[i].second);
    for (const ScoutSite& site : report.city) report.cityForms.push_back(rockForms(world, site.at));
    report.siteTiles = world.atlas.finished() - before;
    return report;
}

} // namespace derivean::sim

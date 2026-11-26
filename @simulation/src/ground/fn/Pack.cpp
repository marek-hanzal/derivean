#include "ground/fn/Fabric.hpp"
#include "ground/fn/Partings.hpp"

#include "core/fn/Hash.hpp"
#include "ground/constant/Ground.hpp"

namespace derivean::sim {
namespace {

constexpr uint64_t FabricStream = 0x46414252;
constexpr int Octaves = 4;               // size classes, each twice the one before
constexpr float SmallestPitch = 0.25f;   // m between the places one stone of the smallest class can lie

// Share of a log-normal size distribution that lies between two sizes.
float shareBetween(const Fabric& fabric, float low, float high) {
    const auto below = [&](float size) { return 0.5f * std::erfc(-std::log(size / fabric.median) / (fabric.spread * 1.41421356f)); };
    return below(high) - below(low);
}

} // namespace

void packFabric(StoneBed& bed, const FlowScratch& scratch, const SiteView& sites, const TileLookup& lookup) {
    const FabricContext context = fabricContextOf(bed, scratch, lookup);
    const Domain& domain = bed.domain;
    const uint64_t base = stream(bed.seed, FabricStream);
    const Vec2 corner = bed.tileCorner();
    const double span = TileCells * domain.cell;
    // The largest class first, each on its own world-anchored grid of places.
    for (int octave = Octaves - 1; octave >= 0; --octave) {
        const double pitch = SmallestPitch * double(1 << octave);
        const float low = 0.88f * float(pitch), high = 2 * low, typical = 1.3f * low;
        const int64_t firstX = int64_t(std::floor(corner.x / pitch)), firstZ = int64_t(std::floor(corner.z / pitch));
        const int count = int(span / pitch);
        for (int gz = 0; gz < count; ++gz) {
            for (int gx = 0; gx < count; ++gx) {
                const uint64_t draw = hash(base + uint64_t(octave), firstX + gx, firstZ + gz);
                const Vec2 at{(double(firstX + gx) + unit(hash(draw, 11))) * pitch, (double(firstZ + gz) + unit(hash(draw, 12))) * pitch};
                if (!bed.owns(at)) continue;
                const size_t cell = bed.cellAt(at);
                if (bed.taken[cell]) continue;
                const Fabric fabric = fabricAt(context, cell, draw);
                if (fabric.cover <= 0 || fabric.median <= 0 || low > fabric.largest) continue;
                const float part = shareBetween(fabric, low, high);
                // Stones per square metre: as many as cover the share of the surface that is theirs,
                // and no more than the blocks of the coarse layer make up.
                const float byCover = fabric.cover * part / (1.1f * typical * typical);
                const float byVolume = domain.ground.sediment[Coarse][cell] * fabric.blockShare * part / (0.55f * fabric.flatness * typical * typical * typical);
                if (unit(hash(draw, 13)) >= std::min(byCover, byVolume) * pitch * pitch) continue;
                StoneOffer offer;
                offer.at = at;
                offer.size = std::min(fabric.largest, low * std::pow(2.0f, float(unit(hash(draw, 14)))));
                offer.flatness = fabric.flatness;
                offer.roundness = fabric.roundness;
                // A stone under fine cover sits in the layer below it with a third of its height.
                const float height = offer.size * fabric.flatness;
                offer.sink = fabric.buried >= 0 ? std::min(1.0f, (fabric.buried + 0.3f * height) / height)
                                                : fabric.sinkLow + (fabric.sinkHigh - fabric.sinkLow) * float(unit(hash(draw, 15)));
                if (offer.sink > 0.9f) continue;
                offer.heading = fabric.imbricate ? fabric.heading : float(unit(hash(draw, 16)) * 6.2832);
                offer.imbricate = fabric.imbricate;
                offer.rock = fabric.rock;
                offer.origin = fabric.origin;
                offer.source = fabric.source;
                offer.travel = fabric.travel;
                offer.draw = draw;
                if (fabric.origin == ClastOrigin::Stream) {
                    // Most bed material is local, some has come the length of the stream; it wears round on the way.
                    const float journey = float(unit(hash(draw, 1)) * unit(hash(draw, 2))) * reachOf(domain.stream[cell]);
                    const Upstream from = upstreamOf(sites, at, journey);
                    const RockTraits& rock = traitsOf(from.rock);
                    offer.rock = from.rock;
                    offer.source = from.at;
                    offer.travel = std::max(from.followed, journey);
                    offer.flatness = blockHabitOf(rock).flatness;
                    offer.roundness = float(std::clamp(1.0 - std::exp(-double(offer.travel) * rock.abrasion * 33.0), 0.15, 0.95));
                }
                layStone(bed, offer);
            }
        }
    }
}

} // namespace derivean::sim

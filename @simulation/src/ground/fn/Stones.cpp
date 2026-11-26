#include "ground/fn/Stones.hpp"

#include "core/fn/Hash.hpp"

namespace derivean::sim {
namespace {

constexpr float PayReach = 3.0f;      // intermediate axes around a stone whose coarse layer pays for it
constexpr float WidestPay = 6.0f;     // m
constexpr float LeastShare = 0.35f;   // share of its volume a layer must hold for a smaller stone to be laid instead
constexpr float TooSteep = 0.95f;     // rise over run of ground on which no loose stone stays

// How much of a cell's coarse layer a stone `far` cells away (squared) draws on: all of it under the
// stone, none at the edge of its reach, so that no rim shows where the reach ends.
float drawOn(int far, int reach) { return 1.0f - float(far) / float(reach * reach + 1); }

// m³ of coarse layer a stone at a domain cell can draw on in the tile's own cells within `reach` cells.
double coarseAround(const Domain& domain, int cx, int cz, int reach) {
    double held = 0;
    for (int z = std::max(Halo, cz - reach); z <= std::min(Halo + TileCells - 1, cz + reach); ++z)
        for (int x = std::max(Halo, cx - reach); x <= std::min(Halo + TileCells - 1, cx + reach); ++x) {
            const int far = (x - cx) * (x - cx) + (z - cz) * (z - cz);
            if (far <= reach * reach) held += domain.ground.sediment[Coarse][domain.index(x, z)] * drawOn(far, reach);
        }
    return held * domain.cell * domain.cell;
}

} // namespace

bool StoneBed::owns(Vec2 at) const {
    const Vec2 corner = tileCorner();
    const double span = TileCells * domain.cell;
    return at.x >= corner.x && at.z >= corner.z && at.x < corner.x + span && at.z < corner.z + span;
}

size_t StoneBed::cellAt(Vec2 at) const {
    return domain.index(std::clamp(int((at.x - domain.origin.x) / domain.cell), 0, domain.size - 1), std::clamp(int((at.z - domain.origin.z) / domain.cell), 0, domain.size - 1));
}

bool layStone(StoneBed& bed, const StoneOffer& offer) {
    Domain& domain = bed.domain;
    if (!bed.owns(offer.at)) return false;
    const size_t cell = bed.cellAt(offer.at);
    const int cx = int(cell % size_t(domain.size)), cz = int(cell / size_t(domain.size));
    if (bed.taken[cell]) return false;

    Clast stone;
    stone.b = 0.5f * offer.size;
    stone.a = stone.b * float(1.15 + 0.5 * unit(hash(offer.draw, 101)));
    stone.c = std::min(stone.b, stone.b * offer.flatness * float(0.75 + 0.4 * unit(hash(offer.draw, 102))));
    if (stone.a > LargestClast) {
        const float fit = LargestClast / stone.a;
        stone.a *= fit, stone.b *= fit, stone.c *= fit;
    }
    stone.roundness = offer.roundness;
    if (stone.b < SmallestClast) return false;
    // The fall line under the stone, over its own width. Nothing loose rests on ground much steeper
    // than its angle of repose, unless water has wedged it into a bed.
    const int span = std::clamp(int(stone.b / float(domain.cell)), 1, 6);
    const auto top = [&](int x, int z) { return bed.groundAt(domain.index(std::clamp(x, 0, domain.size - 1), std::clamp(z, 0, domain.size - 1))); };
    const float dx = (top(cx + span, cz) - top(cx - span, cz)) / float(2 * span * domain.cell), dz = (top(cx, cz + span) - top(cx, cz - span)) / float(2 * span * domain.cell);
    const float steep = std::min(1.2f, std::sqrt(dx * dx + dz * dz)), fall = std::atan2(-dz, -dx);
    if (steep > TooSteep && offer.origin != ClastOrigin::Stream) return false;
    const float wanted = stone.volume();
    bed.ledger.offered += wanted;

    int reach = std::max(2, int(PayReach * 2 * stone.b / float(domain.cell)));
    double held = coarseAround(domain, cx, cz, reach);
    if (held < wanted && reach < int(WidestPay / domain.cell)) {
        reach = std::min(int(WidestPay / domain.cell), 2 * reach);
        held = coarseAround(domain, cx, cz, reach);
    }
    if (held < LeastShare * wanted) return false;
    if (held < wanted) {
        const float fit = std::cbrt(float(held / wanted) * 0.98f);
        stone.a *= fit, stone.b *= fit, stone.c *= fit;
        if (stone.b < SmallestClast) return false;
    }
    const float volume = stone.volume();
    const float taken = float(volume / held);
    for (int z = std::max(Halo, cz - reach); z <= std::min(Halo + TileCells - 1, cz + reach); ++z)
        for (int x = std::max(Halo, cx - reach); x <= std::min(Halo + TileCells - 1, cx + reach); ++x) {
            const int far = (x - cx) * (x - cx) + (z - cz) * (z - cz);
            if (far <= reach * reach) domain.ground.sediment[Coarse][domain.index(x, z)] *= 1.0f - taken * drawOn(far, reach);
        }

    // A stream leaves its stones across the current, leaning upstream; anything else lies as it came to rest.
    stone.yaw = offer.imbricate ? offer.heading + 1.5708f + float(unit(hash(offer.draw, 103)) - 0.5) * 0.7f
                                : offer.heading + float(unit(hash(offer.draw, 103)) - 0.5) * 2.6f;
    stone.pitch = std::atan(-steep * std::cos(stone.yaw - fall)) + float(unit(hash(offer.draw, 104)) - 0.5) * 0.3f * (1 - offer.roundness);
    stone.roll = std::atan(-steep * std::sin(fall - stone.yaw)) + (offer.imbricate ? 0.3f : float(unit(hash(offer.draw, 105)) - 0.5) * 0.3f);
    const Vec2 corner = bed.tileCorner();
    stone.x = float(offer.at.x - corner.x);
    stone.z = float(offer.at.z - corner.z);
    // On a slope a stone beds in deeper: its downhill end would hang clear otherwise.
    stone.y = bed.groundAt(cell) + stone.c * (1 - 2 * std::clamp(offer.sink + 0.25f * steep, 0.0f, 0.9f));
    stone.rock = offer.rock;
    stone.origin = offer.origin;
    stone.source = offer.source;
    stone.travel = offer.travel;

    const int foot = std::max(0, int(0.7f * stone.b / float(domain.cell)));
    for (int z = std::max(0, cz - foot); z <= std::min(domain.size - 1, cz + foot); ++z)
        for (int x = std::max(0, cx - foot); x <= std::min(domain.size - 1, cx + foot); ++x) bed.taken[domain.index(x, z)] = 1;
    bed.stones.push_back(stone);
    bed.ledger.stones += volume;
    ++bed.ledger.byOrigin[size_t(offer.origin)];
    return true;
}

} // namespace derivean::sim

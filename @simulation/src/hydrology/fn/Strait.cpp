#include "hydrology/fn/Strait.hpp"

#include "hydrology/fn/Water.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

namespace derivean::sim {
namespace {

// m of head, of difference between the ends, or of margin over a shore or a member's height that
// counts as none. Levels are metres in doubles derived from volumes; below this they no longer say
// which way water runs, and a strait left with less is still.
constexpr double Still = 1e-9;
constexpr int MostSegments = 1'000'000;

// One end of the throat as the water over it stands, read from the stores as they are now: the
// topmost basin above its cell holding water of its own, and that basin's surface. A dry end has
// neither. `lake` is the lake its cell lies in as the lakes were last found.
struct End {
    int32_t cell{}, holder{-1}, lake{};
    double level{-std::numeric_limits<double>::infinity()};
};

End endAt(const Ocean& ocean, int32_t cell) {
    const BasinTree& tree = ocean.basins;
    End end{cell, -1, ocean.lakes.lakeOf[size_t(tree.basinOf[size_t(cell)])]};
    for (int32_t id = tree.basinOf[size_t(cell)]; id > 0; id = tree.basins[size_t(id)].parent)
        if (ocean.lakes.own[size_t(id)] > 0) end.holder = id;
    if (end.holder > 0) end.level = levelOf(tree, end.holder, ocean.lakes.own[size_t(end.holder)]);
    return end;
}

// Members of a basin below `level`; they are kept by rising height.
int32_t membersBelow(const BasinTree& tree, const Basin& basin, double level) {
    const auto first = tree.members.begin() + basin.first;
    return int32_t(std::partition_point(first, first + basin.count, [&](int32_t cell) { return double(tree.height[size_t(cell)]) < level; }) - first);
}

double memberHeight(const BasinTree& tree, const Basin& basin, int32_t member) { return double(tree.height[size_t(tree.members[size_t(basin.first + member)])]); }

enum class Event { Time, DonorArea, DonorEmpty, ReceiverArea, Submerged, ReceiverFull, Head };

void credit(Ocean& ocean, const Throat& throat, int32_t fromCell, const Tally& carried) {
    for (const size_t index : throat.straits) {
        const Strait& strait = ocean.straits[index];
        const double share = strait.area / throat.way.area, sign = strait.a == fromCell ? 1.0 : -1.0;
        ocean.moved[index].water += sign * share * carried.water;
        ocean.moved[index].salt += sign * share * carried.salt;
    }
}

// The throat is a pipe whose height is small against the falls, so it passes orifice flow,
// Q = C·A·√(2g·H) (Torricelli 1644; C after Kirchhoff 1869, docs/credits/hydrology.md), under the
// head H of the upper surface over the higher of its sill and the lower surface. The sill is the
// throat's effective one, never under the ground at either end (Throat::way). While the lower
// surface stands under the sill or the lower end is dry the outflow is free and only the donor
// falls: dH/dt = −Q/A_u. Above the sill it is submerged and both move: dH/dt = −Q·(1/A_u + 1/A_d),
// and 1/A_d is 0 while the lake at the lower end is full and passes on what it gets. In both √H
// falls linearly while the wet areas stand, and both surfaces are linear in H, so every change is
// a threshold in H and the time to it is exact. After each change the ends, the direction, the
// donor, the receiver and the donor's reserve are found again.
void segments(Ocean& ocean, Exchange& exchange, const Throat& throat, double seconds) {
    const BasinTree& tree = ocean.basins;
    std::vector<double>& own = ocean.lakes.own;
    const Strait& way = throat.way;
    const double pull = OrificeCoefficient * way.area * std::sqrt(2 * Gravity);
    double left = seconds;
    for (int segment = 0; left > 0; ++segment) {
        assert(segment < MostSegments);
        if (segment >= MostSegments) return;
        // Two dry ends, or two surfaces at one level, leave the strait still.
        const End a = endAt(ocean, way.a), b = endAt(ocean, way.b);
        if ((a.holder < 0 && b.holder < 0) || std::abs(a.level - b.level) <= Still) return;
        const End& up = a.level > b.level ? a : b;
        const End& down = a.level > b.level ? b : a;
        const int32_t donor = up.holder, leaf = tree.leafOf[size_t(down.cell)];
        const int32_t receiver = leaf > 0 ? pourTarget(tree, own, leaf) : 0;
        // Where what comes out runs into the donor's own lake, the two have become one.
        if (receiver <= 0 || ocean.lakes.lakeOf[size_t(donor)] == receiver || up.lake == down.lake) return;

        const Basin& from = tree.basins[size_t(donor)];
        const Basin& to = tree.basins[size_t(receiver)];
        const double sill = way.sill;
        // The receiver's surface is the one at the lower end, or that end's lake is full and passes on.
        const bool atEnd = down.lake == receiver;
        const double lower = atEnd ? levelOf(tree, receiver, own[size_t(receiver)]) : down.level;
        const bool submerged = atEnd ? own[size_t(receiver)] >= volumeOf(tree, receiver, sill) : lower >= sill;
        const double outlet = submerged ? std::max(sill, lower) : sill;
        const double upper = up.level;
        const double head = upper - outlet;
        // The upper surface at or under the sill: the link is broken. Since the sill is never under
        // the upper end's ground, that end stays wet while water runs.
        if (head <= Still) return;

        // The donor's wet area while it falls; a cell its surface is just at is already dry.
        const int32_t wetUp = membersBelow(tree, from, upper - Still);
        const double areaUp = double(from.below + wetUp) * tree.area;
        const double nextUp = wetUp > 0 ? memberHeight(tree, from, wetUp - 1) : from.base;
        double inverseDown = 0, step = std::numeric_limits<double>::infinity();
        Event receiverEvent = Event::ReceiverFull;
        double receiverRoom = to.capacity - own[size_t(receiver)];
        if (submerged && atEnd) {
            // The receiver's wet area while it rises; a cell its surface is just at is already wet.
            const int32_t wetDown = membersBelow(tree, to, lower + Still);
            inverseDown = 1.0 / (double(to.below + wetDown) * tree.area);
            if (wetDown < to.count) step = (memberHeight(tree, to, wetDown) - lower) / inverseDown;
        } else if (!submerged && atEnd) {
            step = volumeOf(tree, receiver, sill) - own[size_t(receiver)];
        }
        if (step < receiverRoom) {
            receiverRoom = step;
            receiverEvent = submerged ? Event::ReceiverArea : Event::Submerged;
        }

        // Volume against head: v = (H₀ − H) / S, with √H falling at `pace` per second.
        const double spread = 1.0 / areaUp + inverseDown;
        const double pace = 0.5 * pull * spread;
        const double root = std::sqrt(head);
        Event event = Event::Head;
        double volume = head / spread;
        const auto sooner = [&](double candidate, Event kind) {
            if (candidate < volume) volume = candidate, event = kind;
        };
        sooner(areaUp * (upper - nextUp), nextUp > from.base ? Event::DonorArea : Event::DonorEmpty);
        sooner(receiverRoom, receiverEvent);
        const double rootAtEnd = root - pace * left;
        if (rootAtEnd > 0) sooner((head - rootAtEnd * rootAtEnd) / spread, Event::Time);
        // Time to the event without the cancellation of √H₀ − √H for small transfers.
        const double fallen = volume * spread;
        const double time = event == Event::Time ? left : fallen / (pace * (root + std::sqrt(std::max(0.0, head - fallen))));

        // The volume is the formula's, except where a store reaches a level exactly.
        if (event == Event::DonorEmpty) volume = own[size_t(donor)];
        if (event == Event::ReceiverFull) volume = to.capacity - own[size_t(receiver)];
        if (event == Event::Submerged) volume = volumeOf(tree, receiver, sill) - own[size_t(receiver)];
        volume = std::clamp(volume, 0.0, own[size_t(donor)]);
        const bool wasFull = own[size_t(donor)] >= from.capacity;
        if (volume > 0) credit(ocean, throat, up.cell, book(ocean, exchange, donor, receiver, leaf, volume));
        left = event == Event::Time ? 0.0 : left - std::min(left, time);
        // Lakes part when a full donor falls and meet when a receiver fills.
        if (wasFull || event == Event::DonorEmpty || event == Event::ReceiverFull) relake(ocean, exchange);
    }
}

} // namespace

// The next throat of the round reads the lakes as this one left them.
void flowThrough(Ocean& ocean, Exchange& exchange, const Throat& throat, double seconds) {
    const size_t before = exchange.segments;
    segments(ocean, exchange, throat, seconds);
    if (exchange.segments > before) relake(ocean, exchange);
}

} // namespace derivean::sim

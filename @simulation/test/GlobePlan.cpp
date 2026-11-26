// The transfer plan (S3-design.md §4, §11 test 7, Codex V2, DREV-CLAUDE-003-03 and C3): the A/B/C
// fixture settles in three rounds with nothing consumed, a chain ends in its survivor with the kind
// decided there, a cycle defers its lowest key, two donors share a receiver's room by key and back up,
// and the plan does not depend on how the pieces are numbered.
#include "GlobeFixture.hpp"

#include "planet/fn/Plan.hpp"

#include <algorithm>

using namespace derivean::sim;
using namespace derivean::sim::globe_test;

namespace {

struct Piece {
    PieceKey key;
    int plate{};
    double felsic{}, sediment{}, basalt{}, room{};
    bool continent{true};
};

struct Case {
    std::vector<Piece> pieces;
    struct Candidate {
        int piece;
        std::vector<PlanShare> receivers;
        std::vector<int> behind;
    };
    std::vector<Candidate> candidates;
};

// The case with its pieces renumbered by `order` (order[i] is the new index of piece i).
PlanInput inputOf(const Case& c, const std::vector<int>& order) {
    PlanInput input;
    const size_t n = c.pieces.size();
    input.key.resize(n), input.plate.resize(n), input.felsic.resize(n), input.sediment.resize(n), input.basalt.resize(n), input.room.resize(n);
    std::vector<uint8_t> continent(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t j = size_t(order[i]);
        const Piece& p = c.pieces[i];
        input.key[j] = p.key, input.plate[j] = p.plate, input.felsic[j] = p.felsic, input.sediment[j] = p.sediment;
        input.basalt[j] = p.basalt, input.room[j] = p.room, continent[j] = p.continent;
    }
    for (const Case::Candidate& k : c.candidates) {
        PlanCandidate candidate{order[size_t(k.piece)], {}, {}};
        for (const PlanShare& r : k.receivers) candidate.receivers.push_back({order[size_t(r.piece)], r.share});
        for (const int b : k.behind) candidate.behind.push_back(order[size_t(b)]);
        input.candidates.push_back(candidate);
    }
    std::reverse(input.candidates.begin(), input.candidates.end());
    input.sinks = [continent](int donor, int survivor) { return !continent[size_t(donor)] && continent[size_t(survivor)]; };
    return input;
}

std::vector<int> identity(size_t n) {
    std::vector<int> order(n);
    for (size_t i = 0; i < n; ++i) order[i] = int(i);
    return order;
}

// The plan in keys, so two numberings compare.
struct Keyed {
    std::vector<std::tuple<PieceKey, PieceKey, int, double, double, double>> transfers;
    std::vector<PieceKey> consumed, cancelled, deferred;
    int rounds{};
};

Keyed keyed(const PlanInput& input, const PlanResult& plan) {
    Keyed k;
    for (const Transfer& t : plan.transfers)
        k.transfers.push_back({input.key[size_t(t.from)], t.to < 0 ? PieceKey{-9, -9, -9} : input.key[size_t(t.to)], int(t.kind), t.felsic, t.basalt, t.cover});
    for (const int p : plan.consumed) k.consumed.push_back(input.key[size_t(p)]);
    for (const int p : plan.cancelled) k.cancelled.push_back(input.key[size_t(p)]);
    for (const int p : plan.deferred) k.deferred.push_back(input.key[size_t(p)]);
    k.rounds = plan.rounds;
    return k;
}

bool same(const Keyed& a, const Keyed& b) {
    if (a.rounds != b.rounds || !(a.consumed == b.consumed) || !(a.cancelled == b.cancelled) || !(a.deferred == b.deferred)) return false;
    if (a.transfers.size() != b.transfers.size()) return false;
    for (size_t i = 0; i < a.transfers.size(); ++i) {
        const auto& [f1, t1, k1, x1, y1, z1] = a.transfers[i];
        const auto& [f2, t2, k2, x2, y2, z2] = b.transfers[i];
        if (!(f1 == f2) || !(t1 == t2) || k1 != k2 || !sameBits(x1, x2) || !sameBits(y1, y2) || !sameBits(z1, z2)) return false;
    }
    return true;
}

// Every case must give the same plan under a shuffled numbering of its pieces.
PlanResult planned(const Case& c, PlanInput* used = nullptr) {
    const PlanInput input = inputOf(c, identity(c.pieces.size()));
    const PlanResult plan = planTransfers(input);
    std::vector<int> order = identity(c.pieces.size());
    Random random{uint64_t(c.pieces.size()) * 31 + 7};
    for (size_t i = order.size(); i > 1; --i) std::swap(order[i - 1], order[size_t(random.next() * double(i))]);
    const PlanInput shuffled = inputOf(c, order);
    expect(same(keyed(input, plan), keyed(shuffled, planTransfers(shuffled))), "a plan depends on how its pieces are numbered");
    if (used) *used = input;
    return plan;
}

// DREV-CLAUDE-003-03: A (5) collides into B, B (10, at StackLimit) into C (room 5), no room behind.
void checkABC() {
    Case c;
    c.pieces = {{{0, 1, 0}, 0, 5, 0, 0, 0}, {{0, 2, 0}, 1, 10, 0, 0, 0}, {{0, 3, 0}, 2, 7, 0, 0, 5}, {{0, 4, 0}, 0, 3, 0, 0, 0}, {{0, 5, 0}, 1, 3, 0, 0, 0}};
    c.candidates = {{0, {{1, 1.0}}, {3}}, {1, {{2, 1.0}}, {4}}};
    const PlanResult plan = planned(c);
    expect(plan.rounds == 3, "A/B/C does not settle in three rounds", plan.rounds);
    expect(plan.consumed.empty() && plan.transfers.empty(), "A/B/C consumes something", double(plan.consumed.size()));
    expect(plan.cancelled.size() == 2 && plan.cancelled[0] == 1 && plan.cancelled[1] == 0, "A/B/C cancels other than B, then A");
}

// A collides into B, which sinks under C: A's crust ends on C, by the rule between A and C.
void checkChain() {
    Case c;
    c.pieces = {{{0, 1, 0}, 0, 4, 0, 1, 0}, {{0, 2, 0}, 1, 2, 1, 3, 0, false}, {{0, 3, 0}, 2, 30, 0, 0, 100}};
    c.candidates = {{0, {{1, 1.0}}, {}}, {1, {{2, 1.0}}, {}}};
    const PlanResult plan = planned(c);
    expect(plan.consumed.size() == 2 && plan.rounds == 1, "the chain A → B → C does not consume A and B in one round");
    double onC = 0, intoInterior = 0;
    for (const Transfer& t : plan.transfers) {
        expect(t.to != 1, "a transfer ends in the consumed B");
        if (t.from == 0 && t.to == 2) onC += t.felsic, expect(t.kind == TransferKind::Collide, "A rides onto C other than by collision");
        if (t.from == 0 && t.to < 0) intoInterior += t.felsic;
    }
    expect(onC == 4 && intoInterior == 0, "A's crust does not end on C", onC);
}

// A → B → C → A: the lowest key survives this substep and the chain ends in it.
void checkCycle() {
    Case c;
    c.pieces = {{{0, 7, 0}, 0, 1, 0, 0, 50}, {{0, 3, 0}, 1, 1, 0, 0, 50}, {{0, 5, 0}, 2, 1, 0, 0, 50}};
    c.candidates = {{0, {{1, 1.0}}, {}}, {1, {{2, 1.0}}, {}}, {2, {{0, 1.0}}, {}}};
    const PlanResult plan = planned(c);
    expect(plan.deferred.size() == 1 && plan.deferred[0] == 1, "a cycle does not defer its lowest key");
    expect(plan.consumed.size() == 2, "a cycle does not consume the other two", double(plan.consumed.size()));
    for (const Transfer& t : plan.transfers) expect(t.to == 1, "a cycle's crust does not end in its survivor", t.to);
}

// Two donors of 1 into a receiver with room for 1.5: the lower key first, the rest backs up.
void checkCapacity() {
    Case c;
    c.pieces = {{{0, 1, 0}, 0, 1, 0, 0, 0}, {{0, 2, 0}, 0, 1, 0, 0, 0}, {{0, 3, 0}, 1, 20, 0, 0, 1.5}, {{0, 4, 0}, 0, 2, 0, 0, 0.25}, {{0, 5, 0}, 0, 2, 0, 0, 1}};
    c.candidates = {{1, {{2, 1.0}}, {3, 4}}, {0, {{2, 1.0}}, {3, 4}}};
    const PlanResult plan = planned(c);
    expect(plan.consumed.size() == 2 && plan.cancelled.empty(), "two donors over capacity do not both fit");
    double first = 0, second = 0, back = 0;
    for (const Transfer& t : plan.transfers) {
        if (t.to == 2) (t.from == 0 ? first : second) += t.felsic;
        if (t.kind == TransferKind::Back) back += t.felsic;
    }
    expect(first == 1 && second == 0.5 && back == 0.5, "room is not allotted by key with the surplus backed up", second);
    // Full: no room behind either, the later donor is cancelled and nothing of it moves.
    c.pieces[3].room = c.pieces[4].room = 0;
    const PlanResult full = planned(c);
    expect(full.consumed.size() == 1 && full.cancelled.size() == 1 && full.cancelled[0] == 1, "a full continent does not cancel the later donor");
    for (const Transfer& t : full.transfers) expect(t.from == 0, "a cancelled donor still moves crust");
}

// Subduction: a quarter of the sediment is scraped onto the survivor, the rest and the basalt sink.
void checkSubduction() {
    Case c;
    c.pieces = {{{0, 1, 0}, 0, 8, 4, 6, 0, false}, {{0, 2, 0}, 1, 30, 0, 0, 100}, {{0, 3, 0}, 1, 30, 0, 0, 100}};
    c.candidates = {{0, {{1, 0.75}, {2, 0.25}}, {}}};
    const PlanResult plan = planned(c);
    double accreted = 0, felsic = 0, basalt = 0;
    for (const Transfer& t : plan.transfers) {
        if (t.kind == TransferKind::Accrete) accreted += t.felsic;
        if (t.kind == TransferKind::Subduct) felsic += t.felsic, basalt += t.basalt;
    }
    expect(accreted == 1 && felsic == 7 && basalt == 6, "subduction does not scrape a quarter of the sediment", accreted);
}

} // namespace

void testGlobePlan() {
    checkABC();
    checkChain();
    checkCycle();
    checkCapacity();
    checkSubduction();
}

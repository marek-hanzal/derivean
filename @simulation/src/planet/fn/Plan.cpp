#include "planet/fn/Plan.hpp"

#include "planet/fn/Pieces.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace derivean::sim {
namespace {

// A donor fits when what is left over after its receivers and its own plate behind it is below this
// share of its crust (rounding of the shares).
constexpr double FitTolerance = 1e-12;

struct Round {
    const PlanInput& input;
    const std::vector<int>& slot;        // piece → index into input.candidates, or -1
    const std::vector<uint8_t>& active;  // per candidate index
    std::vector<int> path;
    std::vector<int> cycle;

    bool consumed(int piece) const { return slot[size_t(piece)] >= 0 && active[size_t(slot[size_t(piece)])]; }

    // Follows a donor's receivers through consumed pieces to survivors, multiplying the shares.
    // Returns false on a cycle, whose members are then in `cycle`.
    bool route(int piece, double share, std::map<PieceKey, std::pair<int, double>>& survivors) {
        path.push_back(piece);
        for (const PlanShare& to : input.candidates[size_t(slot[size_t(piece)])].receivers) {
            if (!consumed(to.piece)) {
                auto& entry = survivors[input.key[size_t(to.piece)]];
                entry.first = to.piece;
                entry.second += share * to.share;
                continue;
            }
            const auto again = std::find(path.begin(), path.end(), to.piece);
            if (again != path.end()) {
                cycle.assign(again, path.end());
                return false;
            }
            if (!route(to.piece, share * to.share, survivors)) return false;
        }
        path.pop_back();
        return true;
    }
};

int lowestKey(const PlanInput& input, const std::vector<int>& pieces) {
    return *std::min_element(pieces.begin(), pieces.end(), [&](int a, int b) { return input.key[size_t(a)] < input.key[size_t(b)]; });
}

} // namespace

PlanResult planTransfers(const PlanInput& input) {
    PlanResult result;
    std::vector<int> slot(input.key.size(), -1), order;
    for (size_t c = 0; c < input.candidates.size(); ++c) {
        slot[size_t(input.candidates[c].piece)] = int(c);
        order.push_back(int(c));
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) { return input.key[size_t(input.candidates[size_t(a)].piece)] < input.key[size_t(input.candidates[size_t(b)].piece)]; });
    std::vector<uint8_t> active(input.candidates.size(), 1);

    // Every round removes one candidate (cancelled or deferred) or confirms the plan: at most N + 1.
    for (size_t limit = 0; limit <= input.candidates.size(); ++limit) {
        ++result.rounds;
        result.transfers.clear();
        result.consumed.clear();
        Round round{input, slot, active, {}, {}};
        std::vector<std::vector<std::pair<int, double>>> routes(input.candidates.size());
        bool restart = false;
        for (const int c : order) {
            if (!active[size_t(c)]) continue;
            std::map<PieceKey, std::pair<int, double>> survivors;
            round.path.clear();
            if (!round.route(input.candidates[size_t(c)].piece, 1.0, survivors)) {
                const int keep = lowestKey(input, round.cycle);
                active[size_t(slot[size_t(keep)])] = 0;
                result.deferred.push_back(keep);
                restart = true;
                break;
            }
            for (const auto& [key, entry] : survivors) routes[size_t(c)].push_back(entry);
        }
        if (restart) continue;

        // Room is reserved from nothing in every round, donors in key order (C3).
        std::vector<double> room(input.room);
        std::vector<int> unplaced;
        for (const int c : order) {
            if (!active[size_t(c)]) continue;
            const PlanCandidate& candidate = input.candidates[size_t(c)];
            const int k = candidate.piece;
            const double felsic = input.felsic[size_t(k)], sediment = input.sediment[size_t(k)], basalt = input.basalt[size_t(k)];
            double left = 0;
            for (const auto& [r, share] : routes[size_t(c)]) {
                if (input.sinks(k, r)) {
                    const double scraped = std::min(OffscrapeShare * sediment * share, felsic * share);
                    const double put = std::clamp(room[size_t(r)], 0.0, scraped);
                    room[size_t(r)] -= put;
                    if (put > 0) result.transfers.push_back({k, r, TransferKind::Accrete, put, 0.0, 0.0});
                    result.transfers.push_back({k, -1, TransferKind::Subduct, felsic * share - put, basalt * share, 0.0});
                } else {
                    const double need = felsic * share;
                    const double put = std::clamp(room[size_t(r)], 0.0, need);
                    room[size_t(r)] -= put;
                    result.transfers.push_back({k, r, TransferKind::Collide, put, basalt * share, felsic > 0 ? put / felsic : share});
                    left += need - put;
                }
            }
            for (const int b : candidate.behind) {
                if (left <= 0) break;
                if (round.consumed(b)) continue;
                const double put = std::clamp(room[size_t(b)], 0.0, left);
                if (put <= 0) continue;
                room[size_t(b)] -= put;
                left -= put;
                result.transfers.push_back({k, b, TransferKind::Back, put, 0.0, put / felsic});
            }
            if (left > FitTolerance * felsic) unplaced.push_back(k);
            result.consumed.push_back(k);
        }
        if (unplaced.empty()) return result;
        const int cancel = lowestKey(input, unplaced);
        active[size_t(slot[size_t(cancel)])] = 0;
        result.cancelled.push_back(cancel);
    }
    globeFailure("a transfer plan did not settle in N + 1 rounds", double(input.candidates.size()));
}

} // namespace derivean::sim

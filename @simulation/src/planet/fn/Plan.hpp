#pragma once

#include "planet/type/Globe.hpp"

#include <functional>

namespace derivean::sim {

// The transfer plan of one substep (S3-design.md §4, Codex V2 and DREV-CLAUDE-003-03): decide which
// pieces are consumed, route each one's stock along chains of consumed receivers to a survivor or the
// interior, allot the receivers' room in the order of the donors' keys, and rebuild the whole plan
// from nothing whenever the set of consumed pieces changes, until it is stable. Pieces are numbered
// locally (0 .. pieces); the plan never touches the globe, the boundary executes it.

struct PlanShare {
    int piece{};
    double share{};   // of the donor's stock; a candidate's shares sum to 1
};

// A piece the overriding plates cover by half or more, with the pieces of its receiving plate in
// proportion to how much of it each one covers.
struct PlanCandidate {
    int piece{};
    std::vector<PlanShare> receivers;
    std::vector<int> behind;   // pieces of its own plate behind it, against its motion, nearest first (up to 8)
};

struct PlanInput {
    std::vector<PieceKey> key;
    std::vector<int> plate;
    std::vector<double> felsic;     // m³
    std::vector<double> sediment;   // m³ of the felsic that is cover
    std::vector<double> basalt;     // m³
    std::vector<double> room;       // m³ the piece may still take before StackLimit
    std::vector<PlanCandidate> candidates;
    // Whether a donor sinks under a survivor (subduction into the interior) or rides onto it (collision,
    // by the room). Decided between the donor and its final survivor, never inherited along a chain.
    std::function<bool(int donor, int survivor)> sinks;
};

enum class TransferKind : uint8_t {
    Subduct,   // donor → interior: felsic not scraped off, all basalt
    Accrete,   // donor → survivor: the scraped share of the sediment
    Collide,   // donor → survivor: crust stacked
    Back,      // donor → a piece of its own plate behind it: what the survivors had no room for
};

struct Transfer {
    int from{};
    int to{-1};          // -1: the interior
    TransferKind kind{};
    double felsic{};     // m³
    double basalt{};     // m³
    double cover{};      // share of the donor's cover that goes along
};

struct PlanResult {
    std::vector<Transfer> transfers;   // by donor key, then in the order the donor's routes were met
    std::vector<int> consumed;         // in key order
    std::vector<int> cancelled;        // could not be placed: survive with their overlap, brake their plate
    std::vector<int> deferred;         // lowest key of a cycle: consumed in a later substep
    int rounds{};
};

inline constexpr double OffscrapeShare = 0.25;    // of the sediment of a sinking piece, scraped onto the survivor

PlanResult planTransfers(const PlanInput& input);

} // namespace derivean::sim

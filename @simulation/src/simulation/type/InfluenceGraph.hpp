#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>

namespace derivean::sim {

enum class InfluenceTiming { SameStep, NextStep };

struct QueueStop {
    std::size_t visitsPerCell, allowance;

    constexpr std::size_t limit(std::size_t cells) const {
        if (visitsPerCell && cells > (std::numeric_limits<std::size_t>::max() - allowance) / visitsPerCell)
            throw std::overflow_error("influence iteration budget exceeds size_t");
        return cells * visitsPerCell + allowance;
    }
};

// Parts of the same solver publish together. A non-null stop rule names a group
// whose same-step feedback is handled by that solver.
struct InfluenceStage {
    std::string_view name;
    std::size_t order;
    const QueueStop* feedback{};
};

struct InfluenceLink {
    std::size_t source, target;
    InfluenceTiming time{InfluenceTiming::SameStep};
    std::string_view signal{};
};

// The declared order is over solver groups. Delayed links read the previous
// publication and never constrain the order within the current step.
constexpr bool validInfluences(std::span<const InfluenceStage> stages, std::span<const InfluenceLink> links) {
    for (std::size_t i = 0; i < stages.size(); ++i) {
        if (stages[i].name.empty()) return false;
        for (std::size_t j = 0; j < i; ++j) {
            if (stages[i].name == stages[j].name) return false;
            if (stages[i].order == stages[j].order &&
                (!stages[i].feedback || stages[i].feedback != stages[j].feedback)) return false;
        }
    }
    for (const InfluenceLink& link : links) {
        if (link.source >= stages.size() || link.target >= stages.size()) return false;
        if (link.time == InfluenceTiming::NextStep) continue;
        const auto& from = stages[link.source];
        const auto& to = stages[link.target];
        if (from.order > to.order || (from.order == to.order && !from.feedback)) return false;
    }
    return true;
}

} // namespace derivean::sim

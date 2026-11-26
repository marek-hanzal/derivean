#include "landscape/fn/Process.hpp"

#include "core/service/Workers.hpp"

namespace derivean::sim {
namespace {

constexpr int LeastRows = 16;   // rows below which a range is not worth handing to another thread

} // namespace

void forRows(const Domain& domain, int count, const std::function<void(int, int)>& body) {
    if (domain.workers) domain.workers->parallelFor(count, body, LeastRows);
    else body(0, count);
}

} // namespace derivean::sim

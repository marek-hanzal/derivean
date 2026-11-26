#include "aeolian/fn/Dunes.hpp"

namespace derivean::sim {
namespace {

constexpr int StepX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
constexpr int StepZ[8] = {0, 1, 1, 1, 0, -1, -1, -1};
constexpr float Reach[8] = {1, 1.41421356f, 1, 1.41421356f, 1, 1.41421356f, 1, 1.41421356f};
constexpr float Slack = 1e-3f;    // m above repose a pair may stand before it is worth a move
constexpr int MostMoves = 8192;   // per call; a slide that long has left the neighbourhood it began in

} // namespace

// A cell slides towards its steepest lower neighbour by what the pair stands above repose; a
// neighbour that now overhangs it is looked at in turn. Held and wet cells keep their height: a held
// cell feeds the slope below it without sinking, and sand that slides onto either is gone.
void slideSand(SandBed& bed, size_t cell, std::vector<int32_t>& stack) {
    const int size = bed.size;
    const float limit = bed.reposeTan * float(bed.cell), area = float(bed.cell * bed.cell);
    stack.clear();
    stack.push_back(int32_t(cell));
    for (int moves = 0; !stack.empty() && moves < MostMoves; ++moves) {
        const size_t c = size_t(stack.back());
        stack.pop_back();
        const BedCell kind = bed.kind[c];
        if (kind == BedCell::Wall || kind == BedCell::Wet) continue;
        const int x = int(c % size_t(size)), z = int(c / size_t(size));
        const float here = bed.top(c);
        int best = -1;
        float steepest = limit + Slack;
        for (int k = 0; k < 8; ++k) {
            const int nx = x + StepX[k], nz = z + StepZ[k];
            if (nx < 0 || nz < 0 || nx >= size || nz >= size) continue;
            const size_t n = bed.index(nx, nz);
            if (bed.kind[n] == BedCell::Wall) continue;
            const float fall = (here - bed.top(n)) / Reach[k];
            if (fall > steepest) steepest = fall, best = k;
            else if (-fall > limit + Slack && bed.sand[n] > 0 && bed.kind[n] != BedCell::Wet) stack.push_back(int32_t(n));
        }
        if (best < 0 || bed.sand[c] <= 0) continue;
        const size_t n = bed.index(x + StepX[best], z + StepZ[best]);
        const BedCell other = bed.kind[n];
        if (kind == BedCell::Held && other != BedCell::Open) continue;
        const float excess = here - bed.top(n) - limit * Reach[best];
        float move = kind == BedCell::Open && other == BedCell::Open ? 0.5f * excess : excess;
        if (kind == BedCell::Open) {
            move = std::min(move, bed.sand[c]);
            bed.sand[c] -= move;
            if (other == BedCell::Held) bed.ledger.left += double(move) * area;
            if (other == BedCell::Wet) bed.ledger.drowned += double(move) * area;
        } else {
            bed.ledger.entered += double(move) * area;
        }
        if (other == BedCell::Open) bed.sand[n] += move, stack.push_back(int32_t(n));
        stack.push_back(int32_t(c));
    }
}

void slideAll(SandBed& bed) {
    std::vector<int32_t> stack;
    for (int pass = 0; pass < 4; ++pass)
        for (size_t i = 0; i < bed.cells(); ++i)
            if (bed.kind[i] == BedCell::Open && bed.sand[i] > 0) slideSand(bed, i, stack);
}

} // namespace derivean::sim

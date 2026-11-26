#include "aeolian/fn/Dunes.hpp"

namespace derivean::sim {
namespace {

constexpr int StepX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
constexpr int StepZ[8] = {0, 1, 1, 1, 0, -1, -1, -1};
constexpr float Reach[8] = {1, 1.41421356f, 1, 1.41421356f, 1, 1.41421356f, 1, 1.41421356f};
constexpr float Slack = 1e-3f;    // m above repose a pair may stand before it is worth a move
constexpr int MostMoves = 8192;   // per call; a slide that long has left the neighbourhood it began in

// Moves what a pair stands above repose from its high cell to its low one and returns it. Between
// two open cells half the excess evens the pair out; held and wet cells keep their height: a held
// cell feeds the slope below it without sinking, and sand that slides onto either is gone.
float shed(SandBed& bed, size_t high, size_t low, float excess) {
    const BedCell from = bed.kind[high], to = bed.kind[low];
    if (from == BedCell::Wet || bed.sand[high] <= 0 || (from == BedCell::Held && to != BedCell::Open)) return 0;
    const double area = bed.cell * bed.cell;
    float move = from == BedCell::Open && to == BedCell::Open ? 0.5f * excess : excess;
    if (from == BedCell::Open) {
        move = std::min(move, bed.sand[high]);
        bed.sand[high] -= move;
        if (to == BedCell::Held) bed.ledger.left += move * area;
        if (to == BedCell::Wet) bed.ledger.drowned += move * area;
    } else {
        bed.ledger.entered += move * area;
    }
    if (to == BedCell::Open) bed.sand[low] += move;
    return move;
}

} // namespace

void slideSand(SandBed& bed, size_t cell, std::vector<int32_t>& stack) {
    const int size = bed.size;
    const float limit = bed.reposeTan * float(bed.cell);
    stack.clear();
    stack.push_back(int32_t(cell));
    for (int moves = 0; !stack.empty() && moves < MostMoves; ++moves) {
        const size_t c = size_t(stack.back());
        stack.pop_back();
        if (bed.kind[c] == BedCell::Wall || bed.kind[c] == BedCell::Wet) continue;
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
            // A neighbour that now overhangs this cell is looked at in its turn.
            else if (-fall > limit + Slack && bed.sand[n] > 0 && bed.kind[n] != BedCell::Wet) stack.push_back(int32_t(n));
        }
        if (best < 0) continue;
        const size_t n = bed.index(x + StepX[best], z + StepZ[best]);
        if (shed(bed, c, n, here - bed.top(n) - limit * Reach[best]) <= 0) continue;
        if (bed.kind[n] == BedCell::Open) stack.push_back(int32_t(n));
        stack.push_back(int32_t(c));
    }
}

void slidePairs(SandBed& bed, int turn) {
    const int size = bed.size;
    const float limit = bed.reposeTan * float(bed.cell);
    // A slide runs on within one sweep only in the direction of the sweep; the four directions take turns.
    const bool backX = turn & 1, backZ = turn & 2;
    for (int row = 0; row < size; ++row) {
        const int z = backZ ? size - 1 - row : row;
        for (int column = 0; column < size; ++column) {
            const int x = backX ? size - 1 - column : column;
            const size_t a = bed.index(x, z);
            if (bed.kind[a] == BedCell::Wall) continue;
            // East, south-east, south and south-west: every pair of neighbours once.
            for (int k = 0; k < 4; ++k) {
                const int nx = x + StepX[k], nz = z + StepZ[k];
                if (nx < 0 || nx >= size || nz >= size) continue;
                const size_t b = bed.index(nx, nz);
                if (bed.kind[b] == BedCell::Wall) continue;
                const float fall = bed.top(a) - bed.top(b), excess = std::abs(fall) - limit * Reach[k];
                if (excess > Slack) shed(bed, fall > 0 ? a : b, fall > 0 ? b : a, excess);
            }
        }
    }
}

void slideAll(SandBed& bed) {
    std::vector<int32_t> stack;
    for (int pass = 0; pass < 4; ++pass)
        for (size_t i = 0; i < bed.cells(); ++i)
            if (bed.kind[i] == BedCell::Open && bed.sand[i] > 0) slideSand(bed, i, stack);
}

} // namespace derivean::sim

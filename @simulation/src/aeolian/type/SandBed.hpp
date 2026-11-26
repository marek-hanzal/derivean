#pragma once

#include <cstdint>
#include <vector>

namespace derivean::sim {

class Workers;

// How a cell of a sand bed takes part in the transport.
enum class BedCell : uint8_t {
    Open,   // loses and gains sand
    Held,   // a finished neighbour's ground: it sends sand and takes it, and stays as it is
    Wet,    // water: what lands in it is gone from the wind, and it gives none
    Wall,   // the outer ring of a domain: no sand lands on it or leaves it
};

// m³ of sand, counted once each.
struct BedLedger {
    double entered{};   // from held cells into open ones
    double left{};      // from open cells onto held ones
    double drowned{};   // from open cells into water
};

// A square of ground as the wind meets it: a surface it cannot move and loose dry sand on that.
// Sand that leaves across an edge comes back in across the opposite one, so a bed without held
// cells keeps exactly the sand it was given: the bed stands for a piece of a wider field in the
// same state, and what the field as a whole gains or loses is decided a level above.
struct SandBed {
    int size{};                   // cells per side
    double cell{};                // m
    float reposeTan{0.62f};       // rise over run at which loose sand slides
    uint64_t seed{};              // every draw of a run follows from it
    std::vector<float> base;      // m, top of what the wind cannot move
    std::vector<float> sand;      // m of loose dry sand on it
    std::vector<BedCell> kind;
    BedLedger ledger;
    Workers* workers{};           // pool the rounds may spread rows over; the result is the same without

    size_t index(int x, int z) const { return size_t(z) * size_t(size) + size_t(x); }
    size_t cells() const { return size_t(size) * size_t(size); }
    float top(size_t i) const { return base[i] + sand[i]; }
};

// How long and how finely a bed is worked.
struct DuneRun {
    int rounds{};         // every open cell is asked for a slab once per round, on average
    float slab{};         // m of sand one draw lifts
    int hop{};            // cells a slab travels before it may come down
    int year{16};         // rounds after which the winds of the rose have each had their turn
};

} // namespace derivean::sim

#pragma once

#include <cstdint>

namespace derivean::sim {

// Every seeded decision in the simulation goes through these integer hashes, so a result depends
// only on the seed and the coordinates that were hashed, never on evaluation order.
inline constexpr uint64_t scramble(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

inline constexpr uint64_t hash(uint64_t seed, int64_t a, int64_t b = 0, int64_t c = 0) {
    uint64_t state = scramble(seed ^ (uint64_t(a) * 0xd6e8feb86659fd93ULL));
    state = scramble(state ^ (uint64_t(b) * 0xa0761d6478bd642fULL));
    return scramble(state ^ (uint64_t(c) * 0xe7037ed1a0b428dbULL));
}

// Uniform in [0, 1).
inline constexpr double unit(uint64_t bits) {
    return double(bits >> 11) * (1.0 / 9007199254740992.0);
}

inline constexpr double unit(uint64_t seed, int64_t a, int64_t b = 0, int64_t c = 0) {
    return unit(hash(seed, a, b, c));
}

// Uniform in [low, high).
inline constexpr double ranged(uint64_t bits, double low, double high) {
    return low + (high - low) * unit(bits);
}

// Named sub-streams keep independent subsystems from sharing random decisions.
inline constexpr uint64_t stream(uint64_t seed, uint64_t purpose) {
    return scramble(seed * 0x2545f4914f6cdd1dULL + purpose);
}

} // namespace derivean::sim

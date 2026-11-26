#pragma once

#include "planet/type/Globe.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace derivean::sim::globe_test {

inline void expect(bool passed, const char* message, double value = 0) {
    if (passed) return;
    std::cerr << "Globe: " << message << " (" << value << ")\n";
    std::exit(1);
}

// A reproducible uniform number in [0, 1) and unit directions.
struct Random {
    std::uint64_t state{};
    double next() {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        return double(state >> 11) / double(1ull << 53);
    }
    Vec3 direction() {
        const double z = 2 * next() - 1, angle = 6.283185307179586 * next(), r = std::sqrt(1 - z * z);
        return {r * std::cos(angle), r * std::sin(angle), z};
    }
};

template <class T> bool sameBits(const T& a, const T& b) { return std::memcmp(&a, &b, sizeof(T)) == 0; }

// Every piece of one plate with one Euler vector, as a rigid turn of the whole crust.
inline void onePlate(Globe& globe, Vec3 euler) {
    for (CrustPiece& piece : globe.pieces) piece.plate = 0;
    globe.applied.assign(1, euler);
}

void checkZeroRotation();
void checkRigidTurn();
void checkSeamCrossing();
void checkKinematics();

} // namespace derivean::sim::globe_test

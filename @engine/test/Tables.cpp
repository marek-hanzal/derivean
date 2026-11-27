// Tables committed as data against what they were made from.
#include "water/constant/ParcelRoots.hpp"
#include "water/type/Water.hpp"

#include "Expect.hpp"

#include <cmath>
#include <iostream>

using namespace derivean;

void testTables() {
    // The C library may differ in the last bit between platforms; the table is macOS's, so elsewhere it agrees
    // to within that bit and on macOS exactly.
    int exact = 0;
    for (int amount = 0; amount <= WaterUnits; ++amount) {
        volatile double share = amount / double(WaterUnits);
        const double root = std::cbrt(share);
        expect(std::abs(ParcelRoots[size_t(amount)] - root) <= 2.3e-16 * root, "A parcel's cube root must be std::cbrt's");
        exact += ParcelRoots[size_t(amount)] == root;
    }
#ifdef __APPLE__
    expect(exact == WaterUnits + 1, "On macOS the parcel roots must be std::cbrt's to the bit");
#endif
    std::cout << "tables: parcel roots, " << exact << " of " << WaterUnits + 1 << " exact\n";
}

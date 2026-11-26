#pragma once

#include <cstdlib>
#include <iostream>

// A failed check reports to stderr and exits non-zero, so reaching the end of main means every check passed.
inline void expect(bool passed, const char* message) {
    if (passed) return;
    std::cerr << message << '\n';
    std::exit(1);
}

#pragma once

#include <cstdint>

// Prints of the tiles test/Fingerprint.cpp checks, per platform: the maths libraries of macOS and
// Windows differ in the last bits, and so do the worlds built on them. Data, rewritten from the
// lines the test prints when a change is meant to move the world.
struct StoredPrint {
    const char* platform;
    const char* world;
    int level;
    int32_t x, z;
    uint64_t print;
};

inline constexpr StoredPrint StoredPrints[] = {
    {"macos", "young", 1, 56, 36, 0x2ba87a70568c9df1ULL},
    {"macos", "young", 2, 906, 584, 0xc55ce3e430692a13ULL},
    {"macos", "young", 3, 14504, 9352, 0x16fba40b407f2605ULL},
    {"macos", "young", 4, 232073, 149640, 0x9a537e58f96e1ed7ULL},
    {"macos", "present", 1, 44, 16, 0x5e7fa0cf9edc48d9ULL},
    {"macos", "present", 2, 705, 261, 0xa32dc31e95ae1789ULL},
    {"macos", "present", 1, 21, 38, 0x9f3a02bd19cd18aaULL},
    {"macos", "present", 3, 5474, 9868, 0xa07b70497f92bda7ULL},
    {"windows", "young", 1, 56, 36, 0x3b53c275dd471d8dULL},
    {"windows", "young", 2, 906, 584, 0x8866bff89f0a4674ULL},
    {"windows", "young", 3, 14504, 9352, 0x838ec9a45ef7f755ULL},
    {"windows", "young", 4, 232073, 149640, 0x0086b2549f0c1e7aULL},
    {"windows", "present", 1, 44, 16, 0x681a7bf02e01bb23ULL},
    {"windows", "present", 2, 705, 261, 0x4703a8fd1c69adf7ULL},
    {"windows", "present", 1, 21, 38, 0x44e65727e386c583ULL},
    {"windows", "present", 3, 5474, 9868, 0x460463857da56885ULL},
};

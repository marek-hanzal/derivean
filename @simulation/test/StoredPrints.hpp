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
    {"macos", "young", 1, 56, 36, 0xf1f28c8361ae4138ULL},
    {"macos", "young", 2, 906, 584, 0xc55ce3e430692a13ULL},
    {"macos", "young", 3, 14504, 9352, 0x16fba40b407f2605ULL},
    {"macos", "young", 4, 232073, 149640, 0x9a537e58f96e1ed7ULL},
    {"macos", "present", 1, 44, 16, 0xc8fa95680d1cfbe4ULL},
    {"macos", "present", 2, 705, 261, 0x10b5f02ea365d9acULL},
    {"macos", "present", 1, 21, 38, 0x4cac3f0df1b6ccf5ULL},
    {"macos", "present", 3, 5474, 9868, 0x92282235b010277fULL},
    {"windows", "young", 1, 56, 36, 0xffd69df674a0cb6bULL},
    {"windows", "young", 2, 906, 584, 0x8866bff89f0a4674ULL},
    {"windows", "young", 3, 14504, 9352, 0x838ec9a45ef7f755ULL},
    {"windows", "young", 4, 232073, 149640, 0x0086b2549f0c1e7aULL},
    {"windows", "present", 1, 44, 16, 0xdd1156db224ef0d4ULL},
    {"windows", "present", 2, 705, 261, 0x0bb05147f725acdcULL},
    {"windows", "present", 1, 21, 38, 0xd57996c1ea3705d6ULL},
    {"windows", "present", 3, 5474, 9868, 0xfd23310259538aa3ULL},
};
